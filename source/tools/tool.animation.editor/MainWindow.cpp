
#include    "MainWindow.h"
#include    <fstream>
#include    "FEFileFormatHelper.hpp"
#include    "fileFormat/fepk/FEFormatFepj.hpp"
#include    "graphic/FEScene.h"
#include    "FEInputSystem.hpp"
#include    "axis/FENodeMoveEditor.h"
#include    "axis/FENodeRotateEditor.h"
#include    "axis/FENodeScaleEditor.h"
#include    "animation/FEAnimationSys.hpp"
#include    "animation/FEAnimationHelper.hpp"
#include    "UiProperties.h"
#include    "FEAnimationXmlIO.hpp"
#include    "UndoCommand.h"

MainWindow* _mainApp   =   nullptr;

MainWindow::MainWindow()
{
    _mainApp    =   this;

    ui.setupUi(this);

    /// 属性面板嵌入 widget_properties 占位容器(零边距)
    _propsUi    =   new UiProperties(ui.widget_properties);
    {
        auto    propsLayout =   new QVBoxLayout(ui.widget_properties);
        propsLayout->setContentsMargins(0, 0, 0, 0);
        propsLayout->addWidget(_propsUi);
    }

    ui.action_savePrj->setShortcut(QKeySequence::Save);
    ui.action_undo->setShortcut(QKeySequence::Undo);
    ui.action_redo->setShortcut(QKeySequence::Redo);

    /// 复制/剪切/粘贴快捷键: 作为 MainWindow 级 QAction,不依赖 QtTree 键盘焦点
    {
        auto* actCopy = new QAction(this);
        actCopy->setShortcut(QKeySequence::Copy);
        connect(actCopy, &QAction::triggered, this, [this]()
        {
            auto cur = ui.modelTree->currentItem();
            if (cur && cur->cast<FENode>())
                ctxCopyNode(cur->cast<FENode>());
        });
        addAction(actCopy);

        auto* actCut = new QAction(this);
        actCut->setShortcut(QKeySequence::Cut);
        connect(actCut, &QAction::triggered, this, [this]()
        {
            auto cur = ui.modelTree->currentItem();
            if (cur && cur->cast<FENode>())
                ctxCutNode(cur->cast<FENode>());
        });
        addAction(actCut);

        auto* actPaste = new QAction(this);
        actPaste->setShortcut(QKeySequence::Paste);
        connect(actPaste, &QAction::triggered, this, [this]()
        {
            ctxPasteNode(_clipNode.get());
        });
        addAction(actPaste);
    }

    ui.timeLineEditor->linkScrollBar(ui.horizontalScrollBar);
    ui.splitter->setSizes({200, 800});
    ui.animationTree->linkToTickMgr(ui.timeLineEditor);
    
    ui.modelTree->setup(ui.sceneViewer->scene());
    ui.sceneViewer->notify()    +=  {this,&MainWindow::notifyEngineStart};

    /// 模型树右键菜单: 添加/移除到动画树 + 创建/删除/清除动画
    connect(ui.modelTree, &QtTree::signalContextMenu, this
        , [this](const QPoint& pt, Object obj)
    {
        if (!obj)
            return;
        auto    node    =   obj->cast<FENode>();
        auto    anim    =   obj->cast<FEAnimation>();
        if  (!node && !anim)
            return;
        QMenu   menu(ui.modelTree);

        if  (node)
        {
            /// FENode: 添加/移除到动画树 + 创建动画 + 清除动画
            bool    inTree  =   ui.animationTree->containsObject(obj);
            auto    action  =   menu.addAction(inTree ? u8"从动画树移除" : u8"添加到动画树");
            connect(action, &QAction::triggered, this, [this, obj]()
            {
                double  curTime =   ui.timeLineEditor->curTime();
                if  (_undoStack)
                {
                    bool    inTree  =   ui.animationTree->containsObject(obj);
                    _undoStack->beginMacro(inTree ? u8"从动画树移除" : u8"添加到动画树");
                    _undoStack->push(new FE::ToggleTreeObjectCmd(ui.animationTree, obj, curTime));
                    _undoStack->endMacro();
                }
                else
                {
                    ui.animationTree->toggleObject(obj, curTime);
                }
            });

            menu.addSeparator();

            /// 添加已有动画到动画树: 递归遍历所有子节点,将有动画的节点添加到动画树
            auto    addExistAnim =   menu.addAction(u8"添加已有动画到动画树");
            connect(addExistAnim, &QAction::triggered, this, [this, node]()
            {
                /// 递归收集有动画的节点
                std::vector<FE::FENode*>   nodesWithAnim;
                std::function<void(FE::FENode*)>  collect;
                collect    =   [&](FE::FENode* n)
                {
                    if  (!n)    return;
                    auto    anims   =   n->objects<FEAnimation>();
                    if  (!anims.empty())
                        nodesWithAnim.push_back(n);
                    for (auto& child : n->children())
                        collect(child.get());
                };
                collect(node);

                double  curTime =   ui.timeLineEditor->curTime();
                if  (_undoStack)
                {
                    _undoStack->beginMacro(u8"添加已有动画到动画树");
                    for (auto* n : nodesWithAnim)
                    {
                        if  (!ui.animationTree->containsObject(n))
                            _undoStack->push(new FE::ToggleTreeObjectCmd(ui.animationTree, n, curTime));
                    }
                    _undoStack->endMacro();
                }
                else
                {
                    for (auto* n : nodesWithAnim)
                    {
                        if  (!ui.animationTree->containsObject(n))
                            ui.animationTree->toggleObject(n, curTime);
                    }
                }
            });

            /// 创建动画
            auto    createAnim =   menu.addAction(u8"创建动画");
            connect(createAnim, &QAction::triggered, this, [this, node]()
            {
                auto    anim    =   FEAnimationHelper::createNodeAnimtion(node->ctx());
                if  (_undoStack)
                {
                    _undoStack->beginMacro(u8"添加动画");
                    _undoStack->push(new FE::CreateAnimCmd(node, anim, [this]{ ui.animationTree->updateUi(); }));
                    _undoStack->endMacro();
                }
                else
                {
                    node->addComponent(anim.get());
                    auto    scene   =   node->ctx().scene();
                    if  (scene)
                    {
                        auto    sys =   scene->animationSystem();
                        if  (sys)   sys->addObject(anim.get());
                    }
                    ui.animationTree->updateUi();
                }
            });

            /// 清除动画(清除动画树中所有节点的动画)
            auto    clearAnim  =   menu.addAction(u8"清除动画");
            connect(clearAnim, &QAction::triggered, this, [this]()
            {
                ui.animationTree->slotClearAllAnimations();
            });

            menu.addSeparator();

            /// 复制节点(Ctrl+C)
            auto    actCopy  =   menu.addAction(u8"复制");
            connect(actCopy, &QAction::triggered, this, [this, node]()
            {
                ctxCopyNode(node);
            });
            /// 粘贴节点(Ctrl+V): 复制=克隆为兄弟,剪切=移动到本节点下
            auto    actPaste  =   menu.addAction(u8"粘贴");
            connect(actPaste, &QAction::triggered, this, [this, node]()
            {
                ctxPasteNode(_clipNode.get());
            });
            /// 剪切节点(Ctrl+X)
            auto    actCut   =   menu.addAction(u8"剪切");
            connect(actCut, &QAction::triggered, this, [this, node]()
            {
                ctxCutNode(node);
            });

            menu.addSeparator();

            /// 导出节点动画:导出该节点上的全部动画到 XML
            auto    exportAnim  =   menu.addAction(u8"导出节点动画");
            connect(exportAnim, &QAction::triggered, this, [this, node]()
            {
                auto    anims   =   node->objects<FEAnimation>();
                if  (anims.empty())
                {
                    QMessageBox::information(this, C2Q("提示"), C2Q("该节点没有可导出的动画!"), QMessageBox::Ok);
                    return;
                }
                auto    filePath    =   QFileDialog::getSaveFileName(this, C2Q("导出节点动画"), "D:/"
                    , C2Q("动画 XML(*.xml)"));
                if  (filePath.isEmpty())
                    return;

                std::vector<FE::FEAnimationXmlIO::AnimExportItem>  items;
                for (auto* a : anims)
                    items.push_back({a, node->objectId()});

                std::string err;
                if  (!FE::FEAnimationXmlIO::saveAnimsToXml(items, filePath.toStdString(), &err))
                    QMessageBox::warning(this, C2Q("提示"), C2Q(err.c_str()), QMessageBox::Ok);
            });

            /// 导入节点动画:从 XML 读取动画并挂接到该节点
            auto    importAnim  =   menu.addAction(u8"导入节点动画");
            connect(importAnim, &QAction::triggered, this, [this, node]()
            {
                auto    filePath    =   QFileDialog::getOpenFileName(this, C2Q("导入节点动画"), "D:/"
                    , C2Q("动画 XML(*.xml)"));
                if  (filePath.isEmpty())
                    return;

                std::string err;
                auto    entries =   FE::FEAnimationXmlIO::loadAnimsFromXml(node->ctx(), filePath.toStdString(), &err);
                if  (entries.empty())
                {
                    QMessageBox::warning(this, C2Q("提示")
                        , C2Q(err.empty() ? u8"XML 中没有可导入的动画!" : err.c_str()), QMessageBox::Ok);
                    return;
                }

                auto    refreshCb   =   [this]
                {
                    ui.animationTree->updateUi();
                    ui.timeLineEditor->refreshKeyframes();
                    noticeUpdatePropsUi();
                };
                _undoStack->beginMacro(u8"导入节点动画");
                for (auto& e : entries)
                    _undoStack->push(new FE::CreateAnimCmd(node, e.anim, refreshCb));
                _undoStack->endMacro();
            });
        }
        else    /// FEAnimation
        {
            /// FEAnimation: 添加到动画树(同步选中) + 删除动画
            auto    owner       =   anim->owner();
            auto    ownerNode   =   owner ? owner->cast<FENode>() : nullptr;
            bool    inTree      =   ownerNode && ui.animationTree->containsObject(ownerNode);
            auto    action      =   menu.addAction(inTree ? u8"从动画树移除" : u8"添加到动画树");
            connect(action, &QAction::triggered, this, [this, ownerNode, anim]()
            {
                if  (!ownerNode)
                    return;
                double  curTime =   ui.timeLineEditor->curTime();
                if  (_undoStack)
                {
                    bool    inTree  =   ui.animationTree->containsObject(ownerNode);
                    _undoStack->beginMacro(inTree ? u8"从动画树移除" : u8"添加到动画树");
                    _undoStack->push(new FE::ToggleTreeObjectCmd(ui.animationTree, ownerNode, curTime));
                    _undoStack->endMacro();
                }
                else
                {
                    ui.animationTree->toggleObject(ownerNode, curTime);
                }
            });

            menu.addSeparator();

            /// 删除动画
            auto    deleteAnim =   menu.addAction(u8"删除动画");
            connect(deleteAnim, &QAction::triggered, this, [this, anim, ownerNode]()
            {
                if  (_undoStack)
                {
                    FE::DeleteAnimCmd::AnimInfos   infos;
                    infos.push_back({ownerNode, anim});
                    _undoStack->beginMacro(u8"删除动画");
                    _undoStack->push(new FE::DeleteAnimCmd(std::move(infos), [this]{ ui.animationTree->updateUi(); }));
                    _undoStack->endMacro();
                }
                else
                {
                    auto    scene   =   anim->ctx().scene();
                    if  (scene)
                    {
                        auto    sys =   scene->animationSystem();
                        if  (sys)   sys->removeObject(anim);
                    }
                    if  (ownerNode)
                        ownerNode->removeComponent(anim);
                    ui.animationTree->updateUi();
                }
            });
        }

        menu.exec(ui.modelTree->mapToGlobal(pt));
    });

    /// 模型树拖拽投放: 按投放模式分发到重挂父或同父排序
    /// 跨父 DM_Before/DM_After 退化为重挂到目标父(成兄弟,排序顺序由 addChild 决定)
    connect(ui.modelTree, &QtTree::signalNodeDropped, this
        , [this](Object obj, Object target, FE::DropMode mode)
    {
        auto    node        =   obj ? obj->cast<FENode>() : nullptr;
        if  (!node)
            return;
        auto    targetNode  =   target ? target->cast<FENode>() : nullptr;
        switch (mode)
        {
        case FE::DM_Top:
            ctxReparentNode(node, nullptr);
            break;
        case FE::DM_Into:
            ctxReparentNode(node, targetNode);
            break;
        case FE::DM_Before:
            /// 同父 = 兄弟排序;跨父 = 重挂到目标父
            if  (targetNode && node->parent().get() == targetNode->parent().get())
                ctxReorderNode(node, targetNode, false);
            else if (targetNode)
                ctxReparentNode(node, targetNode->parent().get());
            break;
        case FE::DM_After:
            if  (targetNode && node->parent().get() == targetNode->parent().get())
                ctxReorderNode(node, targetNode, true);
            else if (targetNode)
                ctxReparentNode(node, targetNode->parent().get());
            break;
        default:
            break;
        }
    });

    /// 复制节点到剪贴板(Ctrl+C): 仅记录引用,延迟到粘贴时克隆
    connect(ui.modelTree, &QtTree::signalCopyNode, this, [this](Object obj)
    {
        ctxCopyNode(obj ? obj->cast<FENode>() : nullptr);
    });

    /// 剪切节点到剪贴板(Ctrl+X): 标记半透明绘制,粘贴时移动
    connect(ui.modelTree, &QtTree::signalCutNode, this, [this](Object obj)
    {
        ctxCutNode(obj ? obj->cast<FENode>() : nullptr);
    });

    /// 粘贴(Ctrl+V): 剪切=移动原节点,复制=克隆为源父兄弟
    connect(ui.modelTree, &QtTree::signalPasteNode, this, [this]()
    {
        ctxPasteNode(_clipNode.get());
    });

    /// Esc 取消剪切: 清空剪贴板(树侧已自行恢复节点正常绘制)
    connect(ui.modelTree, &QtTree::signalCancelCut, this, [this]()
    {
        _clipNode    =   nullptr;
        _clipCut     =   false;
    });

    /// F2 重命名: 名称变化时 push RenameNodeCmd
    connect(ui.modelTree, &QtTree::signalRenameRequested, this
        , [this](Object obj, const QString& newName)
    {
        auto    node    =   obj ? obj->cast<FENode>() : nullptr;
        if  (!node)
            return;
        QString oldName =   C2Q(node->name());
        if  (newName.isEmpty() || newName == oldName)
            return;
        UNDO_STACK->beginMacro(u8"重命名节点");
        UNDO_STACK->push(new FE::RenameNodeCmd(Node(node)
            , newName.toUtf8().data(), oldName.toUtf8().data()));
        UNDO_STACK->endMacro();
        refreshModelTree();
        noticeUpdatePropsUi();
    });

    /// 将模型树链接到时间线编辑器,用于获取选中节点添加关键帧
    ui.timeLineEditor->setModelTree(ui.modelTree);

    _undoStack  =   new QUndoStack(this);
    ui.undoView->setStack(_undoStack);
    ui.timeLineEditor->setUndoStack(_undoStack);
    ui.animationTree->setUndoStack(_undoStack);

    /// 模型树选择事件: 选中动画对象时同步选中动画树对应项
    ui.modelTree->_selectEvts += {this,[this](Object obj, bool)
    {
        if  (!obj)
            return;
        auto    anim    =   obj->cast<FEAnimation>();
        if  (anim)
            ui.animationTree->selectItemByObject(anim);
    }};

    /// 模型树选择事件: 同步刷新属性面板(节点/动画)
    ui.modelTree->_selectEvts += {this,[this](Object obj, bool)
    {
        if  (_propsUi && obj)
            _propsUi->slotSelectObject(obj);
    }};

    /// 时间线游标移动/关键帧变化:刷新属性面板轨道勾选态
    connect(ui.timeLineEditor, SIGNAL(sigCurFrameChanged(double)), this, SLOT(slotPropsCurTimeChanged(double)));
    connect(ui.timeLineEditor, &UiTickMgr::sigKeyframesChanged, this, [this]{ noticeUpdatePropsUi(); });

    /// 时间线编辑通知
    /// 用来控制动画
    connect(ui.timeLineEditor,  SIGNAL(sigCurFrameChanged(double)), ui.sceneViewer,     SLOT(slotTimeLineChanged(double)));
    connect(ui.timeLineEditor,  SIGNAL(sigKeyframesChanged()),      ui.sceneViewer,     SLOT(slotKeyframesChanged()));
    connect(ui.frontRunBtn,     SIGNAL(clicked()),                  ui.timeLineEditor,  SLOT(slotPlayToNextFrame()));
    connect(ui.pushButtonAddKey,SIGNAL(clicked()),                  ui.timeLineEditor,  SLOT(slotAddKeyframe()));

    setTitile("");

    connect(ui.action_importModel,  SIGNAL(triggered()),    this,   SLOT(slotImportModel()));
    connect(ui.action_openPrj,      SIGNAL(triggered()),    this,   SLOT(slotOpenProject()));
    connect(ui.action_savePrj,      SIGNAL(triggered()),    this,   SLOT(slotSaveProject()));
    connect(ui.actionReset,         SIGNAL(triggered()),    this,   SLOT(slotReset()));
    connect(ui.action_resetLayout,  SIGNAL(triggered()),    this,   SLOT(slotResetLayout()));
    connect(ui.action_redo,         SIGNAL(triggered()),    this,   SLOT(slotRedo()));
    connect(ui.action_undo,         SIGNAL(triggered()),    this,   SLOT(slotUndo()));

    connect(ui.action_move,         SIGNAL(triggered()),    this,   SLOT(slotMoveEditor()));
    connect(ui.action_rotation,     SIGNAL(triggered()),    this,   SLOT(slotRotEditor()));
    connect(ui.action_scale,        SIGNAL(triggered()),    this,   SLOT(slotScaleEditor()));
}
MainWindow::~MainWindow()
{
    ui.sceneViewer->notify().clear();
    ui.modelTree->_selectEvts.clear();
}
Scene   MainWindow::scene()
{
    return  ui.sceneViewer->scene();
}

void    MainWindow::setTitile(QString fileName)
{
    if (fileName.isEmpty())
        setWindowTitle("FEEditor - unnamed.fepj*");
    else
        setWindowTitle("FEEditor - " + fileName);

}
void    MainWindow::disableAllAnimations()
{
    auto    scene   =   this->scene();
    if (!scene)
        return;
    auto    sys     =   scene->animationSystem();
    if (!sys)
        return;
    /// 遍历动画系统中所有 action,禁用每个 action 管理的动画
    for (auto& pair : sys->actions())
    {
        auto&   action  =   pair.second;
        auto&   anims   =   action->objects();
        for (auto& anim : anims)
        {
            anim->setEnable(false);
        }
    }
}

void    MainWindow::slotImportModel()
{
    String  sptList =   "";
    auto    readers =   scene()->ctx().readers();
    size_t  cnt     =   readers.data().size();
    size_t  index   =   0;
    for (auto& var : readers.data())
    {
        auto    filter  =   var.second.toString();
        sptList += filter;
        if (index != cnt - 1)
            sptList += ";;";
        ++index;
    }
    /// 打开加载模型对话框
    /// 动态获取支持导入的模型格式
    // 相对路径转绝对路径
    QString     qFilter = "";
    auto        fileNames = QFileDialog::getOpenFileNames(this
        , C2Q("导入模型文件")
        , ""
        , C2Q(sptList.c_str())
        , &qFilter);

    if (fileNames.isEmpty())
        return;
    FE::Format  fmt;
    if(!readers.query(qFilter.toStdString(),fmt))
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("没有适合的格式化组件!"), QMessageBox::Ok);
        return;
    }
        
    auto        reader  =   FEFileFormatHelper::queryReader(FEContext::instance(),fmt);
    if (reader == nullptr)
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("没有查到适合的格式化组件!"), QMessageBox::Ok);
        return;
    }
    Strings     files;
    String      filter  =   qFilter.toStdString().c_str();
    for (auto& var : fileNames)
    {
        String  file    =   var.toLocal8Bit().data();
        files.push_back(file);
    }
    auto    objects =   reader->readFiles(files);
    Nodes   nodes;
    for (auto var : objects)
    {   
        Node    node    =   var->cast<FENode>();
        if (node == nullptr)
            continue;
        else
            nodes.push_back(node);
    }
    scene()->dispatchNodesToSystem(nodes);
    scene()->addNodesToTree(nodes);
    /// 模型导入后禁用所有动画
    disableAllAnimations();
}
void    MainWindow::slotOpenProject()
{
    auto    fileName    =   QFileDialog::getOpenFileName(this
        , C2Q("打开工程")
        , qApp->applicationDirPath()
        , C2Q("工程文件(*.fepj)"));

    String  gbkName    =   fileName.toLocal8Bit().data();
    if (scene()->open(gbkName.c_str()))
    {
        _projectName    =   fileName;
        setTitile(_projectName);
        /// 工程打开后禁用所有动画
        disableAllAnimations();
        QMessageBox::information(this, C2Q("提示"), C2Q("打开工程文件成功!"), QMessageBox::Ok);
    }  
    else
    {
        QMessageBox::information(this, C2Q("提示"), C2Q("打开工程文件失败!"), QMessageBox::Ok);
    }
}
void    MainWindow::slotSaveProject()
{
    if (scene()->nodeTree().topLevelNodes().empty())
        return;
    if (_projectName.isEmpty())
    {
        auto    fileName    =   QFileDialog::getSaveFileName( this, C2Q("另存为.."), "D:/", C2Q("工程文件(*.fepj)"));
        if (fileName.isEmpty())
            return;
        _projectName    =   fileName;
    }
    String  fileName    =   _projectName.toLocal8Bit().data();
    if(!scene()->save(fileName.c_str()))
        QMessageBox::information(this, C2Q("提示"), C2Q("保存失败!"), QMessageBox::Ok);
    else
        setTitile(_projectName);
}

void    MainWindow::slotReset()
{
    if (scene())
    {
        scene()->clear();
        _projectName.clear();
        setTitile("");
        ui.animationTree->reset();
        ui.modelTree->reset();
    }
}
void    MainWindow::slotRedo()
{
    if  (_undoStack)
        _undoStack->redo();
}
void    MainWindow::slotUndo()
{
    if  (_undoStack)
        _undoStack->undo();
}

void    MainWindow::slotMoveEditor()
{
    assert(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem());
    
    if (!(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem()))
        return;

    auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeMoveEditor));
    if (editor == nullptr)
        return;
    Objects curObjs =   ui.modelTree->selected();
    if (editor->flags().hasFlag(FE::FLAG_VISIBLE))
    {
        editor->flags().removeFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeMoveEditor>()->setObjects({});
    }  
    else
    {
        editor->flags().addFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeMoveEditor>()->setObjects(curObjs);
    }
}
void    MainWindow::slotRotEditor()
{
    assert(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem());

    if (!(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem()))
        return;
    auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeRotateEditor));
    if (editor == nullptr)
        return;
    Objects curObjs =   ui.modelTree->selected();
    if (editor->flags().hasFlag(FE::FLAG_VISIBLE))
    {
        editor->flags().removeFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeRotateEditor>()->setObjects({});
    }  
    else
    {
        editor->flags().addFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeRotateEditor>()->setObjects(curObjs);
    }
}
void    MainWindow::slotScaleEditor()
{
    assert(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem());

    if (!(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem()))
        return;

    auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeScaleEditor));
    if (editor == nullptr)
        return;
    Objects curObjs =   ui.modelTree->selected();
    if (editor->flags().hasFlag(FE::FLAG_VISIBLE))
    {
        editor->flags().removeFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeScaleEditor>()->setObjects({});
    }  
    else
    {
        editor->flags().addFlag(FE::FLAG_VISIBLE);
        editor->as<FENodeScaleEditor>()->setObjects(curObjs);
    }
}


void    MainWindow::notifyEngineStart(FEScene& scene)
{
    assert(ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem());
    if ((ui.sceneViewer != nullptr && ui.sceneViewer->scene() && ui.sceneViewer->scene()->inputSystem()))
    {
        /// 模型树点选后通知，到编辑对象
        {
            auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeRotateEditor));
            ui.modelTree->_selectEvts   +=  {editor.get(),[editor](Object object,bool)
            {
                auto pEditor =   (FENodeRotateEditor*)(editor->as<FENodeRotateEditor>());
                if (pEditor->flags().hasFlag(FE::FLAG_VISIBLE))
                    pEditor->setObjects({object});
                else
                    pEditor->setObjects({}); 
                
            }};
        }
        {
            auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeMoveEditor));
            ui.modelTree->_selectEvts   +=  {editor.get(),[editor](Object object,bool)
            {
                auto pEditor =   (FENodeMoveEditor*)(editor->as<FENodeMoveEditor>());
                if (pEditor->flags().hasFlag(FE::FLAG_VISIBLE))
                    pEditor->setObjects({object});
                else
                    pEditor->setObjects({}); 
            }};
        }
        {
            auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeRotateEditor));
            ui.modelTree->_selectEvts   +=  {editor.get(),[editor](Object object,bool)
            {
                auto pEditor =   (FENodeRotateEditor*)(editor->as<FENodeRotateEditor>());
                if (pEditor->flags().hasFlag(FE::FLAG_VISIBLE))
                    pEditor->setObjects({object});
                else
                    pEditor->setObjects({});
            }};
        }
        /// 移动编辑器拖拽通知: 用于 undo/redo
        /// EditStart 时快照旧位置, EditEnd 时构造 MoveNodeCmd push 到 undoStack
        {
            auto    editor  =   ui.sceneViewer->scene()->inputSystem()->query(UUIDOF(FENodeMoveEditor));
            auto    moveEditor  =   editor ? editor->as<FENodeMoveEditor>() : nullptr;
            if  (moveEditor)
            {
                moveEditor->mDelegate() +=  {this,[this](FE::FEEditAxis::EditStatus status
                    , const real3&, const real3&, FEEditAxisMove&)
                {
                    if  (status == FE::FEEditAxis::EditStart)
                    {
                        _moveSnapshots.clear();
                        auto    ed  =   this->scene()->inputSystem()->query(UUIDOF(FENodeMoveEditor));
                        auto    me  =   ed ? ed->as<FENodeMoveEditor>() : nullptr;
                        if  (me)
                        {
                            for (auto obj : me->objects())
                            {
                                if  (!obj)
                                    continue;
                                MoveNodeCmd::ObjectMove  snap;
                                snap.obj    =   obj;
                                snap.oldPos =   MoveNodeCmd::getPosition(obj);
                                snap.newPos =   snap.oldPos;
                                _moveSnapshots.push_back(snap);
                            }
                        }
                    }
                    else if (status == FE::FEEditAxis::EditEnd)
                    {
                        if  (_moveSnapshots.empty())
                            return;
                        MoveNodeCmd::ObjectMoves   moves;
                        for (auto& snap : _moveSnapshots)
                        {
                            if  (!snap.obj)
                                continue;
                            real3   newPos  =   MoveNodeCmd::getPosition(snap.obj);
                            if  (newPos != snap.oldPos)
                            {
                                MoveNodeCmd::ObjectMove  m;
                                m.obj    =   snap.obj;
                                m.oldPos =   snap.oldPos;
                                m.newPos =   newPos;
                                moves.push_back(std::move(m));
                            }
                        }
                        if  (!moves.empty() && _undoStack)
                        {
                            _undoStack->beginMacro(u8"移动节点");
                            _undoStack->push(new MoveNodeCmd(std::move(moves)));
                            _undoStack->endMacro();
                        }
                        _moveSnapshots.clear();
                    }
                }};
            }
        }
    }
}

QIcon   MainWindow::objectIcon(ImageIndex type)
{
    auto    icon    =   ui.modelTree->icon();
    auto    h       =   icon.height();
    QPixmap pixmap  =   icon.copy(((int)type) * h, 0, h, h);
    return QIcon(pixmap);
}
QRect   MainWindow::objectIconRect(ImageIndex type)
{
    auto    icon    =   ui.modelTree->icon();
    auto    height  =   icon.height();
    return QRect(type * height, 0, height, height);
}

int     MainWindow::rowHeight() const
{
    return  ui.modelTree->rowHeight();
}

double  MainWindow::curTime() const
{
    return  ui.timeLineEditor->curTime();
}

bool    MainWindow::autoKeyframe() const
{
    return  ui.checkBox_autoKey->isChecked();
}

void    MainWindow::noticeUpdatePropsUi()
{
    if  (_propsUi)
        _propsUi->updateUi();
}

void    MainWindow::blockRealtimeUpdate(bool block)
{
    if  (_propsUi)
        _propsUi->blockRealtimeUpdate(block);
}

void    MainWindow::refreshAnimTree()
{
    ui.animationTree->updateUi();
}

void    MainWindow::refreshTimeline()
{
    ui.timeLineEditor->refreshKeyframes();
}

void    MainWindow::refreshModelTree()
{
    ui.modelTree->update();
}

/// 重挂父节点(模型树拖拽嵌入/跨父前/后/顶层): newParent 为空表示提升到顶层
bool    MainWindow::ctxReparentNode(FE::FENode* node, FE::FENode* newParent)
{
    if  (node == nullptr)
        return  false;
    /// 同父重挂无意义(包含 newParent==旧父 与 newParent==自身 两种情形)
    if  (newParent == node->parent().get() || newParent == node)
        return  false;
    /// 目标为自身后代会成环(拖拽 canDropOn 已保护,此处覆盖粘贴入口)
    for (auto p = newParent; p != nullptr; p = p->parent().get())
    {
        if  (p == node)
            return  false;
    }
    auto    refreshCb  =   [this]()
    {
        refreshModelTree();
        noticeUpdatePropsUi();
    };
    UNDO_STACK->beginMacro(u8"重挂父节点");
    UNDO_STACK->push(new FE::ReparentNodeCmd(node, newParent, refreshCb));
    UNDO_STACK->endMacro();
    return  true;
}

/// 同父兄弟排序(模型树拖拽同父前/后): 跨父已由 ctxReparentNode 处理,此处仅处理同父
void    MainWindow::ctxReorderNode(FE::FENode* node, FE::FENode* target, bool after)
{
    if  (node == nullptr || target == nullptr)
        return;
    auto    parent  =   node->parent();
    /// 顶层节点排序(双父均为空)暂不支持(FENodeTree 无 insertAt 接口)
    if  (!parent || target->parent().get() != parent.get())
        return;
    auto    refreshCb  =   [this]()
    {
        ui.modelTree->update();
    };
    UNDO_STACK->beginMacro(u8"调整节点顺序");
    UNDO_STACK->push(new FE::ReorderNodeCmd(node, target, after, refreshCb));
    UNDO_STACK->endMacro();
}

/// 复制节点到剪贴板(Ctrl+C): 仅记录源引用,克隆延迟到粘贴 redo 时进行
void    MainWindow::ctxCopyNode(FE::FENode* node)
{
    if  (node == nullptr)
        return;
    _clipNode    =   node;
    _clipCut     =   false;
    /// 复制覆盖剪贴板,原剪切状态失效
    ui.modelTree->setCutNode(nullptr);
    {
        std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
        if (f.is_open()) f << "[MainWindow] ctxCopyNode node=" << (void*)node << " _clipNode=" << (void*)_clipNode.get() << std::endl;
    }
}

/// 剪切节点到剪贴板(Ctrl+X): 仅标记状态,粘贴时才执行重挂(移动语义)
void    MainWindow::ctxCutNode(FE::FENode* node)
{
    if  (node == nullptr)
        return;
    _clipNode    =   node;
    _clipCut     =   true;
    /// 树中该节点行半透明绘制以区分剪切状态
    ui.modelTree->setCutNode(Object(node));
}

/// 粘贴(Ctrl+V): 剪切=重挂到当前选中下(移动),复制=克隆为源父兄弟(PasteNodeCmd)
void    MainWindow::ctxPasteNode(FE::FENode* node)
{
    /// node = _clipNode.get() = 源节点(由 signalPasteNode 直接传入)
    {
        std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
        if (f.is_open()) f << "[MainWindow] ctxPasteNode entry node=" << (void*)node << " _clipNode=" << (void*)_clipNode.get() << " _clipCut=" << _clipCut << std::endl;
    }
    if  (node == nullptr || _clipNode == nullptr)
    {
        std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
        if (f.is_open()) f << "[MainWindow] ctxPasteNode BAIL: node or _clipNode null" << std::endl;
        return;
    }
    if  (_clipCut)
    {
        /// 剪切 = 移动: 重挂到当前选中节点下(无选中 = 顶层)
        auto    sel              =   ui.modelTree->selected();
        auto    target           =   (!sel.empty() && sel[0]) ? sel[0]->cast<FENode>() : nullptr;
        /// 目标为自身/当前父(无意义)则不执行,保留剪贴板
        if  (target == node || target == node->parent().get())
            return;
        ctxReparentNode(node, target);
        /// 移动完成清空剪贴板与剪切绘制状态,展开目标父并选中节点
        _clipNode    =   nullptr;
        _clipCut     =   false;
        ui.modelTree->setCutNode(nullptr);
        if  (target)
            FE::setExpand(*target, true);
        ui.modelTree->selectNode(Object(node));
        noticeUpdatePropsUi();
        return;
    }
    /// 复制 = 克隆: 与剪切粘贴(移动原节点)是完全不同的路径;
    /// 克隆作为源节点的兄弟插入(新父 = 源节点父;源已删除离场则退化为顶层),
    /// 命令每次 redo 从源节点重新克隆(克隆 -> 挂树 -> 加入系统),undo 从系统移除并删除克隆
    FE::FENode*  parent      =   node->parent().get();
    auto         refreshCb   =   [this](FE::PasteNodeCmd& cmd, bool isRedo)
    {
        if  (isRedo)
        {
            refreshModelTree();
            /// 展开父节点并选中粘贴出的新节点(push 同步执行 redo,克隆已创建)
            if  (cmd.parent())
                FE::setExpand(*cmd.parent(), true);
            if  (cmd.node())
                ui.modelTree->selectNode(Object(cmd.node().get()));
            noticeUpdatePropsUi();
        }
        else
        {
            /// 被移除节点的引用已在命令 undo 入口精确剔除,此处只刷新视图
            noticeUpdatePropsUi();
            refreshModelTree();
        }
    };
    UNDO_STACK->beginMacro(u8"粘贴节点");
    UNDO_STACK->push(new FE::PasteNodeCmd(_clipNode, parent, refreshCb));
    UNDO_STACK->endMacro();
}

void    MainWindow::slotPropsCurTimeChanged(double)
{
    noticeUpdatePropsUi();
}

void    MainWindow::slotResetLayout()
{
    /// 恢复 Dock 到默认布局:模型树在左,属性在右,动画编辑/消息在底部
    addDockWidget(Qt::LeftDockWidgetArea, ui.dockWidget_modelTree);
    addDockWidget(Qt::RightDockWidgetArea, ui.dockWidget_properities);
    addDockWidget(Qt::BottomDockWidgetArea, ui.dockWidget_animationEditor);
    addDockWidget(Qt::BottomDockWidgetArea, ui.dockWidgetMessage);
    splitDockWidget(ui.dockWidget_animationEditor, ui.dockWidgetMessage, Qt::Horizontal);

    ui.dockWidget_modelTree->show();
    ui.dockWidget_properities->show();
    ui.dockWidget_animationEditor->show();
    ui.dockWidgetMessage->show();

    /// 工具栏归位
    addToolBar(Qt::TopToolBarArea, ui.toolBar);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(Qt::TopToolBarArea, ui.toolBar_2);
}

void    MainWindow::closeEvent(QCloseEvent*event)
{
    ui.modelTree->close();
    ui.sceneViewer->close();
}
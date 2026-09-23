
#include    "MainWindow.h"
#include    "FEFileFormatHelper.hpp"
#include    "fileFormat/fepk/FEFormatFepj.hpp"
#include    "graphic/FEScene.h"
#include    "FEInputSystem.hpp"
#include    "axis/FENodeMoveEditor.h"
#include    "axis/FENodeRotateEditor.h"
#include    "axis/FENodeScaleEditor.h"
#include    "animation/FEAnimationSys.hpp"

MainWindow* _mainApp   =   nullptr;

MainWindow::MainWindow()
{
    _mainApp    =   this;

    ui.setupUi(this);

    ui.action_savePrj->setShortcut(QKeySequence::Save);
    ui.action_undo->setShortcut(QKeySequence::Undo);
    ui.action_redo->setShortcut(QKeySequence::Redo);

    ui.timeLineEditor->linkScrollBar(ui.horizontalScrollBar);
    ui.splitter->setSizes({200, 800});
    ui.animationTree->linkToTickMgr(ui.timeLineEditor);
    
    ui.modelTree->setup(ui.sceneViewer->scene());
    ui.sceneViewer->notify()    +=  {this,&MainWindow::notifyEngineStart};

    /// 模型树右键菜单: 添加/移除到动画树
    connect(ui.modelTree, &QtTree::signalContextMenu, this
        , [this](const QPoint& pt, Object obj)
    {
        if (!obj)
            return;
        auto    node    =   obj->cast<FENode>();
        if (!node)
            return;
        QMenu   menu(ui.modelTree);
        bool    inTree  =   ui.animationTree->containsObject(obj);
        auto    action  =   menu.addAction(inTree ? u8"从动画树移除" : u8"添加到动画树");
        connect(action, &QAction::triggered, this, [this, obj]()
        {
            double  curTime =   ui.timeLineEditor->curTime();
            ui.animationTree->toggleObject(obj, curTime);
        });
        menu.exec(ui.modelTree->mapToGlobal(pt));
    });

    /// 将模型树链接到时间线编辑器,用于获取选中节点添加关键帧
    ui.timeLineEditor->setModelTree(ui.modelTree);

    _undoStack  =   new QUndoStack(this);
    ui.undoView->setStack(_undoStack);
    ui.timeLineEditor->setUndoStack(_undoStack);

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

void    MainWindow::closeEvent(QCloseEvent*event)
{
    ui.modelTree->close();
    ui.sceneViewer->close();
}
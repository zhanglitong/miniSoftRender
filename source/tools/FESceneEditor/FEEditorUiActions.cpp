/// <summary>
/// FEEditorUi 的菜单动作、工程读写与各面板绘制,
/// 从 FEEditorUi.cpp 拆分而来;版式常量与绘制工具见 FEEditorUiInternal.hpp。
/// </summary>

#include    "FEEditorUi.hpp"
#include    "FEEditorUiInternal.hpp"
#include    "FEUiStrings.hpp"
#include    "FEEditorUndoCmds.hpp"
#include    "FEPropertyView.hpp"
#include    "FESettingPropertyModel.hpp"

#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/FEImage.h"
#include    "../../../inc/FEMessage.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../core/FEImageHelper.hpp"
#include    "../../../inc/FEApp.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/node/FENode.hpp"
#include    "../../../inc/FEInputSystem.hpp"
#include    "../../../inc/axis/FENodeMoveEditor.h"
#include    "../../../inc/axis/FENodeRotateEditor.h"
#include    "../../../inc/axis/FENodeScaleEditor.h"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../../inc/geometry/FEGeometryPyramid.hpp"
#include    "../../../inc/geometry/FEGeometryTorus.hpp"
#include    "../../../inc/geometry/FEGeometryTorusKnot.hpp"
#include    "../../../inc/geometry/FEGeometryPRCP.hpp"
#include    "../../../inc/geometry/FEGeometryPRCN.hpp"
#include    "../../../inc/geometry/FEGeometryPRPY.hpp"
#include    "../../../inc/geometry/FEGeometryPRRT.hpp"
#include    "../../../inc/geometry/FEGeometryPRDH.hpp"

#include    <algorithm>
#include    <cctype>
#include    <chrono>
#include    <cmath>
#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <fstream>
#include    <imgui.h>
#include    <iostream>
#include    <map>
#include    <sstream>
#include    <string>
#include    <vector>

using   namespace   FE::ui;
using   namespace   FE::ui::internal;

namespace   FE
{
    void    FEEditorUi::drawFileDialog()
    {
        if (!_fileDialog.isOpen())
            return;
        std::string file;
        if (!_fileDialog.draw(_iconFont,file))
            return;
        if (file.empty())
            return;

        /// <summary>
        /// 另存为模式走保存,导入模式走导入模型,其余走打开。
        /// 三者都会改场景(open 会新建渲染工厂),必须推迟到渲染趟之外执行
        /// </summary>
        const   bool    saveMode    =   _fileDialogSaveMode;
        const   bool    importMode  =   _fileDialogImportMode;
        _fileDialogSaveMode     =   false;
        _fileDialogImportMode   =   false;
        if (saveMode)
            _pendingActions.push_back([this,file]() {   saveProjectTo(file);    });
        else if (importMode)
            _pendingActions.push_back([this,file]() {   importModel(file);  });
        else
            _pendingActions.push_back([this,file]() {   openProject(file);  });
    }

    void    FEEditorUi::onAction(const FEUiMenuItem* action)
    {
        if (action == nullptr)
            return;
        const   std::string&    name    =   action->_name;
        if (name == "action_openPrj")
        {
            /// <summary>
            /// 打开工程:与 Qt 版一致,默认从程序目录开始
            /// </summary>
            _fileDialogSaveMode =   false;
            _fileDialog.open(FEFileDialog::kOpen,"打开工程",{{"工程文件 (*.fepj)",".fepj"}},_startDir,"");
        }
        else if (name == "action_savePrj")
        {
            _pendingActions.push_back([this]() {    saveProject();  });
        }
        else if (name == "actionReset")
        {
            _pendingActions.push_back([this]() {    resetScene();   });
        }
        else if (name == "action_undo")
        {
            /// <summary>
            /// 撤销会移除场景节点(改渲染工厂),与创建一样必须推迟到 MSG_UPDATE
            /// </summary>
            _pendingActions.push_back([this]() {
                _undoStack.undo();
                /// <summary>
                /// 撤销可能增删节点上的动画等组件,组件变更不触发节点树事件,需手动刷新行列表
                /// </summary>
                _modelTree.refresh();
            });
        }
        else if (name == "action_redo")
        {
            _pendingActions.push_back([this]() {
                _undoStack.redo();
                /// <summary>
                /// 重做可能增删节点上的动画等组件,组件变更不触发节点树事件,需手动刷新行列表
                /// </summary>
                _modelTree.refresh();
            });
        }
        else if (name == "action_ShortcutSettings")
        {
            /// <summary>
            /// 打开快捷键设置窗口;不改场景,无需推迟到 MSG_UPDATE
            /// </summary>
            _shortcutOpen   =   true;
            _shortcutError.clear();
            if (_shortcutSelected < 0 && !_shortcuts._items.empty())
                _shortcutSelected   =   0;
        }
        else if (name == "action_SettingViewer")
        {
            /// <summary>
            /// 打开运行时配置窗口(通用 key/value 控件),不改场景
            /// </summary>
            _settingOpen    =   true;
        }
        else if (name == "action_importModel")
        {
            /// <summary>
            /// 导入模型:默认选 .gltf/.glb,类型下拉框可切到所有文件;
            /// 解析与入场景推迟到下一帧 MSG_UPDATE
            /// (与创建节点同理,避免渲染趟内新建渲染工厂)
            /// </summary>
            _fileDialogSaveMode     =   false;
            _fileDialogImportMode   =   true;
            _fileDialog.open(FEFileDialog::kOpen,"导入模型",
                             {{"3D 模型 (*.gltf;*.glb)",".gltf;.glb"},
                              {"所有文件 (*.*)",""}},
                             _startDir,"");
        }
        else if (name == "action_TimeLine")
        {
            /// <summary>
            /// 时间线面板开关;只改 UI 状态,不改场景
            /// </summary>
            _timeLine.toggle();
        }
        else if (name == "action_undoView")
        {
            /// <summary>
            /// 撤销历史窗口开关;只改 UI 状态,不改场景
            /// </summary>
            _undoView.toggle();
        }
        else if (name == "action_axisEditor")
        {
            /// <summary>
            /// 坐标轴编辑窗口开关;只改 UI 状态,不改场景
            /// </summary>
            _axisOpen   =   !_axisOpen;
        }
        else if (name == "action_about")
        {
            /// <summary>
            /// 关于窗口:非模态,点击菜单后重新显示
            /// </summary>
            _aboutOpen  =   true;
        }
        else if (name == "action_move" || name == "action_rotation" || name == "action_scale")
        {
            /// <summary>
            /// 三维变换抓手开关(参考 Qt 版 slotMoveEditor/slotRotEditor/slotScaleEditor):
            /// 三个动作各自独立切换,不互斥;只改抓手的可见标志与编辑对象,
            /// 不修改场景数据,因此不产生撤销记录
            /// </summary>
            toggleTransformTool(name);
        }
        else if (name.rfind("action_add",0) == 0)
        {
            /// <summary>
            /// "模型"分组下的几何体/空节点创建按钮(action_addBox … action_addEmpty);
            /// 相机节点名为 action_AddCameraNode,不会被此前缀匹配。
            /// 这里只登记动作,真正的创建推迟到下一帧 MSG_UPDATE(见 onMessage),
            /// 因为本函数由 Ribbon 绘制调用,而绘制发生在渲染趟内部:此刻新建渲染工厂
            /// 会赶在同一帧被渲染,而工厂的 update 尚未执行、索引缓冲尚未生成
            /// </summary>
            _pendingActions.push_back([this,name]() {   createModelNode(name);  });
        }
    }

    void    FEEditorUi::openProject(const std::string& path)
    {
        if (_scene == nullptr)
            return;
        if (_scene->open(toNativePath(path).c_str()))
        {
            /// <summary>
            /// 场景已被整体替换,旧命令引用的节点不再属于当前工程
            /// </summary>
            _undoStack.clear();
            _timeLine.reset();
            _projectName    =   path;
            _startDir       =   std::filesystem::u8path(path).parent_path().u8string();
            _statusText     =   "打开工程成功";
        }
        else
        {
            _statusText     =   "打开工程失败:" + path;
        }
    }

    void    FEEditorUi::saveProject()
    {
        if (_scene == nullptr)
            return;
        /// <summary>
        /// 与 Qt 版一致:节点树为空时不做任何处理
        /// </summary>
        if (_scene->nodeTree().topLevelNodes().empty())
        {
            _statusText =   "场景为空,没有可保存的内容";
            return;
        }
        if (_projectName.empty())
        {
            _fileDialogSaveMode =   true;
            _fileDialog.open(FEFileDialog::kSave,"另存为",{{"工程文件 (*.fepj)",".fepj"}},_startDir,"unname");
            return;
        }
        saveProjectTo(_projectName);
    }

    void    FEEditorUi::saveProjectTo(const std::string& path)
    {
        if (_scene == nullptr)
            return;
        if (_scene->save(toNativePath(path).c_str()))
        {
            _projectName    =   path;
            _startDir       =   std::filesystem::u8path(path).parent_path().u8string();
            _statusText     =   "保存工程成功";
        }
        else
        {
            _statusText     =   "保存工程失败:" + path;
        }
    }

    void    FEEditorUi::resetScene()
    {
        if (_scene == nullptr)
            return;
        /// <summary>
        /// 与 Qt 版 slotReset 一致:清空场景与工程名,不退出程序。
        /// 模型树订阅了节点树的清空事件,下一帧自动刷新
        /// </summary>
        _scene->clear();
        _undoStack.clear();
        _timeLine.reset();
        _projectName.clear();
        _statusText =   "已重置";
    }

    namespace
    {
        /// <summary>
        /// 编辑器创建几何体时统一使用的属性流(位置/法线/UV),
        /// 与脚本绑定及 MCP 工具的默认流一致
        /// </summary>
        Attrs   editorGeoAttrs()
        {
            return  Attrs({{IS_VERTEX_POS,         FMT_R32G32B32_FLOAT},
                           {IS_VERTEX_NOR,         FMT_R32G32B32_FLOAT},
                           {IS_VERTEX_TEXCOORD0,   FMT_R32G32_FLOAT}});
        }

        /// <summary>
        /// 生成不与现有顶层节点重名的新名字:重名时追加 " 2"、" 3" 依次递增
        /// </summary>
        /// <param name="scene">场景(取其顶层节点名)</param>
        /// <param name="base">期望的基础名字</param>
        /// <returns>可用的节点名</returns>
        std::string editorUniqueName(FEScene& scene,const std::string& base)
        {
            std::string name    =   base;
            for (int index = 2; ; ++index)
            {
                bool    used    =   false;
                for (auto&  n   :   scene.nodeTree().topLevelNodes())
                {
                    if (n != nullptr && name == n->name())
                    {
                        used    =   true;
                        break;
                    }
                }
                if (!used)
                    return  name;
                name    =   base + " " + std::to_string(index);
            }
        }
    }

    void    FEEditorUi::createModelNode(const std::string& actionName)
    {
        if (_scene == nullptr || _ctx == nullptr)
            return;

        /// <summary>
        /// 统一入口:节点名先唯一化(命令重做时沿用它,避免因场景已有同名节点而改名),
        /// 再压入撤销栈;push() 会立即执行一次 redo(),把节点加入节点树并分发到
        /// 渲染工厂与各系统(模型树显示与真正进入渲染)。
        /// 本函数运行在 MSG_UPDATE(见 onAction),此时新建工厂能在同一帧的 update
        /// 中生成 VBO/IBO,不会出现空索引缓冲
        /// </summary>
        auto    addWithUndo =   [this](Node node,const std::string& name)
        {
            if (node == nullptr)
                return;
            node->setName(editorUniqueName(*_scene,name));
            _undoStack.push(new FEEditorCreateNodeCmd(*_scene,{node},"创建" + name));
        };

        /// <summary>
        /// 按动作名生成网格;每种几何体用默认参数即可获得合理的初始形状
        /// </summary>
        Mesh        mesh;
        std::string label;
        if (actionName == "action_addBox")
        {
            FEGeometryBox   geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "正方体";
        }
        else if (actionName == "action_addSphere")
        {
            FEGeometrySphere    geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "球体";
        }
        else if (actionName == "action_addCylinder")
        {
            FEGeometryCylinder  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆柱";
        }
        else if (actionName == "action_addSphericalCap")
        {
            FEGeometryPRCP  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "球冠体";
        }
        else if (actionName == "action_addConeFrustum")
        {
            FEGeometryPRCN  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆锥台";
        }
        else if (actionName == "action_addPyramidFrustum")
        {
            FEGeometryPRPY  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "棱锥台";
        }
        else if (actionName == "action_addTorusSector")
        {
            /// <summary>
            /// 圆环扇形:取四分之一圆的弧段
            /// </summary>
            FEGeometryTorus geo(*_ctx);
            geo.param().arc    =   float(TWO_PI) * 0.25f;
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环扇形";
        }
        else if (actionName == "action_addTorus")
        {
            FEGeometryTorus geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环";
        }
        else if (actionName == "action_addHyperboloid")
        {
            /// <summary>
            /// 引擎暂无独立的双曲面实现,用椭球体(PRDH)代替
            /// </summary>
            FEGeometryPRDH  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "双曲面";
        }
        else if (actionName == "action_addPyramid")
        {
            /// <summary>
            /// 四棱锥:FEGeometryPyramid 默认即为 radiusTop=0、radialSegments=4
            /// </summary>
            FEGeometryPyramid   geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "四棱锥";
        }
        else if (actionName == "action_addTorusSolid")
        {
            FEGeometryPRRT  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环体";
        }
        else if (actionName == "action_addTorusKnot")
        {
            FEGeometryTorusKnot geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环扭结";
        }
        else if (actionName == "action_addEmpty")
        {
            /// <summary>
            /// 空节点:只有名字,没有网格与材质
            /// </summary>
            Node    empty   =   new FENode(*_ctx);
            addWithUndo(empty,"空节点");
            _statusText =   "已创建空节点";
            return;
        }
        else
        {
            return;
        }

        if (mesh == nullptr)
        {
            _statusText =   "几何体生成失败:" + label;
            return;
        }

        Node    node    =   new FENode(*_ctx);
        node->setMesh(mesh);

        /// <summary>
        /// 普通 PBR 材质:明亮的漫反射与自发光,使新对象在默认光照下清晰可见
        /// </summary>
        FEMaterialPBR*  mat =   new FEMaterialPBR(*_ctx);
        mat->data()._value._diffuse     =   float4(0.35f,0.55f,0.85f,1.0f);
        mat->data()._value._emissive    =   float4(0.08f,0.16f,0.28f,1.0f);
        mat->data().update();
        node->setMaterial(mat);

        addWithUndo(node,label);
        _statusText =   "已创建" + label;
    }

    void    FEEditorUi::importModel(const std::string& path)
    {
        if (_scene == nullptr || _ctx == nullptr || path.empty())
            return;

        const   std::filesystem::path   fsPath  =   std::filesystem::u8path(path);
        std::string ext =   fsPath.extension().u8string();
        std::transform(ext.begin(),ext.end(),ext.begin(),
                       [](unsigned char c) {   return  char(std::tolower(c));   });

        /// <summary>
        /// 格式必须与 FEContext 注册读取器时使用的完全一致(见 FEContext.cpp 的
        /// DEFINE_FORMAT_READER(FEFormatGLTFReader)),否则 queryReader 查不到
        /// </summary>
        FEFileFormat    fmt(".gltf","1.0.0.0","GLTF text Format!");
        if (ext == ".glb")
            fmt =   FEFileFormat(".glb","1.0.0.0","GLTF binary Format!");
        else if (ext != ".gltf")
        {
            _statusText =   "不支持导入该格式:" + fsPath.filename().u8string();
            return;
        }

        auto    reader  =   FEFileFormatHelper::queryReader(*_ctx,fmt);
        if (reader == nullptr)
        {
            _statusText =   "导入失败:没有可用的模型读取器";
            return;
        }

        /// <summary>
        /// 解析与入场景都在 MSG_UPDATE 内执行(由 _pendingActions 推迟):
        /// 此时新建的渲染工厂能在同一帧 update 里生成顶点/索引缓冲
        /// </summary>
        const   Objects objects =   reader->readFiles({toNativePath(path)});
        Nodes   nodes;
        for (auto object : objects)
        {
            Node    node    =   object->cast<FENode>();
            if (node != nullptr)
                nodes.push_back(node);
        }
        if (nodes.empty())
        {
            _statusText =   "导入失败,未解析到节点:" + fsPath.filename().u8string();
            return;
        }

        /// <summary>
        /// 顶层节点名唯一化(重做时沿用该名字,避免与场景已有节点重名);
        /// 文件内节点无名字时用文件名兜底
        /// </summary>
        const   std::string stem    =   fsPath.stem().u8string();
        for (Node& node : nodes)
        {
            const   std::string current =   node->name() != nullptr ? node->name() : "";
            const   std::string base    =   current.empty() ? stem : current;
            node->setName(editorUniqueName(*_scene,base));
        }

        _undoStack.push(new FEEditorCreateNodeCmd(*_scene,nodes,"导入" + stem));
        _statusText =   "已导入 " + std::to_string(nodes.size()) + " 个节点:"
                    +   fsPath.filename().u8string();
    }

    void    FEEditorUi::drawModelTree()
    {
        if (!_treeOpen)
            return;
        ImGui::SetNextWindowPos(ImVec2(kModelTreeX,kRibbonHeight + kModelTreeYOff),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kModelTreeW,kModelTreeH),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("模型树",&_treeOpen))
        {
            /// <summary>
            /// 模型树的数据来源是场景的 FENodeTree
            /// </summary>
            _modelTree.draw(_scene ? &_scene->nodeTree() : nullptr,_iconFont);
        }
        ImGui::End();
    }

    void    FEEditorUi::drawTimeLine()
    {
        _timeLine.draw(_iconFont);
    }

    void    FEEditorUi::drawProperty()
    {
        if (!_propertyOpen)
            return;
        /// <summary>
        /// 首次打开落在模型树右侧,避免与模型树重叠
        /// </summary>
        ImGui::SetNextWindowPos(ImVec2(kPropertyX,kRibbonHeight + kPropertyYOff),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kPropertyW,kPropertyH),ImGuiCond_FirstUseEver);
        /// <summary>
        /// 数据来源:模型树当前选中节点;未选中时面板显示提示文字。
        /// 约束目标拾取期间改为固定显示约束所有者对象(拾取点击只更新候选目标)
        /// </summary>
        FENode* node    =   nullptr;
        Object  selected    =   _propertyPanel.pickingOwner();
        if (!selected)
            selected        =   _modelTree.selected();
        if (selected)
            node            =   selected->cast<FENode>();
        /// <summary>
        /// 标题栏显示当前节点/对象名称;"###属性"保证窗口 ID 不随名称变化,
        /// 改名或切换选中时窗口位置/停靠状态保持不变
        /// </summary>
        String  title   =   "属性";
        if (selected)
        {
            const   char*   name    =   (node != nullptr && node->name() != nullptr)
                                      ? node->name() : selected->className();
            title  +=  " - ";
            title  +=  (name != nullptr && name[0] != '\0') ? name : "(未命名)";
        }
        title  +=  "###属性";
        if (ImGui::Begin(title.c_str(),&_propertyOpen))
        {
            /// <summary>
            /// 时间线提供当前时间的关键帧状态(check)与添加/移除关键帧。
            /// 旋转值由节点自己的累计欧拉角账本提供,面板不再需要额外参数
            /// </summary>
            _propertyPanel.draw(node,&_timeLine,&_modelTree.selection());
        }
        ImGui::End();
        /// <summary>
        /// 拾取结束后把模型树选中恢复回约束所有者节点,避免属性面板跳到被拾取对象
        /// </summary>
        const   Object  restore =   _propertyPanel.takeRestoreSelection();
        if (restore)
            _modelTree.select(restore);
    }

    void    FEEditorUi::drawAxisSetting()
    {
        if (!_axisOpen)
            return;
        /// <summary>
        /// 轴编辑窗口:配置编辑轴的轴位置/编辑类型/坐标系/中心/自定义轴心轴向/禁用轴
        /// </summary>
        drawAxisSettingWindow(_axisSetting,_axisOpen,_modelTree.selection());
    }

    void    FEEditorUi::drawUiPreview()
    {
        if (!_uiPreviewOpen)
            return;
        /// <summary>
        /// 首次打开落在功能区下方,避免遮挡标签页
        /// </summary>
        ImGui::SetNextWindowPos(ImVec2(kUiPreviewX,kRibbonHeight + kUiPreviewYOff),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kUiPreviewW,kUiPreviewH),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("UI 预览 - MainWindow.ui",&_uiPreviewOpen))
        {
            if (_uiDoc.root() == nullptr)
            {
                ImGui::TextDisabled("解析 .ui 失败:%s",_uiError.c_str());
                ImGui::TextDisabled("请确认 bin/data/ui/MainWindow.ui 存在");
            }
            else
            {
                _uiRenderer.draw(_uiDoc);
            }
        }
        ImGui::End();
    }

    void    FEEditorUi::toggleGrid()
    {
        _showGrid   =   !_showGrid;
        if (_scene != nullptr)
            _scene->showGrid(_showGrid);
    }

    void    FEEditorUi::resetCamera()
    {
        if (_scene == nullptr)
            return;
        Camera  cam =   _scene->camera();
        if (cam == nullptr)
            return;
        cam->setEye(real3(0.0f,-2.5f,0.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,0.0f,1.0f));
        cam->update();
    }

    void    FEEditorUi::toggleTransformTool(const std::string& actionName)
    {
        if (_scene == nullptr)
            return;
        FEInputSystem*  inputSys    =   _scene->inputSystem();
        if (inputSys == nullptr)
            return;

        /// <summary>
        /// 与 Qt 版一致:已显示则隐藏并清空编辑对象,否则显示并把模型树当前选区
        /// 交给抓手。抓手是常驻的渲染/输入工厂,切换只改可见标志,不产生撤销记录
        /// </summary>
        const   Objects&    selection   =   _modelTree.selection();
        if (actionName == "action_move")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeMoveEditor));
            if (com == nullptr)
                return;
            FENodeMoveEditor*   editor  =   com->as<FENodeMoveEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else if (actionName == "action_rotation")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeRotateEditor));
            if (com == nullptr)
                return;
            FENodeRotateEditor* editor  =   com->as<FENodeRotateEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else if (actionName == "action_scale")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeScaleEditor));
            if (com == nullptr)
                return;
            FENodeScaleEditor*  editor  =   com->as<FENodeScaleEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else
            return;
        /// <summary>
        /// 本次已把选区交给抓手,记录后避免同一选区再触发一次 setObjects
        /// </summary>
        _toolSelection  =   selection;
    }

    void    FEEditorUi::syncTransformTools()
    {
        if (_scene == nullptr)
            return;
        FEInputSystem*  inputSys    =   _scene->inputSystem();
        if (inputSys == nullptr)
            return;

        Component       move    =   inputSys->query(UUIDOF(FENodeMoveEditor));
        Component       rotate  =   inputSys->query(UUIDOF(FENodeRotateEditor));
        Component       scale   =   inputSys->query(UUIDOF(FENodeScaleEditor));
        const   bool    moveOn  =   move    != nullptr && move->flags().hasFlag(FLAG_VISIBLE);
        const   bool    rotateOn=   rotate  != nullptr && rotate->flags().hasFlag(FLAG_VISIBLE);
        const   bool    scaleOn =   scale   != nullptr && scale->flags().hasFlag(FLAG_VISIBLE);
        /// <summary>
        /// 轴配置每帧推送给旋转抓手(轴编辑窗口可能刚被修改)
        /// </summary>
        if (rotateOn)
            rotate->as<FENodeRotateEditor>()->setAxisSetting(&_axisSetting);

        /// <summary>
        /// 把抓手的实际可见状态回写到动作的勾选标志,
        /// 这样工程被重新打开(抓手回到默认隐藏)后 Ribbon 不会残留高亮
        /// </summary>
        FEUiMenuItem*   bar =   _uiDoc.menuBar();
        if (bar != nullptr)
        {
            FEUiMenuItem*   item    =   findActionItem(bar,"action_move");
            if (item != nullptr)
                item->_checked  =   moveOn;
            item    =   findActionItem(bar,"action_rotation");
            if (item != nullptr)
                item->_checked  =   rotateOn;
            item    =   findActionItem(bar,"action_scale");
            if (item != nullptr)
                item->_checked  =   scaleOn;
        }

        /// <summary>
        /// 播放中禁止编辑:清空抓手的编辑对象,暂停后选区变化会自动重新推给抓手
        /// </summary>
        if  (_timeLine.isPlaying())
        {
            if  (!_toolSelection.empty())
            {
                if (moveOn)
                    move->as<FENodeMoveEditor>()->setObjects({});
                if (rotateOn)
                    rotate->as<FENodeRotateEditor>()->setObjects({});
                if (scaleOn)
                    scale->as<FENodeScaleEditor>()->setObjects({});
                _toolSelection.clear();
            }
            return;
        }
        /// <summary>
        /// 模型树选区变化后,把新选区推给已显示的抓手(与 Qt 版选中事件一致)
        /// </summary>
        const   Objects&    selection   =   _modelTree.selection();
        if (sameObjects(selection,_toolSelection))
            return;
        _toolSelection  =   selection;
        if (moveOn)
            move->as<FENodeMoveEditor>()->setObjects(selection);
        if (rotateOn)
            rotate->as<FENodeRotateEditor>()->setObjects(selection);
        if (scaleOn)
            scale->as<FENodeScaleEditor>()->setObjects(selection);
    }

    bool    FEEditorUi::onMessage(const FEMessage& msg)
    {
        if (!_attached || !_hasContext)
            return  false;

        ImGuiIO&        io      =   ImGui::GetIO();
        const   auto    msgId   =   msg.msgId();
        /// <summary>
        /// 按下时刻的捕获状态决定整段拖拽归 GUI 还是归场景
        /// </summary>
        const   bool    captured=   io.WantCaptureMouse;
        bool            block   =   false;
        switch (msgId)
        {
        case MSG_MOUSE_MOVE:
            {
                const auto& m   =   static_cast<const MsgMouseMove&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
            }
            break;
        case MSG_LBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgLButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[0] =   true;
                _guiDrag[0]     =   captured;
            }
            break;
        case MSG_LBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgLButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[0] =   false;
            }
            break;
        case MSG_RBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgRButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[1] =   true;
                _guiDrag[1]     =   captured;
            }
            break;
        case MSG_RBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgRButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[1] =   false;
            }
            break;
        case MSG_MBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgMButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[2] =   true;
                _guiDrag[2]     =   captured;
            }
            break;
        case MSG_MBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgMButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[2] =   false;
            }
            break;
        case MSG_MOUSE_WHEEL:
            {
                const auto& w   =   static_cast<const MsgMouseWheel&>(msg)._info;
                io.MousePos     =   ImVec2(float(w._mouse.x),float(w._mouse.y));
                io.MouseWheel   =   float(w._zDelta) / 120.0f;
            }
            break;
        case MSG_KEYDOWN:
            {
                const auto& k   =   static_cast<const MsgKeyDown&>(msg)._info;
                if (k._key < 512)
                    io.KeysDown[k._key] =   true;
                syncModifierKeys(io);
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_KEYUP:
            {
                const auto& k   =   static_cast<const MsgKeyUp&>(msg)._info;
                if (k._key < 512)
                    io.KeysDown[k._key] =   false;
                syncModifierKeys(io);
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_CHAR:
            {
                const auto& c   =   static_cast<const MsgChar&>(msg)._info;
                /// <summary>
                /// 退格/回车/制表等控制字符由按键状态处理,不能当可见字符插入
                /// </summary>
                if (c._char >= 32)
                    io.AddInputCharacter(ImWchar(c._char));
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_UPDATE:
            /// <summary>
            /// 本消息先于场景到达(宿主 main 的 notify 先调 ui.onMessage 再转发场景),
            /// 而场景在此消息中才执行 onFrameStart/onFrameUpdate:
            /// 在此刻执行排队的场景操作,新建的渲染工厂能在同一帧的 update 中生成
            /// VBO/IBO,避免其赶在 update 之前被渲染而导致空索引缓冲崩溃。
            /// 先整体取出再执行,避免执行过程中新压入的动作在同一次循环里被重复处理
            /// </summary>
            if (!_pendingActions.empty())
            {
                std::vector<std::function<void()>>  actions;
                actions.swap(_pendingActions);
                for (auto& fn : actions)
                    fn();
            }
            /// 脚本自动化:每条命令占一帧,排在排队的场景操作之后执行
            runScript();
            break;
        default:
            break;
        }
        /// <summary>
        /// 在 GUI 上按下的拖拽整段归 GUI(直到对应按键抬起);
        /// 在场景里按下的拖拽即使经过功能区,抬起事件仍交回场景,避免按键状态卡死
        /// </summary>
        switch (msgId)
        {
        case MSG_LBUTTON_DOWN:
            block   =   _guiDrag[0];
            break;
        case MSG_LBUTTON_UP:
            block   =   _guiDrag[0];
            _guiDrag[0]  =   false;
            break;
        case MSG_RBUTTON_DOWN:
            block   =   _guiDrag[1];
            break;
        case MSG_RBUTTON_UP:
            block   =   _guiDrag[1];
            _guiDrag[1]  =   false;
            break;
        case MSG_MBUTTON_DOWN:
            block   =   _guiDrag[2];
            break;
        case MSG_MBUTTON_UP:
            block   =   _guiDrag[2];
            _guiDrag[2]  =   false;
            break;
        case MSG_MOUSE_MOVE:
        case MSG_MOUSE_WHEEL:
            block   =   captured || _guiDrag[0] || _guiDrag[1] || _guiDrag[2];
            break;
        default:
            break;
        }
        return  block;
    }
}

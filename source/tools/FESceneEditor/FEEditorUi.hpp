#pragma     once

#include    "../../../inc/graphic/FEFactoryRender.hpp"
#include    "../../../inc/graphic/FEImGuiBackend.h"
#include    "../../../inc/FEUndoStack.hpp"

#include    "FEUiDocument.hpp"
#include    "FEUiRenderer.hpp"
#include    "FEFileDialog.hpp"
#include    "FEModelTree.hpp"
#include    "FETimeLine.hpp"
#include    "FEUndoView.hpp"
#include    "FEShortcutConfig.hpp"

#include    <filesystem>
#include    <functional>
#include    <map>
#include    <string>
#include    <vector>

struct  ImFont;

namespace   FE
{
    class   FEScene;
    class   FEMessage;

    /// <summary>
    /// 编辑器 GUI 工厂:把 ImGui 的一帧画到屏幕上。
    /// 绘制属性设为 USAGE_Overlay —— 场景工厂之后的最后一层,保证菜单不被几何覆盖。
    /// </summary>
    class   FEEditorImGuiFactory:public FEFactoryRender
    {
    public:
        /// <summary>
        /// 构造工厂
        /// </summary>
        /// <param name="ctx">引擎上下文</param>
        /// <param name="backend">ImGui 渲染后端</param>
        FEEditorImGuiFactory(FEContext& ctx,FEImGuiBackend* backend);

        /// <summary>
        /// 每帧构建 UI 的回调(NewFrame 之后、Render 之前调用)
        /// </summary>
        std::function<void()>           _buildUI;
        /// <summary>
        /// 后端即将销毁时断开工链接,避免工厂持有悬空指针
        /// </summary>
        void    invalidateBackend()     {   _backend    =   nullptr;    }
    public:
        virtual void    update(CMDPtr cmd)  override;
        virtual void    render(CMDPtr cmd)  override;
    protected:
        /// <summary>
        /// 把 ImGui 的绘制数据扁平化成与后端无关的形式
        /// </summary>
        void    extractDrawData();
    protected:
        FEImGuiBackend*             _backend    =   nullptr;
        FEImGuiDrawData             _drawData;
        std::vector<char>           _vtx;
        std::vector<char>           _idx;
        std::vector<FEImGuiDrawCmd> _cmds;
    };

    /// <summary>
    /// FESceneEditor 的主界面:ImGui 上下文/渲染后端/字体图集/Overlay 工厂的挂接,
    /// 以及 Ribbon 功能区(文件/开始/视图/帮助/界面)的绘制、鼠标消息转发
    /// 与 .ui 文件预览。
    /// </summary>
    class   FEEditorUi
    {
    public:
        /// <summary>
        /// 菜单"退出"的回调,由宿主注入平台相关实现(如发送 WM_CLOSE)
        /// </summary>
        std::function<void()>       _onQuit;
    public:
        /// <summary>
        /// 挂接主界面。返回 false 时不会创建任何 ImGui 资源
        /// (后端不支持 ImGui,如 WebGPU 或离屏渲染)。
        /// </summary>
        /// <param name="ctx">引擎上下文</param>
        /// <param name="scene">场景</param>
        /// <returns>true 挂接成功</returns>
        bool    attach(FEContext& ctx,FEScene* scene);
        /// <summary>
        /// 释放 ImGui 后端与上下文。必须在场景(device)销毁之前调用。
        /// </summary>
        void    detach();
        bool    attached()  const   {   return  _attached;  }
        /// <summary>
        /// 把窗口消息(鼠标)喂给 ImGui。返回 true 表示 GUI 捕获了鼠标,
        /// 宿主应跳过场景处理,避免点菜单时同时旋转相机。
        /// </summary>
        /// <param name="msg">窗口消息</param>
        /// <returns>true 表示 GUI 已消费该消息</returns>
        bool    onMessage(const FEMessage& msg);
    protected:
        /// <summary>
        /// 绘制 .ui 解析出的 Ribbon 功能区与各浮动窗口
        /// </summary>
        void    drawRibbon();
        /// <summary>
        /// 处理快捷键(Ctrl+Z / Ctrl+Y 以及用户在快捷键设置页自定义的绑定)。
        /// ImGui 1.65 没有 ImGui::Shortcut,这里用按键原语自行分发
        /// </summary>
        void    processShortcuts();
        /// <summary>
        /// 快捷键设置窗口(表格 + 录制 + 恢复默认/确定/取消)
        /// </summary>
        void    drawShortcutSettings();
        /// <summary>
        /// 运行时配置窗口(通用 key/value 控件显示 FESetting)
        /// </summary>
        void    drawSettingViewer();
        /// <summary>
        /// 录制态下扫描按键,Enter 提交 / Esc 取消
        /// </summary>
        void    captureShortcutKeys();
        /// <summary>
        /// 丢弃未保存的改键(重新读取 XML)
        /// </summary>
        void    discardShortcutChanges();
        /// <summary>
        /// 重新构建快捷键表并读取 exe 同名 XML
        /// </summary>
        void    reloadShortcuts();
        /// <summary>
        /// 按动作名触发 Ribbon 命令(与点击按钮同一条路径)
        /// </summary>
        /// <param name="name">.ui 中的 QAction 名</param>
        void    triggerAction(const std::string& name);
        /// <summary>
        /// 文件选择对话框(打开工程 / 保存工程)
        /// </summary>
        void    drawFileDialog();
        /// <summary>
        /// 分发 Ribbon 按钮动作(按 .ui 中的动作名匹配)
        /// </summary>
        /// <param name="action">被点击的动作</param>
        void    onAction(const FEUiMenuItem* action);
        /// <summary>
        /// 打开工程:目标路径来自文件对话框
        /// </summary>
        /// <param name="path">工程文件路径(UTF-8)</param>
        void    openProject(const std::string& path);
        /// <summary>
        /// 保存工程;_projectName 为空时先弹另存为对话框
        /// </summary>
        void    saveProject();
        /// <summary>
        /// 把工程保存到指定路径并更新工程名
        /// </summary>
        /// <param name="path">工程文件路径(UTF-8)</param>
        void    saveProjectTo(const std::string& path);
        /// <summary>
        /// 重置:清空场景与工程名(与 Qt 版 slotReset 一致,不退出程序)
        /// </summary>
        void    resetScene();
        /// <summary>
        /// 按动作名创建"模型"分组下的几何体节点(或空节点);
        /// 未匹配到的动作名不产生任何对象
        /// </summary>
        /// <param name="actionName">.ui 中的动作名,例如 action_addBox</param>
        void    createModelNode(const std::string& actionName);
        /// <summary>
        /// 导入模型:用引擎的模型读取器(.gltf/.glb)解析文件,把解析出的节点
        /// 整批加入场景并记为一条撤销记录
        /// </summary>
        /// <param name="path">模型文件路径(UTF-8)</param>
        void    importModel(const std::string& path);
        /// <summary>
        /// .ui 文件预览窗口(用 ImGui 渲染解析出的控件树)
        /// </summary>
        void    drawUiPreview();
        /// <summary>
        /// 模型树窗口(参考 QtTree 的 ImGui 实现)
        /// </summary>
        void    drawModelTree();
        /// <summary>
        /// 时间线编辑面板(参考 Qt 版 UiTickMgr)
        /// </summary>
        void    drawTimeLine();
        /// <summary>
        /// 切换场景网格显隐
        /// </summary>
        void    toggleGrid();
        /// <summary>
        /// 相机复位到默认视角
        /// </summary>
        void    resetCamera();
        /// <summary>
        /// 移动/旋转/缩放手柄的开关:显示时把模型树选区推给抓手,
        /// 隐藏时清空其编辑对象(与 Qt 版 slotMoveEditor/slotRotEditor/slotScaleEditor 一致)
        /// </summary>
        /// <param name="actionName">action_move / action_rotation / action_scale</param>
        void    toggleTransformTool(const std::string& actionName);
        /// <summary>
        /// 每帧刷新三个抓手:未显示的保持空选区,已显示的跟随模型树当前选区;
        /// 同时把抓手可见状态回写到动作的勾选标志
        /// </summary>
        void    syncTransformTools();
    protected:
        FEEditorImGuiFactory*       _factory    =   nullptr;
        FEImGuiBackend*             _backend    =   nullptr;
        /// <summary>
        /// Font Awesome 图标字体,随 ImGui 上下文一起销毁
        /// </summary>
        ImFont*                     _iconFont   =   nullptr;
        FEScene*                    _scene      =   nullptr;
        /// <summary>
        /// 引擎上下文(创建几何体/节点/材质时需要)
        /// </summary>
        FEContext*                  _ctx        =   nullptr;
        bool                        _hasContext =   false;
        bool                        _attached   =   false;
        bool                        _showGrid   =   true;
        /// <summary>
        /// 天空盒开关(与设置树 render/showSkyBox 同步)
        /// </summary>
        bool                        _skyShown   =   true;
        bool                        _aboutOpen  =   false;
        /// <summary>
        /// .ui 预览:文档
        /// </summary>
        FEUiDocument                _uiDoc;
        /// <summary>
        /// .ui 预览:渲染器
        /// </summary>
        FEUiRenderer                _uiRenderer;
        /// <summary>
        /// .ui 预览:错误信息
        /// </summary>
        std::string                 _uiError;
        /// <summary>
        /// .ui 预览:窗口开关
        /// </summary>
        bool                        _uiPreviewOpen  =   false;
        /// <summary>
        /// 模型树
        /// </summary>
        FEModelTree                 _modelTree;
        /// <summary>
        /// 上一次推给变换抓手的选区;用于避免选区未变时每帧重复 setObjects
        /// </summary>
        Objects                     _toolSelection;
        /// <summary>
        /// 时间线编辑面板
        /// </summary>
        FETimeLine                  _timeLine;
        /// <summary>
        /// 模型树窗口开关
        /// </summary>
        bool                        _treeOpen       =   true;
        /// <summary>
        /// 撤销历史窗口(参考 Qt 的 QUndoView)
        /// </summary>
        FEUndoView                  _undoView;
        /// <summary>
        /// 当前 Ribbon 标签页(主菜单在 .ui 菜单栏中的序号)
        /// </summary>
        int                         _activeTab  =   0;
        /// <summary>
        /// 功能区横向滚动偏移(像素),换页时归零
        /// </summary>
        float                       _ribbonScrollX  =   0.0f;
        /// <summary>
        /// 各鼠标键的拖拽是否已被 GUI 接管(按下时决定,抬起时释放)
        /// </summary>
        bool                        _guiDrag[3] =   {   false,false,false  };
        /// <summary>
        /// 文件选择对话框(打开工程 / 保存工程)
        /// </summary>
        FEFileDialog                _fileDialog;
        /// <summary>
        /// 文件对话框确认后走保存(true)还是打开(false)
        /// </summary>
        bool                        _fileDialogSaveMode =   false;
        /// <summary>
        /// 文件对话框确认后是否走"导入模型"(优先级低于保存)
        /// </summary>
        bool                        _fileDialogImportMode   =   false;
        /// <summary>
        /// 文件对话框的起始目录(初始为程序目录)
        /// </summary>
        std::string                 _startDir;
        /// <summary>
        /// 当前工程的完整路径(UTF-8);为空表示尚未保存过
        /// </summary>
        std::string                 _projectName;
        /// <summary>
        /// 状态栏/提示信息(打开或保存的结果)
        /// </summary>
        std::string                 _statusText;
        /// <summary>
        /// 待执行的场景操作队列。Ribbon 与对话框回调都发生在渲染趟内部(Overlay
        /// 工厂的 render 回调),此时直接增删场景会让新建的渲染工厂赶在同一帧被渲染,
        /// 而它的 VBO/IBO 要等工厂 update 才生成(表现为绑定空索引缓冲崩溃);
        /// 故把动作压入队列,推迟到下一帧 MSG_UPDATE 统一执行
        /// </summary>
        std::vector<std::function<void()>>  _pendingActions;
        /// <summary>
        /// 撤销/重做栈。命令的 redo/undo 会增删场景节点,所以按钮触发的撤销/重做
        /// 同样要经 _pendingActions 推迟到 MSG_UPDATE 执行。
        /// 打开/重置工程会替换整个场景,此时清空历史(命令引用的旧节点已无意义)
        /// </summary>
        FEUndoStack                         _undoStack;
        /// <summary>
        /// 快捷键表(命令来自 .ui,绑定保存在 exe 同名 XML)
        /// </summary>
        FEShortcutConfig                    _shortcuts;
        /// <summary>
        /// 动作名 → .ui 菜单项,供快捷键分发查找
        /// </summary>
        std::map<std::string,const FEUiMenuItem*>   _actionById;
        /// <summary>
        /// 快捷键设置窗口开关
        /// </summary>
        bool                    _shortcutOpen      =   false;
        /// <summary>
        /// 配置窗口开关
        /// </summary>
        bool                    _settingOpen       =   false;
        /// <summary>
        /// 配置窗口的错误提示
        /// </summary>
        std::string             _settingError;
        /// <summary>
        /// 设置窗口内尚未保存的改动(关闭时提示丢弃)
        /// </summary>
        bool                    _shortcutDirty     =   false;
        /// <summary>
        /// 表格选中行;-1 表示未选中
        /// </summary>
        int                     _shortcutSelected  =   -1;
        /// <summary>
        /// 正在录制的行;-1 表示不在录制态
        /// </summary>
        int                     _shortcutRecord    =   -1;
        /// <summary>
        /// 录制中的候选组合
        /// </summary>
        int                     _recordKey         =   0;
        bool                    _recordCtrl        =   false;
        bool                    _recordShift       =   false;
        bool                    _recordAlt         =   false;
        /// <summary>
        /// 录制候选中已按下过非修饰键
        /// </summary>
        bool                    _recordPressed     =   false;
        /// <summary>
        /// 设置窗口的错误提示(重复绑定等)
        /// </summary>
        std::string             _shortcutError;
    };
}

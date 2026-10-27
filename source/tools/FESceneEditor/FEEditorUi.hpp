#pragma     once

#include    "../../../inc/graphic/FEFactoryRender.hpp"
#include    "../../../inc/graphic/FEImGuiBackend.h"

#include    "FEUiDocument.hpp"
#include    "FEUiRenderer.hpp"
#include    "FEModelTree.hpp"

#include    <filesystem>
#include    <functional>
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
    /// 自绘的文件选择对话框(ImGui 窗口,不依赖平台 API):
    ///   驱动/上级/常用位置导航 + 目录列表 + 当前目录文件列表 + 文件名输入。
    /// 用法:open() 置起标志,之后每帧 draw(),返回 true 时对话框结束。
    /// </summary>
    class   FEFileDialog
    {
    public:
        /// <summary>
        /// 对话框模式
        /// </summary>
        enum    Mode
        {
            kNone   =   0,
            /// <summary>
            /// 打开文件
            /// </summary>
            kOpen   =   1,
            /// <summary>
            /// 另存为
            /// </summary>
            kSave   =   2,
        };
    public:
        /// <summary>
        /// 弹出对话框
        /// </summary>
        /// <param name="mode">模式(打开/另存为)</param>
        /// <param name="title">窗口标题</param>
        /// <param name="ext">扩展名过滤,例如 .fepj;为空表示显示全部文件</param>
        /// <param name="dir">初始目录;为空时用当前工作目录</param>
        /// <param name="defaultName">另存为模式下的默认文件名</param>
        void    open(Mode mode,const std::string& title,const std::string& ext,
                     const std::string& dir,const std::string& defaultName);
        /// <summary>
        /// 关闭对话框(不产出结果)
        /// </summary>
        void    close();
        /// <summary>
        /// 每帧绘制;返回 true 表示本次调用结束了对话框
        /// </summary>
        /// <param name="iconFont">图标字体,可为 nullptr</param>
        /// <param name="resultPath">确认时写入完整路径(UTF-8);取消时不变</param>
        /// <returns>true 表示对话框已关闭</returns>
        bool    draw(ImFont* iconFont,std::string& resultPath);
        /// <summary>
        /// 对话框是否处于打开状态
        /// </summary>
        bool    isOpen() const  {   return  _mode != kNone;   }
    protected:
        /// <summary>
        /// 列表项:一个文件或一个目录
        /// </summary>
        struct  Entry
        {
            std::string     _name;
            bool            _dir    =   false;
        };
        /// <summary>
        /// 左侧导航栏的快捷跳转项
        /// </summary>
        struct  QuickItem
        {
            /// <summary>
            /// 显示文本,例如 桌面 / 文档 / C:
            /// </summary>
            std::string             _label;
            std::filesystem::path   _path;
        };
        /// <summary>
        /// 重新扫描当前目录下的子目录与匹配文件
        /// </summary>
        void    refresh();
        /// <summary>
        /// 收集左侧导航栏的常用目录(主目录/桌面/下载/文档/程序目录/各盘符),
        /// 仅收录实际存在的目录
        /// </summary>
        /// <param name="startDir">初始目录(UTF-8),作为"程序目录"快捷项</param>
        void    buildQuickItems(const std::string& startDir);
        /// <summary>
        /// 切换到指定目录(规范化后扫描)
        /// </summary>
        /// <param name="dir">目标目录(UTF-8)</param>
        void    enter(const std::filesystem::path& dir);
        /// <summary>
        /// 地址栏跳转:可输入目录、盘符根(如 D:\)或文件路径。
        /// 输入文件时跳到其所在目录并填入文件名;路径无效时保留原位并提示
        /// </summary>
        /// <param name="text">用户输入的路径</param>
        void    gotoPath(const std::string& text);
        /// <summary>
        /// 把指定路径写入地址栏输入缓冲
        /// </summary>
        /// <param name="utf8Path">UTF-8 路径</param>
        void    setPathBuf(const std::string& utf8Path);
    protected:
        Mode        _mode   =   kNone;
        std::string _title;
        /// <summary>
        /// 扩展名过滤,例如 .fepj
        /// </summary>
        std::string _ext;
        /// <summary>
        /// 当前目录
        /// </summary>
        std::filesystem::path   _dir;
        /// <summary>
        /// 文件名输入缓冲
        /// </summary>
        char        _nameBuf[260]   =   {   0   };
        /// <summary>
        /// 地址栏输入缓冲(当前目录;可直接编辑以跳转,含盘符根)
        /// </summary>
        char        _pathBuf[1024]  =   {   0   };
        std::vector<Entry>      _entries;
        /// <summary>
        /// 左侧导航栏的快捷跳转项(弹出对话框时构建一次)
        /// </summary>
        std::vector<QuickItem>  _quickItems;
        /// <summary>
        /// 需要重新扫描(切换目录后置位)
        /// </summary>
        bool        _dirty  =   true;
        /// <summary>
        /// 上一帧的选中项(用于双击判定)
        /// </summary>
        int         _selected   =   -1;
        /// <summary>
        /// 状态提示(无法进入目录等)
        /// </summary>
        std::string _notice;
        /// <summary>
        /// 首次绘制时需要 SetNextWindowSize
        /// </summary>
        bool        _firstFrame =   true;
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
        /// 把新建节点挂到场景:加入节点树并分发到渲染工厂与各系统
        /// </summary>
        /// <param name="node">新建的节点</param>
        /// <param name="nodeName">节点名(用于模型树显示与唯一化)</param>
        void    addNodeToScene(Node node,const std::string& nodeName);
        /// <summary>
        /// .ui 文件预览窗口(用 ImGui 渲染解析出的控件树)
        /// </summary>
        void    drawUiPreview();
        /// <summary>
        /// 模型树窗口(参考 QtTree 的 ImGui 实现)
        /// </summary>
        void    drawModelTree();
        /// <summary>
        /// 切换场景网格显隐
        /// </summary>
        void    toggleGrid();
        /// <summary>
        /// 相机复位到默认视角
        /// </summary>
        void    resetCamera();
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
        /// 模型树窗口开关
        /// </summary>
        bool                        _treeOpen       =   true;
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
    };
}

#pragma     once

#include    "../../../inc/graphic/FEFactoryRender.hpp"
#include    "../../../inc/graphic/FEImGuiBackend.h"

#include    "FEUiDocument.hpp"
#include    "FEUiRenderer.hpp"
#include    "FEModelTree.hpp"

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
        /// Ribbon 功能区(标签页 + 分组大按钮)与关于窗口
        /// </summary>
        void    drawRibbon();
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
        /// Font Awesome 图标字体(大按钮图标),随 ImGui 上下文一起销毁
        /// </summary>
        ImFont*                     _iconFont   =   nullptr;
        FEScene*                    _scene      =   nullptr;
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
        /// 当前 Ribbon 标签页:0 文件 / 1 开始 / 2 视图 / 3 帮助 / 4 界面
        /// </summary>
        int                         _activeTab  =   1;
        /// <summary>
        /// 各鼠标键的拖拽是否已被 GUI 接管(按下时决定,抬起时释放)
        /// </summary>
        bool                        _guiDrag[3] =   {   false,false,false  };
    };
}

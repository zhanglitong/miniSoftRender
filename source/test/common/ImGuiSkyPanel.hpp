#pragma     once

#include    "../../inc/graphic/FEFactoryRender.hpp"
#include    "../../inc/graphic/FEImGuiBackend.h"

#include    <functional>
#include    <vector>

namespace   FE
{
    class   FESkyComponent;
    class   FEScene;
    class   FEMessage;

    /// <summary>
    /// 调试 GUI 工厂:把 ImGui 的一帧画到屏幕上(与具体 UI 内容无关)。
    /// 绘制属性设为 USAGE_Overlay —— 场景工厂之后的最后一层,保证 UI 不被几何覆盖。
    /// </summary>
    class   FEImGuiFactory :public FEFactoryRender
    {
    public:
        FEImGuiFactory(FEContext& ctx,FEImGuiBackend* backend);

        /// 每帧构建 UI 的回调(NewFrame 之后、Render 之前调用)
        std::function<void()>           _buildUI;
        /// 后端即将销毁时断开工链接,避免工厂持有悬空指针
        void    invalidateBackend()     {   _backend    =   nullptr;    }
    public:
        virtual void    update(CMDPtr cmd)  override;
        virtual void    render(CMDPtr cmd)  override;
    protected:
        /// 把 ImGui 的绘制数据扁平化成与后端无关的形式
        void    extractDrawData();
    protected:
        FEImGuiBackend*             _backend    =   nullptr;
        FEImGuiDrawData             _drawData;
        std::vector<char>           _vtx;
        std::vector<char>           _idx;
        std::vector<FEImGuiDrawCmd> _cmds;
    };

    /// <summary>
    /// 天空参数调试面板(公共实现,供 test.sky / test.pbr.* 共用):
    ///   - attach() 负责 ImGui 上下文、渲染后端、字体图集与 Overlay 工厂的完整挂接;
    ///   - 显隐由 FESkyComponent::panelVisible() 控制,可在运行时通过接口切换;
    ///   - 滑条改动立即写回 FESkyComponent(实时生效,含 PBR 解析天空 IBL)。
    /// </summary>
    struct  FEImGuiSkyPanel
    {
        float   _turbidity  =   10.0f;
        float   _rayleigh   =   3.0f;
        float   _mie        =   0.005f;
        float   _mieG       =   0.7f;
        float   _luminance  =   0.05f;
        float   _elevation  =   65.0f;
        float   _azimuth    =   0.0f;
        float   _autoSpeed  =   2.0f;
        bool    _autoSun    =   false;
        /// 云参数(命名与 three.js SkyMesh / webgpu_sky 面板一致)
        float   _cloudCoverage  =   0.4f;
        float   _cloudDensity   =   0.4f;
        float   _cloudElevation =   0.5f;
        float   _cloudSpeed     =   0.00002f;

        /// <summary>
        /// 挂接调试界面。visible=false、场景无天空、或后端不支持 ImGui(WebGPU/
        /// 无窗口离屏)时返回 false,且不创建任何 ImGui 资源。
        /// </summary>
        bool    attach(FEContext& ctx,FEScene* scene,bool visible = true);
        /// <summary>
        /// 释放 ImGui 后端与上下文。必须在场景(device)销毁之前调用。
        /// </summary>
        void    detach();
        bool    attached()  const   {   return  _attached;  }
        /// <summary>
        /// 把窗口消息(鼠标)喂给 ImGui。返回 true 表示 GUI 捕获了鼠标,
        /// 宿主应跳过场景处理,避免拖动滑条时同时旋转相机。
        /// 未挂接时直接返回 false。
        /// </summary>
        bool    onMessage(const FEMessage& msg);

        void    draw(FESkyComponent* sky);

    protected:
        FEImGuiFactory*             _factory    =   nullptr;
        FEImGuiBackend*             _backend    =   nullptr;
        bool                        _hasContext =   false;
        bool                        _attached   =   false;
    };
}

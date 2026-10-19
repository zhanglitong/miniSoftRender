#pragma     once

#include    "../../../inc/graphic/FEFactoryRender.hpp"
#include    "../../../inc/graphic/FEImGuiBackend.h"

#include    <functional>
#include    <vector>

namespace   FE
{
    class   FESkyComponent;

    /// <summary>
    /// 调试 GUI 工厂:只负责“把 ImGui 的一帧画到屏幕上”。
    /// 绘制属性(usages)设为 USAGE_Overlay —— FEViewer 的最后一个绘制层。
    /// 场景里的工厂默认归属 USAGE_Scene(见 FEFactoryRender::_usages),本工厂
    /// 放在其后的 Overlay 层,并把渲染优先级设为 EP_Last,保证绝对最后绘制、
    /// 不被场景几何覆盖。
    /// 具体 UI 内容由 _buildUI 回调提供,因此本工厂与后端/业务无关。
    /// </summary>
    class   FEImGuiFactory :public FEFactoryRender
    {
    public:
        FEImGuiFactory(FEContext& ctx,FEImGuiBackend* backend);

        /// 每帧构建 UI 的回调(NewFrame 之后、Render 之前调用)
        std::function<void()>           _buildUI;
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
    /// 天空参数调试面板:滑条改动立即写回 FESkyComponent(实时生效)。
    /// </summary>
    struct  FEImGuiSkyPanel
    {
        float   _turbidity  =   10.0f;
        float   _rayleigh   =   3.0f;
        float   _mie        =   0.005f;
        float   _mieG       =   0.8f;
        float   _luminance  =   0.05f;
        float   _elevation  =   45.0f;
        float   _azimuth    =   180.0f;
        float   _autoSpeed  =   2.0f;
        bool    _autoSun    =   false;

        void    draw(FESkyComponent* sky);
    };
}

#pragma     once

#include    "graphic/FESwapchain.h"

namespace   FE
{
    /// <summary>
    /// 离屏交换链:无窗口/无头渲染时替代窗口交换链。
    /// 只持有一个颜色目标与一帧,acquireNextFrame 直接返回该帧,
    /// queuePresent 不做任何呈现,像素通过 FEDevice::readbackImage 回读。
    /// </summary>
    class   FEOffscreenSwapchain   :public FESwapchain
    {
    public:
        FEOffscreenSwapchain(FEContext& ctx)
            :FESwapchain(ctx)
        {}
        FEOffscreenSwapchain(const FEOffscreenSwapchain& other)
            :FESwapchain(other)
        {
            _color  =   other._color;
            _view   =   other._view;
            _frame  =   other._frame;
        }
        virtual ~FEOffscreenSwapchain()
        {}
    public:
        virtual Frame       acquireNextFrame(uint64 timeout)    override;
        virtual bool        create(const CreateInfo& info)      override;
        virtual bool        queuePresent(const PresentInfo& pInfo)    override;
        virtual FEFormat    colorFormat() const     override;
        virtual GImage      colorImage() const      override;
    protected:
        /// <summary>
        /// 离屏颜色目标(color attachment + transfer src + sampled)
        /// </summary>
        GImage      _color  =   nullptr;
        GImgView    _view   =   nullptr;
        Frame       _frame  =   nullptr;
    };
}

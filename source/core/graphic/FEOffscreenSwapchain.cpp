#include    "FEOffscreenSwapchain.h"
#include    "graphic/FEDevice.h"

namespace   FE
{
    Frame   FEOffscreenSwapchain::acquireNextFrame(uint64 timeout)
    {
        UNUSED(timeout);
        /// 单帧:上一帧的 fence wait 由 FEScene::onFrameStart 里的 frame->reset() 完成
        return  _frame;
    }

    bool    FEOffscreenSwapchain::create(const CreateInfo& info)
    {
        _cInfo  =   info;
        auto&   device  =   _ctx.device();

        /// 颜色目标:格式与 Vulkan 管线硬编码的 RGBA8 一致,
        /// 用途需包含 TRANSFER_SRC 才能回读像素
        _color  =   device.createGImage();
        if (_color == nullptr)
            return  false;

        FEGImage::CreateInfo    imgInfo =   {};
        imgInfo._format                 =   FMT_R8G8B8A8_UNORM;
        imgInfo._aspect                 =   ASPECT_COLOR_BIT;
        imgInfo._type                   =   IT_2D;
        imgInfo._layout                 =   IL_UNDEFINED;
        imgInfo._usage                  =   USAGE_COLOR_ATTACHMENT | USAGE_TRANSFER_SRC | USAGE_SAMPLED;
        imgInfo._width                  =   info._width;
        imgInfo._height                 =   info._height;
        imgInfo._depth                  =   1;
        imgInfo._mips                   =   1;
        imgInfo._layer                  =   1;
        if (!_color->create(imgInfo))
            return  false;

        _view   =   _color->createView();
        if (_view == nullptr)
            return  false;

        /// 单帧:只建 fence(创建即 signaled),无头模式不需要 acquire/present 信号量
        _frame              =   new FEFrame(_ctx);
        _frame->_fenceWait  =   device.createFence();
        if (_frame->_fenceWait == nullptr || !_frame->_fenceWait->create({}))
            return  false;

        _frame->_imageViewer    =   _view;
        _frame->_imageIdx       =   0;
        return  true;
    }

    bool    FEOffscreenSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        UNUSED(pInfo);
        /// 无头模式:没有窗口可呈现,像素由 readbackImage 回读
        return  true;
    }

    FEFormat    FEOffscreenSwapchain::colorFormat() const
    {
        return  FMT_R8G8B8A8_UNORM;
    }

    GImage      FEOffscreenSwapchain::colorImage() const
    {
        return  _color;
    }
}

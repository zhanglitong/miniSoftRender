#include    "SWSwapchain.h"
#include    "SWDevice.h"
#include    "SWRenderSystem.h"

namespace   FE
{
    SWSwapchain::~SWSwapchain()
    {
        cleanup();
    }

    bool    SWSwapchain::acquireNextImage(uint64 timeout, Semaphore sem, Fence fence,uint& imageIndex)
    {
        return  true;
    }

    GImgViews   SWSwapchain::imageViews() const
    {
        return  _imageViews;
    }

    bool    SWSwapchain::create(const SWSwapchain::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    bool    SWSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        return  true;
    }

    void    SWSwapchain::cleanup()
    {
        _imageViews.clear();
        _images.clear();
    }
}

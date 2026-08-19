#include    "SWSwapchain.h"
#include    "SWDevice.h"
#include    "SWRenderSystem.h"

namespace   FE
{
    SWSwapchain::~SWSwapchain()
    {
        cleanup();
    }

    Frame    SWSwapchain::acquireNextFrame(uint64 timeout)
    {
        (void)timeout;
        return  nullptr;
    }

    bool    SWSwapchain::create(const SWSwapchain::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    bool    SWSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        (void)pInfo;
        return  true;
    }

    void    SWSwapchain::cleanup()
    {
        _imageViews.clear();
        _images.clear();
    }
}

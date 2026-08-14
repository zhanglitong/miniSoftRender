#pragma     once

#include    "graphic/FESwapchain.h"
#include    "wgpu.h"
#include    "WGGImage.h"
#include    "WGGImageView.h"

namespace   FE
{
    class   WGSwapchain :public TRSObject<WGPUSurface, FESwapchain>
    {
    public:
        WGSwapchain(FEContext& ctx)
            :TRSObject<WGPUSurface, FESwapchain>(ctx)
        {}
        WGSwapchain(const WGSwapchain& other)
            :TRSObject<WGPUSurface, FESwapchain>(other)
        {}
        virtual ~WGSwapchain();

        virtual Frame   acquireNextFrame(uint64 timeout) override;
        virtual Frames  frames() const override;
        virtual bool    create(const CreateInfo& info) override;
        virtual bool    queuePresent(const PresentInfo& pInfo) override;
        virtual FEFormat    colorFormat() const override;

        void    initSurface(void* platformHandle,void* platformWindow);

    protected:
        GImages         _images;
        Frames          _frames;
        uint32_t        _currentImageIndex    =   0;
        WGPUTexture     _currentTexture       =   nullptr;
        WGPUTextureFormat    _colorFormat    =   WGPUTextureFormat_BGRA8Unorm;
    };
}
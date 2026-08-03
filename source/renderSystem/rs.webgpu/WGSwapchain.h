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

        virtual bool    acquireNextImage(uint64 timeout,Semaphore sem,Fence fence,uint& imageIndex) override;
        virtual GImgViews    imageViews() const override;
        virtual bool    create(const CreateInfo& info) override;
        virtual bool    queuePresent(const PresentInfo& pInfo) override;
        virtual FEFormat    colorFormat() const override;

        void    initSurface(void* platformHandle,void* platformWindow);

    protected:
        GImages        _images;
        GImgViews      _imageViews;
        uint32_t       _currentImageIndex    =   0;
        WGPUTexture    _currentTexture       =   nullptr;
        WGPUTextureFormat    _colorFormat    =   WGPUTextureFormat_BGRA8Unorm;
    };
}
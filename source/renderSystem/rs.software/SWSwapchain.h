#pragma     once

#include    "graphic/FESwapchain.h"
#include    "graphic/FEDevice.h"
#include    "SWGImage.h"
#include    "SWGImageView.h"

namespace   FE
{
    class   SWSwapchain :public TRSObject<void*,FESwapchain>
    {
    public:
        SWSwapchain(FEContext& ctx)
            :TRSObject<void*,FESwapchain>(ctx)
        {}
        SWSwapchain(const SWSwapchain& other)
            :TRSObject<void*,FESwapchain>(other)
        {}
        virtual ~SWSwapchain();
    public:
        virtual Frame       acquireNextFrame(uint64 timeout)  override;
        virtual Frames      frames() const override;
        virtual bool        create(const CreateInfo& info)  override;
        virtual bool        queuePresent(const PresentInfo& pInfo) override;
        virtual FEFormat    colorFormat() const
        {
            return  FMT_R8G8B8A8_UNORM;
        }

        void    cleanup();
    protected:
        GImages             _images             {};
        GImgViews           _imageViews         {};
    };
}

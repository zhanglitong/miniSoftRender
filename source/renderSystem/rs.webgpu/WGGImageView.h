#pragma     once

#include    "graphic/FEGImageView.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGGImage;

    class   WGGImageView :public TRSObject<WGPUTextureView, FEGImageView>
    {
    public:
        WGGImageView(FEContext& ctx,WGPUTextureView native = 0)
            :TRSObject<WGPUTextureView, FEGImageView>(ctx,native)
        {}
        WGGImageView(const WGGImageView& other)
            :TRSObject<WGPUTextureView, FEGImageView>(other)
        {}
        virtual ~WGGImageView();

        virtual void    destroy() override
        {
            if (_native)
            {
                wgpuTextureViewRelease(_native);
                _native =   nullptr;
            }
        }
        void    attach(WGPUTextureView viewer)
        {
            _native =   viewer;
        }   

        virtual bool    create(const CreateInfo& info) override;
    };
}
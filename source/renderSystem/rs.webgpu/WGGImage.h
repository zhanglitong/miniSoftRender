#pragma     once

#include    "graphic/FEGImage.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGGImageView;

    class   WGGImage :public TRSObject<WGPUTexture, FEGImage>
    {
    public:
        WGGImage(FEContext& ctx,WGPUTexture native = 0,bool bRef = false)
            :TRSObject<WGPUTexture, FEGImage>(ctx,native)
            , _ref(bRef)
        {}
        WGGImage(const WGGImage& other)
            :TRSObject<WGPUTexture, FEGImage>(other)
        {}
        virtual ~WGGImage();

        virtual bool    create(const CreateInfo& info) override;
        virtual GImgView    createView() override;

    protected:
        bool    _ref    =   false;
    };
}
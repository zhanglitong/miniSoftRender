#pragma     once

#include    "graphic/FEGImage.h"

namespace   FE
{
    class   SWGImage :public TRSObject<void*,FEGImage>
    {
    protected:
        bool    _ref    =   false;
    public:
        SWGImage(FEContext& ctx,void* native = nullptr,bool bRef = false)
            :TRSObject<void*,FEGImage>(ctx,native)
        {
            _ref    =   bRef;
        }
        SWGImage(const SWGImage& other)
            :TRSObject<void*,FEGImage>(other)
        {}
        virtual ~SWGImage();

        virtual bool        create(const CreateInfo& info)  override;
        virtual GImgView    createView() override;
    };
}

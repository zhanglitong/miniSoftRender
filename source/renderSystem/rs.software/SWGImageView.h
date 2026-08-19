#pragma     once

#include    "graphic/FEGImageView.h"

namespace   FE
{
    class   SWGImageView :public TRSObject<void*,FEGImageView>
    {
    protected:
        bool    _ref    =   false;
    public:
        SWGImageView(FEContext& ctx,void* native = nullptr,bool bRef = false)
            :TRSObject<void*,FEGImageView>(ctx,native)
        {
            _ref    =   bRef;
        }
        SWGImageView(const SWGImageView& other)
            :TRSObject<void*,FEGImageView>(other)
        {}
        virtual ~SWGImageView();

        inline  void    setImage(FEGImage* image)
        {
            _cInfo._image   =   image;
        }

        virtual bool    create(const CreateInfo& info)  override;
    };
}

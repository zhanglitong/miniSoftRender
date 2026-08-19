#pragma     once

#include    "RSObject.h"

namespace   FE
{
    class   FEGImage;
    using   GImage  =   SharedPtr<FEGImage>;

    class   FEGImageView :public RSObject
    {
    public:
        struct  CreateInfo
        {
            GImage  _image;
        };
    public:
        FEGImageView(FEContext& ctx)
            :RSObject(ctx)
        {}

        FEGImageView(const FEGImageView& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;
    protected:
        CreateInfo    _cInfo;
    };

    using   GImgView        =   SharedPtr<FEGImageView>;
    using   GImgViews       =   std::vector<GImgView>;
}



#pragma     once

#include    "RSObject.h"

namespace   FE
{
    class   FEGImage;
    using   GImage  =   SharedPtr<FEGImage>;

    class   FE_API  FEGImageView :public RSObject
    {
    public:
        struct  CreateInfo
        {
            GImage  _image;
            /// <summary>
            /// imageView 覆盖的 mip 范围(默认 0 和 image 的完整 mip 数)
            /// 用于 pass1 只渲 mip0(levelCount=1)、透明管线采样完整链
            /// </summary>
            uint    _baseMip    =   0;
            uint    _levelCount =   0;   /// 0 = 用 image._cInfo._mips
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



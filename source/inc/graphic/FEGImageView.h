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
            /// <summary>
            /// imageView 覆盖的层范围(仅 2D/2D_ARRAY 语义)。
            /// _layerCount=0 表示用 image 的全部层:层数=6 时建 CUBE 视图,
            /// 否则建单层 2D 视图;非 0 时强制建 2D 视图,
            /// 用于把 cubemap 的某一个面单独作为渲染目标(反射探针逐面渲染)
            /// </summary>
            uint    _baseLayer  =   0;
            uint    _layerCount =   0;
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



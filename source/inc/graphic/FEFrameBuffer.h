#pragma     once

#include    "../FEImage.h"
#include    "../FESmallVector.h"

#include    "FEPBuffer.h"
#include    "FERenderPass.h"
#include    "FEGImage.h"
#include    "FEGImageView.h"

namespace   FE
{
    /// <summary>
    /// 用来重载绘制结果的画布
    /// </summary>
    class   FEFrameBuffer :public RSObject
    {
    public:
        using   Colors  =   TSmallVector<FEGImageView,8>;
        struct  CreateInfo 
        {
            RenderPassPtr   _renderPass     =   nullptr;
            GImgViews       _colors;
            GImgView        _depth          =   nullptr;
            /// <summary>
            /// width  必须不能大于 pColors / pDepth : width;
            /// height 必须不能大于 pColors / pDepth : height;
            /// </summary>
            uint32_t        _width          =   0;
            uint32_t        _height         =   0;
            uint32_t        _layers         =   1;
        };
    public:
        FEFrameBuffer(FEContext& ctx)
            :RSObject(ctx)
        {}

        FEFrameBuffer(const FEFrameBuffer& other)
            :RSObject(other)
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;
    protected:
        CreateInfo  _cInfo;
    };
    using   FBCreateInfo    =   FEFrameBuffer::CreateInfo;
    using   FBOPtr          =   SharedPtr<FEFrameBuffer>;
    using   FrameBuffer     =   SharedPtr<FEFrameBuffer>;
    using   FrameBuffers    =   std::vector<FrameBuffer>;
}
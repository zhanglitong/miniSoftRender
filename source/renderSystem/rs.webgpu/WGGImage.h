#pragma     once

#include    "graphic/FEGImage.h"
#include    "FEWebgpu.hpp"
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

        virtual bool        create(const CreateInfo& info) override;
        virtual GImgView    createView() override;
        virtual GImgView    createView(uint baseMip,uint levelCount) override;
        /// 指定层范围(逐面渲染 cubemap 用):WebGPU 端必须建 2D 单层视图,
        /// 否则多层的 Cube/2D 视图作为渲染目标会被判定为 multiview 而校验失败
        virtual GImgView    createView(uint baseMip,uint levelCount,uint baseLayer,uint layerCount) override;

    protected:
        bool    _ref    =   false;
    };
}

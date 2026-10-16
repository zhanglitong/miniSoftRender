#pragma     once

#include    "graphic/FETexture.h"
#include    "WGGImage.h"
#include    "WGGImageView.h"
#include    "wgpu.h"

namespace   FE
{
    /// <summary>
    /// WebGPU 纹理:2D 图像 + 视图 + 采样器(对应 Vulkan 后端的 VKTexture)。
    /// native 句柄为 WGPUTextureView,描述符写入时通过 sampler()/native() 获取;
    /// 立方体贴图以 image 的 _layer == 6 表示,视图维度为 Cube。
    /// </summary>
    class   WGTexture :public TRSObject<WGPUTextureView,FETexture>
    {
    public:
        WGTexture(FEContext& ctx)
            :TRSObject<WGPUTextureView,FETexture>(ctx)
        {}
        WGTexture(const WGTexture& other)
            :TRSObject<WGPUTextureView,FETexture>(other)
        {}
        virtual ~WGTexture();

        virtual bool    create(const CreateInfo& info)  override;
        virtual bool    update(const void* data,uint64 size)    override;

        inline  WGPUSampler sampler() const
        {
            return  _sampler;
        }
        inline  WGPUTexture image() const
        {
            return  _image ? (WGPUTexture)_image->native() : nullptr;
        }
    protected:
        GImage      _image      =   nullptr;
        GImgView    _view       =   nullptr;
        WGPUSampler _sampler    =   nullptr;
    };
}

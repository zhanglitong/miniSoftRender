#pragma once

#include    "graphic/FETexture.h"
#include    "VKGImage.h"
#include    "VKGImageView.h"
#include    "FEVulkan.h"

namespace   FE
{
    /// <summary>
    /// Vulkan 纹理:2D 图像 + 视图 + 采样器
    /// 参考实现:D:\Vulkan\examples\texture(sascha willems)与 VKTexture.h
    /// native 句柄为 VkImageView,描述符写入时通过 sampler()/native() 获取
    /// </summary>
    class   VKTexture :public TRSObject<VkImageView,FETexture>
    {
    public:
        VKTexture(FEContext& ctx)
            :TRSObject<VkImageView,FETexture>(ctx)
        {}
        VKTexture(const VKTexture& other)
            :TRSObject<VkImageView,FETexture>(other)
        {}
        virtual ~VKTexture();

        virtual bool    create(const CreateInfo& info)  override;
        virtual bool    update(const void* data,uint64 size)    override;

        inline  VkSampler   sampler() const
        {
            return  _sampler;
        }
        inline  VkImage     image() const
        {
            return  _image ? (VkImage)_image->native() : nullptr;
        }
    protected:
        GImage      _image      =   nullptr;
        GImgView    _view       =   nullptr;
        VkSampler   _sampler    =   nullptr;
    };
}

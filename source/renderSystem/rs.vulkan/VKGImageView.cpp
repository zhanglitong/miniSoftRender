
#include    "VKGImage.h"
#include    "VKGImageView.h"
#include    "VKDevice.h"
namespace   FE
{
    VKGImageView::~VKGImageView()
    {
        if(!_ref && _native != nullptr)
            vkDestroyImageView(((VKDevice&)(_ctx.device())).logicalDevice(), _native, nullptr);
    }

    bool    VKGImageView::create(const VKGImageView::CreateInfo& info)
    {
        _cInfo              =   info;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        
        VkImageViewCreateInfo imageViewCI   =   {};
        {
            const auto& imgCInfo  =   info._image->cInfo();
            const bool  isCube    =   (imgCInfo._layer == 6);
            imageViewCI.sType               =    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCI.image               =    (VkImage)info._image->native();
            imageViewCI.viewType            =    isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.format              =    system2Native(imgCInfo._format);
            imageViewCI.subresourceRange    =   {};

            imageViewCI.subresourceRange.aspectMask         =   system2Native(imgCInfo._aspect);
            imageViewCI.subresourceRange.baseMipLevel       =   info._baseMip;
            imageViewCI.subresourceRange.levelCount         =   info._levelCount > 0
                                                                    ? info._levelCount
                                                                    : imgCInfo._mips;
            imageViewCI.subresourceRange.baseArrayLayer     =   0;
            imageViewCI.subresourceRange.layerCount         =   imgCInfo._layer;
            /// 仅深度+模板格式(D16S8/D24S8/D32S8)的视图需要附加模板 aspect;
            /// 不能用数值区间比较——块压缩格式(BC=131~146,ETC2/EAC=147~156,
            /// ASTC=157~172)的 VkFormat 数值同样落在该区间之上,误加模板位
            /// 会导致视图非法、采样返回黑(VK-VALIDATION-ERROR)
            switch (imageViewCI.format)
            {
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            default:
                break;
            }
            VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &_native));

            return  _native != nullptr;
        }
    }
}

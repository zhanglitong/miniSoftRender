
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
            imageViewCI.subresourceRange.baseMipLevel       =   0;
            imageViewCI.subresourceRange.levelCount         =   imgCInfo._mips;
            imageViewCI.subresourceRange.baseArrayLayer     =   0;
            imageViewCI.subresourceRange.layerCount         =   imgCInfo._layer;
            if (imageViewCI.format >= VK_FORMAT_D16_UNORM_S8_UINT) 
                imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &_native));

            return  _native != nullptr;
        }
    }
}

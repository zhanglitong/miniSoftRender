
#include    "VKFrameBuffer.h"
#include    "VKDevice.h"

namespace   FE
{

    VKFrameBuffer::~VKFrameBuffer()
    {
        if(_native != nullptr)
            vkDestroyFramebuffer(((VKDevice&)(_ctx.device())).logicalDevice(), _native, nullptr);
    }
    bool    VKFrameBuffer::create(const FEFrameBuffer::CreateInfo& info)  
    {
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        std::array<VkImageView, 2> attachments{};
        attachments[0]      =    (VkImageView)info._colors.front()->native();
        attachments[1]      =    (VkImageView)info._depth->native();   

        VkFramebufferCreateInfo cInfo{};
        cInfo.sType             =   VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        cInfo.renderPass        =   (VkRenderPass)info._renderPass->native();
        cInfo.attachmentCount   =   static_cast<uint32_t>(attachments.size());
        cInfo.pAttachments      =   attachments.data();
        cInfo.width             =   info._width;
        cInfo.height            =   info._height;
        cInfo.layers            =   info._layers;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &cInfo, nullptr, &_native));

        return  _native != nullptr;
    }
}
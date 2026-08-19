
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
        _cInfo              =   info;  
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        std::array<VkImageView, 32> attachments{};
        uint32_t                    iIndex      =   0;
        for (auto var: info._colors)
        {
            if(iIndex < sizeof(attachments)/sizeof(attachments[0]))
            {
                attachments[iIndex++] =   (VkImageView)info._colors.front()->native();
            }
        }
        if (info._depth && iIndex < sizeof(attachments)/sizeof(attachments[0]))
        {
            attachments[iIndex++] =   (VkImageView)info._depth->native();  
        } 

        VkFramebufferCreateInfo cInfo{};
        cInfo.sType             =   VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        cInfo.renderPass        =   info._renderPass != nullptr ? (VkRenderPass)(info._renderPass->native()) : nullptr;
        cInfo.attachmentCount   =   static_cast<uint32_t>(iIndex);
        cInfo.pAttachments      =   attachments.data();
        cInfo.width             =   info._width;
        cInfo.height            =   info._height;
        cInfo.layers            =   info._layers;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &cInfo, nullptr, &_native));

        return  _native != nullptr;
    }

    bool    VKFrameBuffer::resize(const uint3& dims) 
    {
        auto    dImage = _cInfo._depth ? _cInfo._depth->cInfo()._image : nullptr;
        if (dImage)
        {
            auto result         =   dImage->resize(dims,0,0);
            assert(result);
            if (result)
            {
                _cInfo._depth   =  dImage->createView();
                _cInfo._depth->create({dImage});
            }
            else
            {
                return  false;
            }
        }
        for (auto&  var : _cInfo._colors)
        {
            auto    cImage      =   var->cInfo()._image;
            auto    result      =   cImage->resize(dims,0,0);
            assert(result);
            if (result)
            {
                var   =  cImage->createView();
                var->create({cImage});
            }
            else
            {
                return  false;
            }
        }

        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        std::array<VkImageView, 32> attachments{};
        uint32_t                    iIndex      =   0;
        for (auto var: _cInfo._colors)
        {
            if(iIndex < sizeof(attachments)/sizeof(attachments[0]))
            {
                attachments[iIndex++] =   (VkImageView)_cInfo._colors.front()->native();
            }
        }
        if (_cInfo._depth && iIndex < sizeof(attachments)/sizeof(attachments[0]))
        {
            attachments[iIndex++] =   (VkImageView)_cInfo._depth->native();  
        } 

        VkFramebufferCreateInfo cInfo{};
        cInfo.sType             =   VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        cInfo.renderPass        =   _cInfo._renderPass != nullptr ? (VkRenderPass)(_cInfo._renderPass->native()) : nullptr;
        cInfo.attachmentCount   =   static_cast<uint32_t>(iIndex);
        cInfo.pAttachments      =   attachments.data();
        cInfo.width             =   _cInfo._width;
        cInfo.height            =   _cInfo._height;
        cInfo.layers            =   _cInfo._layers;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &cInfo, nullptr, &_native));

        fireNotify();

        return  _native != nullptr;
    }
}

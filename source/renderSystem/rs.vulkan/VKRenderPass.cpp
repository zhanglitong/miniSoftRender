#include    "VKRenderPass.h"
#include    "VkDevice.h"

namespace   FE
{
    VKRenderPass::~VKRenderPass()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyRenderPass(device,_native,nullptr);
        }
    }
    bool    VKRenderPass::create(const CreateInfo& cInfo)
    {
        _cInfo  =   cInfo;

        const bool withColor = cInfo._withColor;
        const bool withDepth = cInfo._withDepth;
        const uint32_t attachmentCount = (withColor ? 1u : 0u) + (withDepth ? 1u : 0u);

        std::vector<VkAttachmentDescription> attachments;
        attachments.reserve(attachmentCount);

        VkAttachmentReference   colorReference  =   {};
        VkAttachmentReference   depthReference  =   {};

        if (withColor)
        {
            uint32_t idx = (uint32_t)attachments.size();
            VkAttachmentDescription desc = {};
            desc.format             =   system2Native(cInfo._colorFmt);
            desc.samples            =   sampleCount2Native(cInfo._samples);
            desc.loadOp             =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            desc.storeOp            =   VK_ATTACHMENT_STORE_OP_STORE;
            desc.stencilLoadOp      =   VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp     =   VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout      =   VK_IMAGE_LAYOUT_UNDEFINED;
            desc.finalLayout        =   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            attachments.push_back(desc);

            colorReference.attachment   =   idx;
            colorReference.layout       =   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        if (withDepth)
        {
            uint32_t idx = (uint32_t)attachments.size();
            VkAttachmentDescription desc = {};
            desc.format             =   system2Native(cInfo._depthFmt);
            desc.samples            =   sampleCount2Native(cInfo._samples);
            desc.loadOp             =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            desc.storeOp            =   VK_ATTACHMENT_STORE_OP_STORE;
            desc.stencilLoadOp      =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            desc.stencilStoreOp     =   VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout      =   VK_IMAGE_LAYOUT_UNDEFINED;
            desc.finalLayout        =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(desc);

            depthReference.attachment   =   idx;
            depthReference.layout       =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpassDescription =   {};
        {
            subpassDescription.pipelineBindPoint          =   VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount       =   withColor ? 1u : 0u;
            subpassDescription.pColorAttachments          =   withColor ? &colorReference : nullptr;
            subpassDescription.pDepthStencilAttachment    =   withDepth ? &depthReference : nullptr;
        }

        // Subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies;
        if (withColor)
        {
            VkSubpassDependency d0 = {};
            d0.srcSubpass       = VK_SUBPASS_EXTERNAL;
            d0.dstSubpass       = 0;
            d0.srcStageMask     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            d0.dstStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            d0.srcAccessMask    = 0;
            d0.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies.push_back(d0);

            VkSubpassDependency d1 = {};
            d1.srcSubpass       = 0;
            d1.dstSubpass       = VK_SUBPASS_EXTERNAL;
            d1.srcStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            d1.dstStageMask     = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            d1.srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            d1.dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT;
            dependencies.push_back(d1);
        }
        if (withDepth)
        {
            VkSubpassDependency d2 = {};
            d2.srcSubpass       = VK_SUBPASS_EXTERNAL;
            d2.dstSubpass       = 0;
            d2.srcStageMask     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            d2.dstStageMask     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            d2.srcAccessMask    = 0;
            d2.dstAccessMask    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies.push_back(d2);
        }

        VkRenderPassCreateInfo renderPassInfo   =   {};
        {
            renderPassInfo.sType            =   VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount  =   attachmentCount;
            renderPassInfo.pAttachments     =   attachments.empty() ? nullptr : attachments.data();
            renderPassInfo.subpassCount     =   1;
            renderPassInfo.pSubpasses       =   &subpassDescription;
            renderPassInfo.dependencyCount  =   (uint32_t)dependencies.size();
            renderPassInfo.pDependencies    =   dependencies.empty() ? nullptr : dependencies.data();
        }
        auto&   vkDev   =   (VKDevice&)(_ctx.device());
        VK_CHECK_RESULT(vkCreateRenderPass(vkDev.logicalDevice(), &renderPassInfo, nullptr, &_native));

        return  _native != nullptr;
    }
}

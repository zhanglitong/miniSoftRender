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

        std::array<VkAttachmentDescription, 2> attachments  =   {};
        {
            // Color attachment
            VkAttachmentDescription &   color   =   attachments[0];
            color.format             =   system2Native(cInfo._colorFmt);
            color.samples            =   VK_SAMPLE_COUNT_1_BIT;
            color.loadOp             =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp            =   VK_ATTACHMENT_STORE_OP_STORE;
            color.stencilLoadOp      =   VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color.stencilStoreOp     =   VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color.initialLayout      =   VK_IMAGE_LAYOUT_UNDEFINED;
            color.finalLayout        =   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
            // Depth attachment
            VkAttachmentDescription&    depth   =    attachments[1];
            depth.format            =   system2Native(cInfo._depthFmt);;
            depth.samples           =   VK_SAMPLE_COUNT_1_BIT;
            depth.loadOp            =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp           =   VK_ATTACHMENT_STORE_OP_STORE;
            depth.stencilLoadOp     =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth.stencilStoreOp    =   VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth.initialLayout     =   VK_IMAGE_LAYOUT_UNDEFINED;
            depth.finalLayout       =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference   colorReference  =   {};
        { 
            colorReference.attachment   =   0;
            colorReference.layout       =   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        };
        VkAttachmentReference   depthReference  =   {};
        { 
            depthReference.attachment   =   1;
            depthReference.layout       =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpassDescription =   {};
        {
            subpassDescription.pipelineBindPoint          =   VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount       =   1;
            subpassDescription.pColorAttachments          =   &colorReference;
            subpassDescription.pDepthStencilAttachment    =   &depthReference;
        }

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 3> dependencies =   {};
        {
            VkSubpassDependency&    depend0    =  dependencies[0];
            
            depend0.srcSubpass      =   VK_SUBPASS_EXTERNAL;
            depend0.dstSubpass      =   0;
            depend0.srcStageMask    =   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            depend0.dstStageMask    =   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            depend0.srcAccessMask   =   0;
            depend0.dstAccessMask   =   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
            VkSubpassDependency&    depend1    =  dependencies[1];
            depend1.srcSubpass      =   0;
            depend1.dstSubpass      =   VK_SUBPASS_EXTERNAL;
            depend1.srcStageMask    =   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            depend1.dstStageMask    =   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            depend1.srcAccessMask   =   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            depend1.dstAccessMask   =   VK_ACCESS_MEMORY_READ_BIT;

            VkSubpassDependency&    depend2    =  dependencies[2];
            depend2.srcSubpass      =   VK_SUBPASS_EXTERNAL;
            depend2.dstSubpass      =   0;
            depend2.srcStageMask    =   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            depend2.dstStageMask    =   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            depend2.srcAccessMask   =   0;
            depend2.dstAccessMask   =   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        VkRenderPassCreateInfo renderPassInfo   =   {};
        {
            renderPassInfo.sType            =   VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount  =   static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments     =   attachments.data();
            renderPassInfo.subpassCount     =   1;
            renderPassInfo.pSubpasses       =   &subpassDescription;
            renderPassInfo.dependencyCount  =   static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies    =   dependencies.data();
        }
        auto&   vkDev   =   (VKDevice&)(_ctx.device());
        VK_CHECK_RESULT(vkCreateRenderPass(vkDev.logicalDevice(), &renderPassInfo, nullptr, &_native));

        return  _native != nullptr;
    }
}

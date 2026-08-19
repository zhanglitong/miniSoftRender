#pragma     once

#include    "graphic/FERenderPass.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKRenderPass :public TRSObject<VkRenderPass,FERenderPass>
    {
    public:
        VKRenderPass(FEContext& ctx)
            :TRSObject<VkRenderPass,FERenderPass>(ctx)
        {
        }
        VKRenderPass(const VKRenderPass& other)
            :TRSObject<VkRenderPass,FERenderPass>(other)
        {}
        virtual ~VKRenderPass();

        virtual bool    create(const CreateInfo& cInfo)   override;
    };
}

#pragma     once

#include    "graphic/FEDSetLayout.h"

#include    "FEVulkan.h"

namespace   FE
{
    class   VKDSetLayout :public TRSObject<VkDescriptorSetLayout,FEDSetLayout>
    {
    public:
        VKDSetLayout(FEContext& ctx)
            :TRSObject<VkDescriptorSetLayout,FEDSetLayout>(ctx)
        {
        }
        VKDSetLayout(const VKDSetLayout& other)
            :TRSObject<VkDescriptorSetLayout,FEDSetLayout>(other)
        {}

        virtual ~VKDSetLayout();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual DSet    createDSet()    override ;
    };
}

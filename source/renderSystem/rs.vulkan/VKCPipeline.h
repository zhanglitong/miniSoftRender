#pragma     once

#include    "graphic/FEGPipeline.h"
#include    "FEVulkan.h"
#include    "VKDSetPool.h"
#include    "VKDSetLayout.h"

namespace   FE
{

    using   DSetPoolVKs =   std::vector<DSetPoolVK>;
    class   VKCPipeline :public TRSObject<VkPipeline,FEGPipeline>
    {
    protected:
        DSetPoolVKs _pools;
    public:
        VKCPipeline(FEContext& ctx)
            :TRSObject<VkPipeline,FEGPipeline>(ctx)
        {}
        VKCPipeline(const VKCPipeline& other)
            :TRSObject<VkPipeline,FEGPipeline>(other)
        {}

        virtual ~VKCPipeline();

        virtual bool    create(const CreateInfo& info)  override;

        virtual DSets   createDSets()  override;

        virtual Handle  nativeLayout() const override
        {
            return  Handle(_layout);
        }
        virtual PLType  type() const override
        {
            return  PL_COMPUTE;
        }
    protected:
        VkPipelineLayout    _layout         =   nullptr;
        DSetLayouts         _dsLayouts      =   {};
    };
}

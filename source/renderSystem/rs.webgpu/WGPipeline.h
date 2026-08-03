#pragma     once

#include    "graphic/FEGPipeline.h"
#include    "wgpu.h"
#include    "WGDSetLayout.h"
#include    "WGDSetPool.h"

namespace   FE
{
    class   WGPipeline :public TRSObject<WGPURenderPipeline, FEGPipeline>
    {
    public:
        WGPipeline(FEContext& ctx)
            :TRSObject<WGPURenderPipeline, FEGPipeline>(ctx)
        {}
        WGPipeline(const WGPipeline& other)
            :TRSObject<WGPURenderPipeline, FEGPipeline>(other)
        {}

        virtual ~WGPipeline();

        virtual bool    create(const CreateInfo& info) override;
        virtual DSets    createDSets() override;
        virtual Handle    nativeLayout() const override
        {
            return Handle(_layout);
        }

    protected:
        WGPUPipelineLayout    _layout    =   nullptr;
        std::vector<WGDSetLayout*>    _dsLayouts;
        std::vector<DSetPool>    _pools;
    };
}
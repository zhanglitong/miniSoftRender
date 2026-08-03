#pragma     once

#include    "graphic/FEGPipeline.h"
#include    "SWDSetPool.h"

namespace   FE
{
    using   DSetPoolSWs =   std::vector<SharedPtr<SWDSetPool>>;

    class   SWGPipeline :public TRSObject<void*,FEGPipeline>
    {
    protected:
        DSetPoolSWs _pools;
    public:
        SWGPipeline(FEContext& ctx)
            :TRSObject<void*,FEGPipeline>(ctx)
        {
        }
        SWGPipeline(const SWGPipeline& other)
            :TRSObject<void*,FEGPipeline>(other)
        {}

        virtual ~SWGPipeline();

        virtual bool    create(const CreateInfo& info)  override;

        virtual DSets   createDSets()  override;

        virtual Handle  nativeLayout() const override
        {
            return  Handle(nullptr);
        }
    protected:
        DSetLayouts         _dsLayouts      =   {};
    };
}

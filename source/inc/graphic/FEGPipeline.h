#pragma     once

#include    "FEPipeline.h"

namespace   FE
{
    class   FE_API  FEGPipeline :public FEPipeline
    {
    public:
        FEGPipeline(FEContext& ctx)
            :FEPipeline(ctx)
        {}
        FEGPipeline(const FEGPipeline& other)
            :FEPipeline(other)
        {
            _cInfo  =   other._cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;
        virtual PLType  type() const override
        {
            return  PL_GRAPIC;
        }
    };
    using   GPipeline       =   SharedPtr<FEGPipeline>;
    using   GPipelinePtr    =   SharedPtr<FEGPipeline>;
}

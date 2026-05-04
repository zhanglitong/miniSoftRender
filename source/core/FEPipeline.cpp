
#include    "FEPipeline.h"
namespace   FE
{
   
    CallResult  FEPipeline::create(const FEPipelineInfo& info)
    {
        _cInfo  =   info;
        return  CallResult::CR_SUCCESS;
    }
}
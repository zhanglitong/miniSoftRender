#include    "SWGPipeline.h"
#include    "SWDevice.h"
#include    "SWShader.h"

namespace   FE
{
    SWGPipeline::~SWGPipeline()
    {
        _pools.clear();
    }

    bool    SWGPipeline::create(const FEGPipeline::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    DSets   SWGPipeline::createDSets()
    {
        return  {};
    }
}

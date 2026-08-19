#include    "SWRenderPass.h"
#include    "SWDevice.h"

namespace   FE
{
    SWRenderPass::~SWRenderPass()
    {
    }

    bool    SWRenderPass::create(const CreateInfo& cInfo)
    {
        _cInfo  =   cInfo;
        return  true;
    }
}

#include    "WGRenderPass.h"

namespace   FE
{
    WGRenderPass::~WGRenderPass()
    {
    }

    bool WGRenderPass::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        return true;
    }
}

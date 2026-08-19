#include    "SWDSetPool.h"
#include    "SWDevice.h"

namespace   FE
{
    SWDSetPool::~SWDSetPool()
    {
    }

    bool    SWDSetPool::create(const FEDSetPool::CreateInfo& cInf)
    {
        _cInfo  =   cInf;
        return  true;
    }
}

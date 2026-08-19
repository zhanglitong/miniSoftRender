#include    "WGSemaphore.h"

namespace   FE
{
    WGSemaphore::~WGSemaphore()
    {
    }

    bool WGSemaphore::create(const CreateInfo& cInfo)
    {
        _cInfo =   cInfo;
        return true;
    }
}

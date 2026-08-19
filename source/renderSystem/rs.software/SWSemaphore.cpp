#include    "SWSemaphore.h"
#include    "SWDevice.h"

namespace   FE
{
    SWSemaphore::~SWSemaphore()
    {
    }

    bool    SWSemaphore::create(const CreateInfo& cInfo)
    {
        (void)cInfo;
        return  true;
    }
}

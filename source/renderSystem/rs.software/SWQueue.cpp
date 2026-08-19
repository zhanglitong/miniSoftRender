#include    "SWQueue.h"
#include    "SWDevice.h"

namespace   FE
{
    SWQueue::~SWQueue()
    {
    }

    bool    SWQueue::submit(uint cnt,const FEQueue::SubmitInfo* pInfo)
    {
        (void)cnt;
        (void)pInfo;
        return  true;
    }
}

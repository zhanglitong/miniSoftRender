#include    "SWFence.h"
#include    "SWDevice.h"

namespace   FE
{
    SWFence::~SWFence()
    {
    }

    void    SWFence::reset()
    {
    }

    void    SWFence::wait(uint64 tm)
    {
        (void)tm;
    }

    bool    SWFence::create(const CreateInfo& cInfo)
    {
        (void)cInfo;
        return  true;
    }
}

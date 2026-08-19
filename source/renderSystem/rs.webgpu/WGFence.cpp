#include    "WGFence.h"

namespace   FE
{
    WGFence::~WGFence()
    {
    }

    bool WGFence::create(const CreateInfo& cInfo)
    {
        _cInfo =   cInfo;
        _signaled =   false;
        return true;
    }

    void WGFence::reset()
    {
        _signaled =   false;
    }

    void WGFence::wait(uint64 tm)
    {
        (void)tm;
        if (!_signaled)
        {
            _signaled =   true;
        }
    }

    void WGFence::signal()
    {
        _signaled =   true;
    }
}

#include    "WGFrameBuffer.h"

namespace   FE
{
    WGFrameBuffer::~WGFrameBuffer()
    {
    }

    bool WGFrameBuffer::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        return true;
    }
}
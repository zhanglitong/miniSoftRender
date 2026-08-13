#include    "WGFrameBuffer.h"

namespace   FE
{
    WGFrameBuffer::~WGFrameBuffer()
    {
    }

    bool    WGFrameBuffer::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        return true;
    }

    bool    WGFrameBuffer::resize(const uint3& dims)
    {
        UNUSED(dims);
        return  false;
    }
}
#include    "SWFrameBuffer.h"
#include    "SWDevice.h"

namespace   FE
{
    SWFrameBuffer::~SWFrameBuffer()
    {
    }

    bool    SWFrameBuffer::create(const FEFrameBuffer::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    bool    SWFrameBuffer::resize(const uint3& )
    {
        return  true;
    }
}

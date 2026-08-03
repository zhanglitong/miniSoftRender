#include    "SWGPUBuffer.h"
#include    "SWDevice.h"

namespace   FE
{
    SWGPUBuffer::~SWGPUBuffer()
    {
        switch(bufferUsage().data())
        {
        case UNIFORM_BUFFER_BIT:
            LOG_DBG("destroy UBO buffer!");
            break;
        case STORAGE_BUFFER_BIT:
            LOG_DBG("destroy SBO buffer!");
            break;
        case INDEX_BUFFER_BIT:
            LOG_DBG("destroy IBO buffer!");
            break;
        case VERTEX_BUFFER_BIT:
            LOG_DBG("destroy VBO buffer!");
            break;
        case INDIRECT_BUFFER_BIT:
            LOG_DBG("destroy ITO buffer!");
            break;
        default:
            LOG_DBG("destroy gpu buffer!");
            break;
        }
    }

    bool    SWGPUBuffer::create(const CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    bool    SWGPUBuffer::update(const void* pData,uint64 length,uint64 offset)
    {
        return  true;
    }

    bool    SWGPUBuffer::resize(uint64 length)
    {
        _cInfo._length  =   length;
        return  true;
    }

    void*   SWGPUBuffer::lock(uint64 size,uint64 offset)
    {
        return  nullptr;
    }

    void    SWGPUBuffer::unlock()
    {
    }
}

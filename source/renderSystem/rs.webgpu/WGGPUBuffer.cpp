#include    "WGGPUBuffer.h"
#include    "WGDevice.h"

namespace   FE
{
    static WGPUBufferUsage getWGPUBufferUsageFlags(BufferUsages usages)
    {
        WGPUBufferUsage flags =   0;
        if (usages.hasFlag(BufferUsage::TRANSFER_SRC_BIT))
            flags |=   WGPUBufferUsage_CopySrc;
        if (usages.hasFlag(BufferUsage::TRANSFER_DST_BIT))
            flags |=   WGPUBufferUsage_CopyDst;
        if (usages.hasFlag(BufferUsage::UNIFORM_BUFFER_BIT))
            flags |=   WGPUBufferUsage_Uniform;
        if (usages.hasFlag(BufferUsage::STORAGE_BUFFER_BIT))
            flags |=   WGPUBufferUsage_Storage;
        if (usages.hasFlag(BufferUsage::INDEX_BUFFER_BIT))
            flags |=   WGPUBufferUsage_Index;
        if (usages.hasFlag(BufferUsage::VERTEX_BUFFER_BIT))
            flags |=   WGPUBufferUsage_Vertex;
        if (usages.hasFlag(BufferUsage::INDIRECT_BUFFER_BIT))
            flags |=   WGPUBufferUsage_Indirect;
        return flags;
    }

    bool createBuffer(WGPUDevice device,const FEGPUBuffer::CreateInfo& info,BufferUsages usage,WGPUBuffer& buffer)
    {
        buffer =   nullptr;
        BufferUsages bufUsages =   info._bufUsages | usage;

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size =   info._length;
        bufferDesc.usage =   getWGPUBufferUsageFlags(bufUsages);
        bufferDesc.mappedAtCreation =   false;

        buffer =   wgpuDeviceCreateBuffer(device,&bufferDesc);

        return buffer != nullptr;
    }

    WGGPUBuffer::~WGGPUBuffer()
    {
        if (_native)
        {
            wgpuBufferDestroy(_native);
        }

        switch (bufferUsage().data())
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

    bool WGGPUBuffer::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        return createBuffer(wgDevice.device(),info,_bufferUsage,_native);
    }

    bool WGGPUBuffer::update(const void* pData,uint64 length,uint64 offset)
    {
        if (!_native || !pData)
            return FEResult::ER_FAILED;

        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto queue = wgDevice.queue();

        wgpuQueueWriteBuffer(queue,_native,offset,pData,length);
        return FEResult::ER_SUCCESS;
    }

    bool WGGPUBuffer::resize(uint64 length)
    {
        if (_native)
        {
            wgpuBufferDestroy(_native);
            _native =   nullptr;
        }
        _cInfo._length =   length;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        return createBuffer(wgDevice.device(),_cInfo,_bufferUsage,_native);
    }

    void* WGGPUBuffer::lock(uint64 size,uint64 offset)
    {
        if (!_native)
            return nullptr;

        if (_mapped)
            return _mappedData;

        WGPUMapMode mapMode =   WGPUMapMode_Read | WGPUMapMode_Write;

        auto mapCallback = [](WGPUMapAsyncStatus status,WGPUStringView message,void* userdata1,void* userdata2) {
            if (status == WGPUMapAsyncStatus_Success)
            {
                auto* buffer = static_cast<WGGPUBuffer*>(userdata1);
                if (buffer)
                {
                    buffer->_mapped =   true;
                }
            }
        };

        WGPUBufferMapCallbackInfo callbackInfo = {};
        callbackInfo.nextInChain =   nullptr;
        callbackInfo.callback =   mapCallback;
        callbackInfo.userdata1 =   this;
        callbackInfo.userdata2 =   nullptr;

        wgpuBufferMapAsync(_native,mapMode,offset,size,callbackInfo);

        void* ptr =   wgpuBufferGetMappedRange(_native,offset,size);
        if (ptr)
        {
            _mapped =   true;
            _mappedData =   ptr;
        }
        return ptr;
    }

    void WGGPUBuffer::unlock()
    {
        if (_native && _mapped)
        {
            wgpuBufferUnmap(_native);
            _mapped =   false;
            _mappedData =   nullptr;
        }
    }
}
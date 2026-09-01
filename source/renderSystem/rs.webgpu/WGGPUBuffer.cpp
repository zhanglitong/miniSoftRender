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

    bool    createBuffer(WGPUDevice device,const FEGPUBuffer::CreateInfo& info,BufferUsages usage,WGPUBuffer& buffer)
    {
        buffer                  =   nullptr;
        BufferUsages bufUsages  =   info._bufUsages | usage;

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size             =   info._length;
        /// 与 Vulkan 后端 (VKGPUBuffer::createBuffer) 对齐:
        /// Vulkan 无条件为所有 buffer 追加 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        /// 使得 VBO/ITO 等也能在 compute shader 中以 storage buffer 形式绑定。
        /// WebGPU 同样追加 WGPUBufferUsage_Storage,保证两个后端行为一致。
        bufferDesc.usage            =   getWGPUBufferUsageFlags(bufUsages) | WGPUBufferUsage_Storage;
        /// 不再使用 mappedAtCreation,因为 WGGPUBuffer 通过 staging + wgpuQueueWriteBuffer
        /// 完成 lock/unlock 模式的更新,避免 WebGPU 一次性映射的限制与 queue 提交时 buffer 仍处 mapped 的验证错误。
        bufferDesc.mappedAtCreation =   false;
        buffer                      =   wgpuDeviceCreateBuffer(device,&bufferDesc);

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

    bool    WGGPUBuffer::create(const CreateInfo& info)
    {
        _cInfo              =   info;
        auto&   wgDevice    =   static_cast<WGDevice&>(_ctx.device());
        _staging.resize(info._length);
        return  createBuffer(wgDevice.device(),info,_bufferUsage,_native);
    }

    bool    WGGPUBuffer::update(const void* pData,uint64 length,uint64 offset)
    {
        if (!_native || !pData)
            return FEResult::ER_FAILED;

        auto&   wgDevice    =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto    queue       =   wgDevice.queue();
        wgpuQueueWriteBuffer(queue,_native,offset,pData,length);
        return FEResult::ER_SUCCESS;
    }

    bool    WGGPUBuffer::resize(uint64 length)
    {
        if (_native)
        {
            wgpuBufferDestroy(_native);
            _native =   nullptr;
        }
        _cInfo._length      =   length;
        _staging.resize(length);
        auto&   wgDevice    =   (WGDevice&)(_ctx.device());
        return  createBuffer(wgDevice.device(),_cInfo,_bufferUsage,_native);
    }

    void*   WGGPUBuffer::lock(uint64 size,uint64 offset)
    {
        if (!_native)
            return nullptr;
        if (offset + size > _cInfo._length)
            return nullptr;
        _lockedOffset   =   offset;
        _lockedLength   =   size;
        return  _staging.data() + offset;
    }

    bool    WGGPUBuffer::flush(uint64 length,uint64 offset)
    {
        UNUSED(length,offset);
        return  true;
    }

    void    WGGPUBuffer::unlock()
    {
        if (!_native || _lockedLength == 0)
            return;
        auto&   wgDevice    =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto    queue       =   wgDevice.queue();
        wgpuQueueWriteBuffer(queue,_native,_lockedOffset,_staging.data() + _lockedOffset,_lockedLength);
        _lockedOffset   =   0;
        _lockedLength   =   0;
    }
}

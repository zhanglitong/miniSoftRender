
#include    "VKGPUBuffer.h"
#include    "VKDevice.h"
#include    "FEVulkan.h"

namespace   FE
{
    bool    createBuffer(FEDevice& dev,const FEGPUBuffer::CreateInfo& info,BufferUsages usage,VkBuffer& buffer,VkDeviceMemory& memory)
    {
        auto&       vkDevice    =   (VKDevice&)dev;
        auto        device      =   vkDevice.logicalDevice();
                    buffer      =   nullptr;
                    memory      =   nullptr;
        VkMemoryRequirements    memReqs =   {};

        VkBufferCreateInfo      bufferInfo{};
        VkMemoryAllocateInfo    allocInfo{};
        BufferUsages            bufUsages   =   info._bufUsages | usage;

        allocInfo.sType             =   VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext             =   nullptr;
        allocInfo.allocationSize    =   0;
        allocInfo.memoryTypeIndex   =   0;
        bufferInfo.sType            =   VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size             =   info._length;
        bufferInfo.usage            =   system2Native(bufUsages) | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        vkGetBufferMemoryRequirements(device, buffer, &memReqs);
        allocInfo.allocationSize        =   memReqs.size;
        allocInfo.memoryTypeIndex       =   vkDevice.getMemoryTypeIndex(memReqs.memoryTypeBits, system2Native(info._memUsages));
      
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));

        return  memory != nullptr;
    }


    VKGPUBuffer::~VKGPUBuffer()
    {
        if (_native)
        {
            auto&       vkDevice    =   (VKDevice&)_ctx.device();
            auto        device      =   vkDevice.logicalDevice();
            vkDestroyBuffer(device, _native, nullptr);
            vkFreeMemory(device,    _memory, nullptr);
        }
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


    bool    VKGPUBuffer::create(const CreateInfo& info)
    {
        _cInfo  =   info;
        return  createBuffer(_ctx.device(),info,_bufferUsage,_native,_memory);
    }

    bool    VKGPUBuffer::update(const void* pData,uint64 length,uint64 offset)
    {
        switch(_cInfo._memUsages.data())
        {
        case MemoryUsage::HOST_VISIBLE_BIT  :
        case MemoryUsage::HOST_COHERENT_BIT :
        case MemoryUsage::HOST_CACHED_BIT   :
            {
                void*   ptr =   lock(length,offset);
                if (ptr == nullptr)
                    return  FEResult::ER_FAILED;
                memcpy(ptr,pData,length);
                unlock();
                return  FEResult::ER_SUCCESS;
            }
        case MemoryUsage::DEVICE_LOCAL_BIT      :
        case MemoryUsage::LAZILY_ALLOCATED_BIT  :
        case MemoryUsage::DEVICE_DEFAULT_BIT    :
            {
                auto    cmdPool =   _ctx.device().transferCmdPool();
                assert (cmdPool != nullptr);
                if (cmdPool != nullptr)
                {
                    /// 这里注意: 是一个临时对象，属性与被更新对象一致，只是数据在主机内存中
                    /// 必须在 cmd 执行完成后是否，否则会有问题
                    GPUBuffer   srcBuf  =   _ctx.device().createFromBuffer(pData,length);
                    CMDPtr      cmd     =   cmdPool->createCmd();
                    cmd->begin(true);
                    cmd->copyBuffer(srcBuf,this,length,0,offset);
                    cmd->end();

                    cmd->submit(_ctx.device().queueTransfer());
                    return  FEResult::ER_SUCCESS;
                }
            }
            break;
        }
        assert(0!=0);
        return  FEResult::ER_FAILED;
    }

    bool    VKGPUBuffer::resize(uint64 length) 
    {
        if (_native)
        {
            auto&       vkDevice    =   (VKDevice&)_ctx.device();
            auto        device      =   vkDevice.logicalDevice();
            vkDestroyBuffer(device, _native, nullptr);
            vkFreeMemory(device,    _memory, nullptr);

            _native =   nullptr;
            _memory =   nullptr;
        }
        _cInfo._length  =   length;
        return  createBuffer(_ctx.device(),_cInfo,_bufferUsage,_native,_memory);
    }
    void*   VKGPUBuffer::lock(uint64 size,uint64 offset)
    {
        auto&           vkDevice    =   (VKDevice&)_ctx.device();
        auto            device      =   vkDevice.logicalDevice();
        void*           ptr     =   nullptr;
        VkDeviceSize    devSize =   size == 0 ?  _cInfo._length - offset :size;
        auto            result  =   vkMapMemory(device, _memory, offset, devSize, 0, &ptr);
        return          result == VK_SUCCESS ? ptr : nullptr;
    }
    void    VKGPUBuffer::unlock()
    {
        auto&       vkDevice    =   (VKDevice&)_ctx.device();
        auto        device      =   vkDevice.logicalDevice();
        vkUnmapMemory(device,  _memory);
    }
}

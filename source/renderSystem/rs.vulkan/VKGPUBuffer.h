
#include    "graphic/FEGPUBuffer.h"
#include    "FEVulkan.h"

namespace   FE
{
    extern   bool    createBuffer(FEDevice& dev,const FEGPUBuffer::CreateInfo& info,BufferUsage usage,VkBuffer& buffer,VkDeviceMemory& memory);

    class   VKGPUBuffer :public TRSObject<VkBuffer,FEGPUBuffer>
    {
    public:
        VKGPUBuffer(FEContext& ctx,BufferUsages usage)
            : TRSObject<VkBuffer,FEGPUBuffer>(ctx,nullptr,usage)
        {
        }
        VKGPUBuffer(const VKGPUBuffer& other)
            : TRSObject<VkBuffer,FEGPUBuffer>(other)
        {}
        virtual ~VKGPUBuffer();

        virtual bool    create(const CreateInfo& info) override;

        virtual bool    update(const void* data,uint64 length,uint64 offset) override;
        virtual bool    resize(uint64 length) override;
        virtual void*   lock(uint64 length,uint64 offset = 0) override;
        virtual void    unlock() override;
    protected:
        VkDeviceMemory  _memory =   nullptr;
    };
}

#pragma     once

#include    "graphic/FEGPUBuffer.h"
#include    "wgpu.h"

namespace   FE
{
    extern bool createBuffer(WGPUDevice device,const FEGPUBuffer::CreateInfo& info,BufferUsages usage,WGPUBuffer& buffer);

    class   WGGPUBuffer :public TRSObject<WGPUBuffer, FEGPUBuffer>
    {
    public:
        WGGPUBuffer(FEContext& ctx,BufferUsages usage)
            :TRSObject<WGPUBuffer, FEGPUBuffer>(ctx,nullptr,usage)
        {
        }
        WGGPUBuffer(const WGGPUBuffer& other)
            :TRSObject<WGPUBuffer, FEGPUBuffer>(other)
        {}
        virtual ~WGGPUBuffer();

        virtual bool    create(const CreateInfo& info) override;

        virtual bool    update(const void* data,uint64 length,uint64 offset) override;
        virtual bool    resize(uint64 length) override;
        virtual void*   lock(uint64 length,uint64 offset = 0) override;
        virtual bool    flush(uint64 length,uint64 offset)  override;
        virtual void    unlock() override;
    };
}

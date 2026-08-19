#pragma     once

#include    "graphic/FEGPUBuffer.h"

namespace   FE
{
    class   SWGPUBuffer :public TRSObject<void*,FEGPUBuffer>
    {
    public:
        SWGPUBuffer(FEContext& ctx,BufferUsages usage)
            : TRSObject<void*,FEGPUBuffer>(ctx,nullptr,usage)
        {
        }
        SWGPUBuffer(const SWGPUBuffer& other)
            : TRSObject<void*,FEGPUBuffer>(other)
        {}
        virtual ~SWGPUBuffer();

        virtual bool    create(const CreateInfo& info) override;

        virtual bool    update(const void* data,uint64 length,uint64 offset) override;
        virtual bool    resize(uint64 length) override;
        virtual void*   lock(uint64 length,uint64 offset = 0) override;
        virtual void    unlock() override;
    };
}

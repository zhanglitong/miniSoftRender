#pragma     once

#include    "graphic/FEGPUBuffer.h"
#include    "wgpu.h"
#include    <vector>

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
    private:
        /// CPU 端 staging 缓冲,用于 lock/unlock 模式
        /// WebGPU 的 buffer 只能在创建时 mapped 一次,无法重复 map,
        /// 因此 lock 返回 staging 指针,unlock 通过 wgpuQueueWriteBuffer 写回 GPU buffer。
        std::vector<uint8_t>    _staging;
        uint64                  _lockedOffset    =   0;
        uint64                  _lockedLength    =   0;
    };
}

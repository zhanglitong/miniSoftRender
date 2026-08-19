#pragma     once
#include    "../FEBuffer.hpp"
#include    "RSObject.h"
#include    "FEGraphicEnums.h"
namespace   FE
{
    class   FEGPUBuffer :public RSObject
    {
    public:
        struct  CreateInfo
        {
            uint64          _length     =   0;
            MemoryUsages    _memUsages  =   DEVICE_DEFAULT_BIT;
            BufferUsages    _bufUsages  =   TRANSFER_DST_BIT | TRANSFER_SRC_BIT ;
        };
    public:
        FEGPUBuffer(FEContext& ctx,BufferUsages usage= BufferUsage::VERTEX_BUFFER_BIT)
            :RSObject(ctx)
            ,_bufferUsage(usage)
        {}

        FEGPUBuffer(const FEGPUBuffer& other)
            :RSObject(other)
        {
            _cInfo          =   other._cInfo;
            _bufferUsage    =   other._bufferUsage;
        }
    public:
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }

        virtual bool    create(const CreateInfo& info)  =   0;
        virtual bool    update(const void* data,uint64 length,uint64 offset)  =   0;
        virtual bool    resize(uint64 length)   =   0;
        virtual void*   lock(uint64 length,uint64 offset)   =   0;
        virtual void    unlock()    =   0;
        /// <summary>
        /// 模板方法，方便使用
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="length"></param>
        /// <param name="offset"></param>
        /// <returns></returns>
        template<typename TValue>
        inline  TValue* lockAs(uint64 length,uint64 offset)
        {
            return  (TValue*)lock(length,offset);
        }
        /// <summary>
        /// 获取主用途
        /// </summary>
        /// <returns></returns>
        inline  auto    bufferUsage() const
        {
            return  _bufferUsage;
        }

    protected:
        CreateInfo      _cInfo;
        /// <summary>
        /// 缓冲区主要用途
        /// </summary>
        BufferUsages    _bufferUsage;
    };

    using   GPUBuffer   =   SharedPtr<FEGPUBuffer>;

    using   VBO         =   GPUBuffer;
    using   IBO         =   GPUBuffer;
    using   UBO         =   GPUBuffer;
    using   ITO         =   GPUBuffer;
    using   SBO         =   GPUBuffer;

    using   VBOs        =   std::vector<VBO>;
    using   IBOs        =   std::vector<IBO>;
    using   UBOs        =   std::vector<UBO>;
    using   ITOs        =   std::vector<ITO>;
    using   SBOs        =   std::vector<SBO>;
}

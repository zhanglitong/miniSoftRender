#pragma     once

#include    "FEDefine.h"
#include    "FEObject.h"
#include    "FEMath.hpp"
#include    "FEVector.hpp"
#include    "FEResult.hpp"
#include    "stream/FEReader.hpp"
#include    "stream/FEWriter.hpp"
#include    "stream/FEReaderFile.hpp"
#include    "FENotify.hpp"


namespace   FE
{
    
    DEFINE_CLASS_UUID(FEBuffer,"{8C83A851-2A7C-4E26-AE03-83F554D1EB92}");
    class   FE_API  FEBuffer :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEBuffer)
    public:
        using   Buffer  =   SharedPtr<FEBuffer>;
    public:
        struct  CreateInfo 
        {
            uint8s  _buffer;
            /// <summary>
            /// 数据的起始偏移量
            /// </summary>
            uint64  _offset =   0;
            /// <summary>
            /// 引用数据的长度
            /// </summary>
            uint64  _range  =   ~0ULL;

            const   auto    dataPtr() const
            {
                return  _buffer.data() +  _offset;
            }
            inline  auto    dataPtr()
            {
                return  _buffer.data() +  _offset;
            }
            inline  auto    data()
            {
                return  _buffer.data() +  _offset;
            }
            inline  auto    data() const
            {
                return  _buffer.data() +  _offset;
            }
            inline  auto    length() const
            {
                return  _range == ~0ULL ? _buffer.size() - _offset : _range;
            }
        };
    public:
        FEBuffer(FEContext& ctx)
            :FEObject(ctx)
        {}

        FEBuffer(const FEBuffer& other)
            :FEObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        virtual ~FEBuffer() =   default;
        /// <summary>
        /// 获取创建信息(读写)
        /// </summary>
        /// <returns></returns>
        inline  auto&       cInfo()
        {
            return  _cInfo;
        }
        /// <summary>
        /// 获取创建信息(只读方式)
        /// </summary>
        /// <returns></returns>
        const   auto&       cInfo() const
        {
            return  _cInfo;
        }
        const   uint64      length() const
        {
            return  _cInfo.length();
        }
        const   uint8*      data() const
        {
            return  _cInfo.dataPtr();
        }
        inline  uint8*      data()
        {
            return  _cInfo.dataPtr();
        }
        template<typename TAsTo>
        const   TAsTo*      dataAs() const
        {
            return  (const TAsTo*)_cInfo.dataPtr();
        }
        template<typename TAsTo>
        inline  TAsTo*      dataAs()
        {
            return   (TAsTo*)_cInfo.dataPtr();
        }
        template<typename U>
        inline  auto&       set(std::initializer_list<U> vlist,const Notify& notify = {})
        {
            _buffer =   vlist;
            if (notify) notify(*this);
            return  *this;
        }
        
        /// <summary>
        /// 调用后，buffer内存管理权 移交给_buffer
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="buffer"></param>
        /// <returns></returns>
        template<class U>   
        inline  auto&       referenceFrom(FEVector<U>& buffer)
        {
            _cInfo._buffer.referenceFrom(buffer);
            return  *this;
        }
        /// <summary>
        /// 执行内存拷贝动作
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="buffer"></param>
        /// <returns></returns>
        template<class U>   
        inline  auto&       copyFrom(const FEVector<U>& buffer)
        {
            _cInfo._buffer.copyFrom(buffer);
            return  *this;
        }
        /// <summary>
        /// 创建
        /// </summary>
        /// <param name="infor"></param>
        /// <returns></returns>
        virtual FEResult    create(const CreateInfo& infor) 
        {
            _cInfo  =   infor;
            return  FEResult::ER_SUCCESS;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& ,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf&,uint version,FESerializeCtx& ctx) override;
    protected:
        CreateInfo  _cInfo;
    public:
        /// <summary>
        /// 加载文件
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="fileName"></param>
        /// <returns></returns>
        static  Buffer     loadFile(FEContext& ctx,const char* fileName,const char* mode = "rb")
        {
            FEReaderFile    reader(ctx,fileName,mode);
            if (!reader.isValid())
                return  nullptr;
            auto        len     =   reader.length();
            Buffer      buf     =   new FEBuffer(ctx);
            auto&       cInf    =   buf->cInfo();
            cInf._buffer.resize(len);
            reader.readBuffer(cInf._buffer.data(),len);
            reader.close();
            return      buf;
        }
    };
    using   BufferCreateInfo    =   FEBuffer::CreateInfo;
    using   Buffer              =   SharedPtr<FEBuffer>;
    using   Buffers             =   TVector<Buffer>;
    using   BufferPtr           =   SharedPtr<FEBuffer>;


}

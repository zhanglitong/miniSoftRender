#pragma     once

#include    <cstring>
#include    "FEBuffer.hpp"
#include    "stream/FEReaderFile.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEFileBuffer,"{9E4F2C6A-7B18-4D5A-8C3E-2A61F0B94D57}");
    /// <summary>
    /// 带文件名的内存缓冲:保存图片/模型等资源的原始(压缩)文件字节。
    /// 与 FEBuffer 一样按字节序列化,额外记录来源文件名,便于调试与重新关联原文件。
    /// 纹理把它作为 source 引用,工程文件里只保存这份压缩数据,不再克隆展开后的像素。
    /// </summary>
    class   FE_API  FEFileBuffer :public FEBuffer
    {
        IMPLEMENT_CLASS_REFLECT(FEFileBuffer)
    public:
        using   Ptr =   SharedPtr<FEFileBuffer>;
    public:
        FEFileBuffer(FEContext& ctx)
            :FEBuffer(ctx)
        {}
        FEFileBuffer(const FEFileBuffer& other)
            :FEBuffer(other)
            ,_fileName(other._fileName)
        {}
        /// <summary>
        /// 来源文件名(完整路径或内嵌资源的标识名)
        /// </summary>
        auto&   fileName() const
        {
            return  _fileName;
        }
        void    setFileName(const String& fileName)
        {
            _fileName   =   fileName;
        }
    protected:
        /// <summary>
        /// 先按基类写字节,再写文件名长度 + 文件名
        /// </summary>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override
        {
            FEBuffer::serializeTraits(writer,chunk,version,ctx);
            writer.write(_fileName);
        }
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override
        {
            FEBuffer::deserializeTraits(reader,chunk,version,ctx);
            reader.read(_fileName);
        }
    
    public:
        /// <summary>
        /// 从文件读取原始字节
        /// </summary>
        /// <param name="fileName">文件路径</param>
        /// <returns>失败返回 nullptr</returns>
        static  Ptr     loadFile(FEContext& ctx,const char* fileName)
        {
            if (fileName == nullptr)
                return  nullptr;
            FEReaderFile    reader(ctx,fileName);
            if (!reader.isValid())
                return  nullptr;
            const   uint64  len     =   reader.length();
            Ptr             buf     =   new FEFileBuffer(ctx);
            auto&           cInf    =   buf->cInfo();
            cInf._buffer.resize(size_t(len));
            reader.readBuffer(cInf._buffer.data(),len);
            reader.close();
            buf->setFileName(fileName);
            return  buf;
        }
        /// <summary>
        /// 从内存字节创建(用于 .glb 内嵌图片的 bufferView 切片等)
        /// </summary>
        static  Ptr     create(FEContext& ctx,const void* data,size_t size,const String& fileName)
        {
            Ptr     buf =   new FEFileBuffer(ctx);
            auto&   cInf    =   buf->cInfo();
            cInf._buffer.resize(size);
            if (size > 0 && data != nullptr)
                memcpy(cInf._buffer.data(),data,size);
            buf->setFileName(fileName);
            return  buf;
        }
    protected:
        /// <summary>
        /// 来源文件名
        /// </summary>
        String      _fileName;
    };
}

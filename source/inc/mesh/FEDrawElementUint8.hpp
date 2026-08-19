#pragma     once

#include    "FEPrimitive.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEDrawElementUint8,"{E08C6535-2B90-4361-9804-B8B6DFC5EB98}");

    class   FEDrawElementUint8 :public FEPrimitive
    {
        IMPLEMENT_CLASS_REFLECT(FEDrawElementUint8)
    public:
        FEDrawElementUint8(FEContext& ctx)
            :FEPrimitive(ctx,DRAW_ELEMENT_UINT8)
        {}
        FEDrawElementUint8(const FEDrawElementUint8& other)
            :FEPrimitive(other)
        {
            _index  =   other._index;
        }
        const   auto&   index() const
        {
            return  _index;
        }
        inline  auto&   index()
        {
            return  _index;
        }
        inline  uint    count() const
        {
            return  uint(_index.size());
        }
        inline  uint    indexOffset() const
        {
            return  _indexOffset;
        }
        inline  void    setIndexOffset(uint offset)
        {
            _indexOffset    =   offset;
        }
        /// <summary>
        /// 返回索引的数量
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="pbuf"></param>
        /// <param name="nSize"></param>
        /// <returns></returns>
        template<typename T>
        inline uint     copyIndexTo(T* pbuf,uint32 nSize) const
        {   
            uint32  count   =   (std::min<uint32>)(nSize,uint(_index.size()));
            for (uint32 i = 0; i < count; i++)
            {
                pbuf[i] =   T(_index[i]);
            }
            return  count;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override
        {
            FEPrimitive::serializeTraits(writer,chk,version,ctx);
            writer.write(_index);
        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chk">数据头，子类可根据chk._flags字段读取控制</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx) override
        {
            FEPrimitive::deserializeTraits(reader,chk,version,ctx);
            reader.read(_index);
        }
    protected:
        uint8s      _index;
        uint        _indexOffset    =   0;
    };
}


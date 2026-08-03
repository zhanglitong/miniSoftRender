#pragma     once

#include    "FEPrimitive.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEDrawArray,"{F609F55D-D34A-4C42-BAEF-46CE1701714C}");

    class   FEDrawArray :public FEPrimitive
    {
        IMPLEMENT_CLASS_REFLECT(FEDrawArray)
    public:
        FEDrawArray(FEContext& ctx)
            :FEPrimitive(ctx,DRAW_ARRAY)
        {}
        FEDrawArray(const FEDrawArray& other)
            :FEPrimitive(other)
        {
           _start   =   other._start;
           _count   =   other._count;
        }
        inline  uint32  start() const
        {
            return  _start;
        }
        inline  uint32  count() const
        {
            return  _count;
        }
        inline  auto&   setStart(uint32 start)
        {
            _start  =   start;
            return  *this;
        }
        inline  auto&   setCount(uint32 count)
        {
            _count  =   count;
            return  *this;
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
            writer.write(_start);
            writer.write(_count);
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
            reader.read(_start);
            reader.read(_count);
        }
    protected:
        uint32  _start  =   0;
        uint32  _count  =   0;
    };


    using   DrawArray   =   SharedPtr<FEDrawArray>;
}


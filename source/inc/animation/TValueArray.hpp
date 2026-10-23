#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"
#include    "../FEUserObject.hpp"
#include    "../FEWriterHelper.hpp"
#include    "../FEReaderHelper.hpp"

namespace FE
{   
    struct  FrameValue
    {
        real    _t  =   0;
        KFValue _v  =   {};
    };
    using   FrameValues     =   std::vector<FrameValue>;
    /// <summary>
    /// 封装数组对象
    /// </summary>
    /// <typeparam name="TValue"></typeparam>
    template<typename TValue>
    class  TValueArray 
        : public FEObject
        , public FEUserObject
    {
    public:
        using   ValueType   =   TValue;
        using   Values      =   std::vector<TValue>;
    public:
        TValueArray(FEContext& ctx)
            :FEObject(ctx)
        {}
        TValueArray(const TValueArray & other)
            :FEObject(other)
        {
            _values     =   other._values;
        }
        ~TValueArray()   =   default;

        Values&         values()
        {
            return  _values;
        }
        const Values&   values() const
        {
            return  _values;
        }

        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override
        {
            UNUSED(writer, chunk, version, ctx);
            uint   cnt = (uint)_values.size();
            writer.write(cnt);
            writer.writeBuffer(_values.data(), sizeof(TValue) * cnt);
        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chunk">数据头，子类可根据chunk._flags字段控制读取</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override
        {   
            UNUSED(reader, chunk, version, ctx);
            uint    cnt = 0;
            reader.read(cnt);
            _values.resize(cnt);
            reader.readBuffer(_values.data(), sizeof(TValue) * cnt);
        }
    protected:
        Values  _values;
    };

    /// <summary>
    /// 声明一个值数组对象,用于替代重复的 TValueArray 空壳子类
    /// </summary>
    /// <param name="ObjectClass">类名</param>
    /// <param name="AliasName">智能指针别名</param>
    /// <param name="ValueT">元素类型</param>
    /// <param name="uuid">类UUID,不要修改</param>
    #define FE_DEFINE_VALUE_ARRAY_OBJECT(ObjectClass, AliasName, ValueT, uuid) \
        DEFINE_CLASS_UUID(ObjectClass, uuid);                                  \
        class   FE_API  ObjectClass :public TValueArray<ValueT>                \
        {                                                                      \
            IMPLEMENT_CLASS_REFLECT(ObjectClass)                               \
        public:                                                                \
            ObjectClass(FEContext& ctx)                                        \
                :TValueArray(ctx)                                              \
            {}                                                                 \
            ObjectClass(const ObjectClass& other)                              \
                :TValueArray(other)                                            \
            {}                                                                 \
            ~ObjectClass()  =   default;                                       \
        };                                                                     \
        using   AliasName   =   SharedPtr<ObjectClass>
}

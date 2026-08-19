#pragma     once

#include    "FEObject.h"
#include    "FEKeyValues.hpp"
#include    "FEReaderHelper.hpp"
#include    "FEWriterHelper.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEProperty,"{92364CA0-EC58-448E-9A16-534A5B2C181C}");

    class   FEProperty :public FEObject
    {
    public:
        /// <summary>
        /// 属性值的类型，目前只有如下集中类型
        /// </summary>
        using   Value       =   std::variant<   std::monostate,int64,real
                                                ,String,Strings
                                                ,uint8s,reals
                                                ,Rgba8
                                                ,FEUuid>;
        /// <summary>
        /// 变更通知
        /// </summary>
        using   NChanged    =   std::function<void(const Value& old,const Value&)>;
        class   ValueObject
        {
        public:
            ValueObject()
            {}
            ValueObject(const Value& value)
            {
                _value  =   value;
            }
            ValueObject(Value&& value)
            {
                _value  =   std::move(value);
            }
            /// <summary>
            /// 获取值
            /// </summary>
            /// <returns></returns>
            const   Value&  value() const
            {
                return  _value;
            }
            /// <summary>
            /// 是否有效
            /// </summary>
            /// <returns></returns>
            inline  bool    isValid() const
            {
                return  !std::holds_alternative<std::monostate>(_value);
            }
            /// <summary>
            /// 设置值
            /// </summary>
            /// <param name="value"></param>
            /// <returns></returns>
            inline   auto&  setValue(const Value& value)
            {
                if (_value != value)
                {
                    if(_notify) _notify(_value,value);
                    _value  =   value;
                }
                return  *this;
            }
        protected:
            Value       _value;
            NChanged    _notify;
        };
        using   KeyValues   =   FEKeyValues<String,ValueObject>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEProperty)
    public:
        FEProperty(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEProperty(const FEProperty& other)
            :FEObject(other)
        {}

        template<typename TValue>
        inline  auto&       setValue(const String& name,const TValue& val)
        {
            static_assert(is_any_type(TValue,String,Strings,uint8s,reals,Rgba8,FEUuid));
            ValueObject*    pVal    =   _values.isExist(name);
            if (pVal != nullptr)
            {
                pVal->setValue(val)
                return  *pVal;
            }
            auto&       kvMap   =   _values.keyValues();
            ValueObject vObject =   val;
            return  kvMap.insert({key,value}).first->second;
        }
        template<typename TValue>
        inline  auto&       setValue(String&& name,TValue&& val)
        {
            ValueObject*    pVal    =   _values.isExist(name);
            if (pVal != nullptr)
            {
                pVal->setValue(val);
                return  *pVal;
            }
            auto&       kvMap   =   _values.data();
            ValueObject vObject =   val;
            return  kvMap.insert({std::move(name),std::move(vObject)}).first->second;
        }
        const   KeyValues&  values() const
        {
            return  _values;
        }
        inline  KeyValues&  values()
        {
            return  _values;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override
        {
            (void)ctx;
            (void)version;
            (void)chk;
            const   auto&   keyVals =   _values.data();
            const   uint    cnt     =   (uint)keyVals.size();
            writer.write(cnt);
            for (auto& var : keyVals)
            {
                uint    index   =   (uint)var.second.value().index();
                writer.write(index);
                writer.write(var.first);
                switch(index)
                {
                case 1:     writer.write(std::get<int64>(var.second.value()));
                    break;
                case 2:     writer.write(std::get<double>(var.second.value()));
                    break;
                case 3:     writer.write(std::get<String>(var.second.value()));
                    break;
                case 4:     writer.write(std::get<Strings>(var.second.value()));
                    break;
                case 5:     writer.write(std::get<uint8s>(var.second.value()));
                    break;
                case 6:     writer.write(std::get<reals>(var.second.value()));
                    break;
                case 7:     writer.write(std::get<Rgba8>(var.second.value()));
                    break;
                case 8:     writer.write(std::get<FEUuid>(var.second.value()));
                    break;
                default:
                    break;
                }
            }
        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx) override
        {
            (void)chk;
            (void)ctx;
            (void)version;
            uint    cnt     =   0;
            uint    index   =   0;
            String  key;
            reader.read(cnt);
            for (uint i = 0 ;i < cnt; ++ i)
            {   
                reader.read(index);
                reader.read(key);
                ValueObject object;
                Value       value;
                switch(index)
                {
                case 1:     value   =   reader.readValue<int64>();   
                    break;
                case 2:     value   =   reader.readValue<real>();   
                    break;
                case 3:     value   =   reader.readValue<String>();  
                    break;
                case 4:     value   =   reader.readValue<Strings>();  
                    break;
                case 5:     value   =   reader.readValue<uint8s>();
                    break;
                case 6:     value   =   reader.readValue<reals>();
                    break;
                case 7:     value   =   reader.readValue<Rgba8>();
                    break;
                case 8:     value   =   reader.readValue<FEUuid>();
                    break;
                default:
                    break;
                }
                object.setValue(value);
                _values.add(key,object);
            }
        }
    protected:
        KeyValues   _values;


    };

    using   Property    =   SharedPtr<FEProperty>;
    using   Propertys   =   FEVector<Object>;
   
}


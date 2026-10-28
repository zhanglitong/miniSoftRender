/// <summary>
/// FESettingPropertyModel
/// 把 FESetting 树适配为通用属性控件(FEPropertyView)的数据源。
/// 控件只认 FEPropertyModel 接口,FESetting 只是其中一种实现。
/// </summary>

#pragma     once

#include    "FEPropertyView.hpp"
#include    "FESetting.hpp"

#include    <string>
#include    <type_traits>
#include    <variant>

namespace   FE
{
    /// <summary>
    /// FESetting → FEPropertyModel 适配器
    /// </summary>
    class   FESettingPropertyModel : public FEPropertyModel
    {
    public:
        explicit    FESettingPropertyModel(FESetting& setting)
            :   _setting(setting)
        {
        }
    public:
        size_t  childCount(Node parent) const override
        {
            const   FESetting::Item*    item    =   itemOf(parent);
            return  item != nullptr ? item->children().size() : 0;
        }
        Node    child(Node parent,size_t index) const override
        {
            const   FESetting::Item*    item    =   itemOf(parent);
            if (item == nullptr || index >= item->children().size())
                return  nullptr;
            return  item->children()[index].get();
        }
        std::string name(Node node) const override
        {
            const   FESetting::Item*    item    =   itemOf(node);
            return  item != nullptr ? item->name() : std::string();
        }
        FEPropertyValue value(Node node) const override
        {
            FEPropertyValue result;
            /// 根节点自身没有值,只展示子节点
            const   FESetting::Item*    item    =   itemOf(node);
            if (item == nullptr)
                return  result;

            std::visit([&result](const auto& arg)
            {
                using   T   =   std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T,std::monostate>)
                {
                }
                else if constexpr (std::is_same_v<T,bool>)
                {
                    result._type    =   FEPropertyValue::Type::Bool;
                    result._bool    =   arg;
                }
                else if constexpr (std::is_same_v<T,String>)
                {
                    result._type    =   FEPropertyValue::Type::String;
                    result._string  =   arg;
                }
                else if constexpr (std::is_same_v<T,int32>)
                {
                    result._type    =   FEPropertyValue::Type::Int32;
                    result._int     =   arg;
                }
                else if constexpr (std::is_same_v<T,uint32>)
                {
                    result._type    =   FEPropertyValue::Type::UInt32;
                    result._uint    =   arg;
                }
                else if constexpr (std::is_same_v<T,int64>)
                {
                    result._type    =   FEPropertyValue::Type::Int64;
                    result._int     =   arg;
                }
                else if constexpr (std::is_same_v<T,uint64>)
                {
                    result._type    =   FEPropertyValue::Type::UInt64;
                    result._uint    =   arg;
                }
                else if constexpr (std::is_same_v<T,float>)
                {
                    result._type    =   FEPropertyValue::Type::Float;
                    result._real    =   arg;
                }
                else if constexpr (std::is_same_v<T,double>)
                {
                    result._type    =   FEPropertyValue::Type::Double;
                    result._real    =   arg;
                }
                else
                {
                    /// 剩余类型都是 glm 向量
                    result._type    =   vecType<T>();
                    for (int i = 0; i < (int)T::length(); ++i)
                        result._vec[i]  =   (double)arg[i];
                }
            },item->raw());
            return  result;
        }
        bool    setValue(Node node,const FEPropertyValue& value) override
        {
            FESetting::Item*    item    =   const_cast<FESetting::Item*>(itemOf(node));
            if (item == nullptr)
                return  false;

            using   Type    =   FEPropertyValue::Type;
            switch (value._type)
            {
            case Type::Bool:
                item->set<bool>(value._bool);
                return  true;
            case Type::Int32:
                item->set<int32>((int32)value._int);
                return  true;
            case Type::UInt32:
                item->set<uint32>((uint32)value._uint);
                return  true;
            case Type::Int64:
                item->set<int64>(value._int);
                return  true;
            case Type::UInt64:
                item->set<uint64>(value._uint);
                return  true;
            case Type::Float:
                item->set<float>((float)value._real);
                return  true;
            case Type::Double:
                item->set<double>(value._real);
                return  true;
            case Type::String:
                item->set(value._string);
                return  true;
            case Type::Int2:
                item->set<int2>(int2((int32)value._vec[0],(int32)value._vec[1]));
                return  true;
            case Type::Int3:
                item->set<int3>(int3((int32)value._vec[0],(int32)value._vec[1],(int32)value._vec[2]));
                return  true;
            case Type::Int4:
                item->set<int4>(int4((int32)value._vec[0],(int32)value._vec[1],(int32)value._vec[2]
                                    ,(int32)value._vec[3]));
                return  true;
            case Type::UInt2:
                item->set<uint2>(uint2((uint32)value._vec[0],(uint32)value._vec[1]));
                return  true;
            case Type::UInt3:
                item->set<uint3>(uint3((uint32)value._vec[0],(uint32)value._vec[1],(uint32)value._vec[2]));
                return  true;
            case Type::UInt4:
                item->set<uint4>(uint4((uint32)value._vec[0],(uint32)value._vec[1],(uint32)value._vec[2]
                                      ,(uint32)value._vec[3]));
                return  true;
            case Type::Float2:
                item->set<float2>(float2((float)value._vec[0],(float)value._vec[1]));
                return  true;
            case Type::Float3:
                item->set<float3>(float3((float)value._vec[0],(float)value._vec[1],(float)value._vec[2]));
                return  true;
            case Type::Float4:
                item->set<float4>(float4((float)value._vec[0],(float)value._vec[1],(float)value._vec[2]
                                        ,(float)value._vec[3]));
                return  true;
            case Type::Real2:
                item->set<real2>(real2(value._vec[0],value._vec[1]));
                return  true;
            case Type::Real3:
                item->set<real3>(real3(value._vec[0],value._vec[1],value._vec[2]));
                return  true;
            case Type::Real4:
                item->set<real4>(real4(value._vec[0],value._vec[1],value._vec[2],value._vec[3]));
                return  true;
            case Type::None:
            default:
                return  false;
            }
        }
    protected:
        /// <summary>
        /// 根节点(nullptr)映射到设置树的根
        /// </summary>
        const   FESetting::Item*    itemOf(Node node) const
        {
            return  node != nullptr ? static_cast<const FESetting::Item*>(node) : _setting.root();
        }
        /// <summary>
        /// glm 向量 → 控件类型
        /// </summary>
        template<class T>
        static  FEPropertyValue::Type   vecType()
        {
            using   Type    =   FEPropertyValue::Type;
            if constexpr (std::is_same_v<T,int2>)       return  Type::Int2;
            else if constexpr (std::is_same_v<T,int3>)  return  Type::Int3;
            else if constexpr (std::is_same_v<T,int4>)  return  Type::Int4;
            else if constexpr (std::is_same_v<T,uint2>) return  Type::UInt2;
            else if constexpr (std::is_same_v<T,uint3>) return  Type::UInt3;
            else if constexpr (std::is_same_v<T,uint4>) return  Type::UInt4;
            else if constexpr (std::is_same_v<T,float2>)return  Type::Float2;
            else if constexpr (std::is_same_v<T,float3>)return  Type::Float3;
            else if constexpr (std::is_same_v<T,float4>)return  Type::Float4;
            else if constexpr (std::is_same_v<T,real2>) return  Type::Real2;
            else if constexpr (std::is_same_v<T,real3>) return  Type::Real3;
            else if constexpr (std::is_same_v<T,real4>) return  Type::Real4;
            else                                        return  Type::None;
        }
    protected:
        FESetting&  _setting;
    };
}

#pragma     once

#include    "FEDefine.h"

namespace   FE
{
    enum    PropertyIndex
    {
        /// <summary>
        /// 位置属性,数据类型是real(double)
        /// </summary>
        PROP_TRANSFORM_X,
        PROP_TRANSFORM_Y,
        PROP_TRANSFORM_Z,
        PROP_TRANSFORM_XY,
        PROP_TRANSFORM_XYZ,
       
        /// <summary>
        /// 缩放属性,数据类型是float
        /// </summary>
        PROP_SCALE_X,
        PROP_SCALE_Y,
        PROP_SCALE_Z,
        PROP_SCALE_XY,
        PROP_SCALE_XYZ,
        /// <summary>
        /// 欧拉角属性,数据类型是float, 单位角度
        /// </summary>
        PROP_ROTATE_X,
        PROP_ROTATE_Y,
        PROP_ROTATE_Z,
        PROP_ROTATE_XYZ,
        // <summary>
        /// 旋转属性,数据类型是 quatf
        /// </summary>
        PROP_QUAT,
        /// <summary>
        /// 颜色属性,数据类型是 float3
        /// </summary>
        PROP_COLOR_RGB,
        /// <summary>
        /// 透明属性,数据类型是 float
        /// </summary>
        PROP_COLOR_ALPHA,
        /// <summary>
        /// 可见性属性,数据类型是 uint8
        /// </summary>
        PROP_VISIBLE,
        /// <summary>
        /// 材质环境光属性,float3
        /// </summary>
        PROP_MAT_DIFFUSE,
    };


    /// <summary>
    /// 关键帧值
    /// </summary>
    using   KFValue =   std::variant<std::monostate,uint8,float,float2,float3,float4,quatf,real,real2,real3,real4,quatr>;

    inline  KFValue operator+(const KFValue& x, const KFValue& y) 
    {
        return std::visit([](auto&& a, auto&& b) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            using   U   =   std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<U, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
            {
                using   Common  =   std::common_type_t<T, U>;
                return  KFValue{Common(a + b)};
            }
            else if constexpr (!std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
                return KFValue{a + T::value_type(b)};
            else if constexpr (std::is_arithmetic_v<T> && !std::is_arithmetic_v<U>) 
                return KFValue{U::value_type(a) + b};
            else if constexpr (std::is_same_v<U,T>) 
                return KFValue{a + b};
            else
                return std::monostate{};
        }, x, y);
    }

    inline  KFValue operator-(const KFValue& x, const KFValue& y) 
    {
        return std::visit([](auto&& a, auto&& b) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            using   U   =   std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<U, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
            {
                using   Common  =   std::common_type_t<T, U>;
                return  KFValue{Common(a - b)};
            }
            else if constexpr (!std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
                return KFValue{a - T::value_type(b)};
            else if constexpr (std::is_arithmetic_v<T> && !std::is_arithmetic_v<U>) 
                return KFValue{U::value_type(a) - b};
            else if constexpr (std::is_same_v<U,T>) 
                return KFValue{a - b};
            else
                return std::monostate{};
        }, x, y);
    }
    
    inline  KFValue operator*(const KFValue& x, const KFValue& y) 
    {
        return std::visit([](auto&& a, auto&& b) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            using   U   =   std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<U, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
            {
                using   Common  =   std::common_type_t<T, U>;
                return  KFValue{Common(a * b)};
            }
            else if constexpr (!std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
                return KFValue{a * T::value_type(b)};
            else if constexpr (std::is_arithmetic_v<T> && !std::is_arithmetic_v<U>) 
                return KFValue{U::value_type(a) * b};
            else if constexpr (std::is_same_v<U,T>) 
                return KFValue{a * b};
            else
                return std::monostate{};
        }, x, y);
    }
    template<typename TValue>
    inline  KFValue operator*(const KFValue& x, const TValue& y) 
    {
        return std::visit([&](auto&& a) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T>) 
            {
                using   Common  =   std::common_type_t<T, TValue>;
                return  KFValue{Common(a * y)};
            }
            else if constexpr (std::is_same_v<TValue,T>) 
                return KFValue{a * b};
            else if constexpr (!std::is_arithmetic_v<T>) 
                return KFValue{a * T::value_type(y)};
            else
                return std::monostate{};
        }, x);
    }
    template<typename TValue>
    inline  KFValue operator*(const TValue& y,const KFValue& x) 
    {
        return std::visit([&](auto&& a) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T>) 
            {
                using   Common  =   std::common_type_t<T, TValue>;
                return  KFValue{Common(a * y)};
            }
            else if constexpr (std::is_same_v<TValue,T>) 
                return KFValue{a * b};
            else if constexpr (!std::is_arithmetic_v<T>) 
                return KFValue{a * T::value_type(y)};
            else
                return std::monostate{};
        }, x);
    }


    inline  KFValue operator/(const KFValue& x, const KFValue& y) 
    {
        return std::visit([](auto&& a, auto&& b) -> KFValue 
        {
            using   T   =   std::decay_t<decltype(a)>;
            using   U   =   std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<U, std::monostate>) 
                return std::monostate{};
            else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) 
            {
                using   Common  =   std::common_type_t<T, U>;
                return  KFValue{Common(a / b)};
            }
            else 
                return KFValue{a / b};
        }, x, y);
    }
}



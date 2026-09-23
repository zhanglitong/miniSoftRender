#pragma     once

#include    "FEDefine.h"
#include    "FEMath.hpp"

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
        PROP_TRANSFORM_XYZ,
       
        /// <summary>
        /// 缩放属性,数据类型是float
        /// </summary>
        PROP_SCALE_X,
        PROP_SCALE_Y,
        PROP_SCALE_Z,
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

    enum    NodeProperyBit
    {
        /// <summary>
        /// 有该标记则创建 PROP_TRANSFORM_X 属性track
        /// </summary>
        NP_TRANSFORM_X      =   (1<<0),
        NP_TRANSFORM_Y      =   (1<<1),
        NP_TRANSFORM_Z      =   (1<<2),
        NP_TRANSFORM_XYZ    =   (1<<3),

        NP_SCALE_X          =   (1<<4),
        NP_SCALE_Y          =   (1<<5),
        NP_SCALE_Z          =   (1<<6),
        NP_SCALE_XYZ        =   (1<<7),
        /// <summary>
        /// 欧拉角属性,数据类型是float, 单位角度
        /// </summary>
        NP_ROTATE_X         =   (1<<8),
        NP_ROTATE_Y         =   (1<<9),
        NP_ROTATE_Z         =   (1<<10),
        NP_ROTATE_XYZ       =   (1<<11),
        /// <summary>
        /// 
        /// </summary>
        NP_QUAT             =   (1<<12),
        NP_COLOR_RGB        =   (1<<13),
        NP_COLOR_ALPHA      =   (1<<14),
        NP_VISIBLE          =   (1<<15),
        /// <summary>
        /// 
        /// </summary>
        NP_Default          =   NP_TRANSFORM_XYZ | NP_SCALE_XYZ | NP_ROTATE_XYZ
    };

    using   NodeProperyBits     =   FEFlags<NodeProperyBit,uint32>;
    /// <summary>
    /// 关键帧值
    /// </summary>
    using   KFValue =   std::variant<std::monostate,uint8,float,float2,float3,float4,quatf,real,real2,real3,real4,quatr,uint8x4
                                    ,int,int2,int3,int4,uint,uint2,uint3,uint4>;

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
    template<typename TValue, typename = std::enable_if_t<std::is_arithmetic_v<TValue>>>
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
                return KFValue{a * y};
            else if constexpr (!std::is_arithmetic_v<T>) 
                return KFValue{a * T::value_type(y)};
            else
                return std::monostate{};
        }, x);
    }
    template<typename TValue, typename = std::enable_if_t<std::is_arithmetic_v<TValue>>>
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
                return KFValue{a * y};
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



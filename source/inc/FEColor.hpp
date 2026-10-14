#pragma     once

#include    <utility>
#include    <limits>
#include    <cmath>
#include    "FEDefine.h"
#include    "FEMath.hpp"
#include    "FEConst.h"
#include    "FEFormat.hpp"

#include    "colorFormat/FER4g4Unorm.hpp"
#include    "colorFormat/FER4g4b4a4Unorm.hpp"
#include    "colorFormat/FEB4g4r4a4Unorm.hpp"
#include    "colorFormat/FER5g6b5Unorm.hpp"
#include    "colorFormat/FEB5g6r5Unorm.hpp"
#include    "colorFormat/FER5g5b5a1Unorm.hpp"
#include    "colorFormat/FEB5g5r5a1Unorm.hpp"
#include    "colorFormat/FEA1r5g5b5Unorm.hpp"


#include    "colorFormat/FEA2b10g10r10Snorm.hpp"
#include    "colorFormat/FEA2b10g10r10Unorm.hpp"
#include    "colorFormat/FEA2r10g10b10Snorm.hpp"
#include    "colorFormat/FEA2r10g10b10Unorm.hpp"
#include    "colorFormat/FEA2b10g10r10Sint.hpp"
#include    "colorFormat/FEA2b10g10r10Uint.hpp"
#include    "colorFormat/FEA2b10g10r10Sscaled.hpp"
#include    "colorFormat/FEA2b10g10r10Uscaled.hpp"

#include    "colorFormat/FER9g9b9a5Unorm.hpp"


namespace   FE
{
    template<typename Type,FEFormat _fmt>
    class   TColor
    {
    public:
        using   TypeRef     =   Type&;
        using   CType       =   const Type;
        using   CTypeRef    =   const Type&;
        using   VType       =   Type;
    public:
        /// <summary>
        /// 默认构造（若Type可默认构造）
        /// </summary>
        TColor() : _value() 
        {}
        template<typename ...Args>
        TColor(Args&&...args)
            :_value(std::forward<Args>(args)...)
        {}
        /// <summary>
        /// 从 const 左值构造
        /// </summary>
        /// <param name="value"></param>
        TColor(const Type& value) 
            : _value(value) 
        {}
        /// <summary>
        /// 从右值构造（真正移动）
        /// </summary>
        /// <param name="value"></param>
        TColor(Type&& value) 
            : _value(std::move(value)) 
        {}
        /// <summary>
        /// 拷贝构造
        /// </summary>
        /// <param name=""></param>
        TColor(const TColor&)   =   default;
        /// <summary>
        /// 移动构造（真正移动）
        /// </summary>
        /// <param name=""></param>
        TColor(TColor&&)        =   default;
        /// <summary>
        /// 获取原始数据值
        /// </summary>
        /// <returns></returns>
        const   Type&   value() const
        {
            return  _value;
        }
        /// <summary>
        /// 赋值操作符
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  TColor& operator=(const TColor&) =   default;
        inline  TColor& operator=(TColor&&)      =   default;
        /// <summary>
        /// 类型转换
        /// </summary>
        inline  operator const Type&() const    
        {
            return _value; 
        }
        /// <summary>
        /// 获取格式
        /// </summary>
        /// <returns></returns>
        constexpr FEFormat  format()   const
        {
            return  _fmt;
        }
    public:
        static  constexpr   FEFormat    Format()
        {
            return  _fmt;
        }
        friend  bool    operator==(const TColor& left,const TColor& right)
        {
            return  left._value == right._value;
        }
        friend  bool    operator!=(const TColor& left,const TColor& right)
        {
            return !(left == right);
        }
    public:
        Type    _value;
    };

   

    using   R8Unorm             =   TColor<uint8,               FMT_R8_UNORM>;
    using   R8Snorm             =   TColor<int8,                FMT_R8_SNORM>;

    using   R8Uscaled           =   TColor<uint8,               FMT_R8_USCALED>;
    using   R8Sscaled           =   TColor<int8,                FMT_R8_SSCALED>;

    using   R8Uint              =   TColor<uint8,               FMT_R8_UINT>;
    using   R8Sint              =   TColor<int8,                FMT_R8_SINT>;
    using   Rg4Unorm            =   TColor<FER4g4Unorm,         FMT_R4G4_UNORM>;


    using   Rg8Unorm            =   TColor<uint8x2,             FMT_R8G8_UNORM>;
    using   Rg8Snorm            =   TColor<int8x2,              FMT_R8G8_SNORM>;
    using   Rg8Uscaled          =   TColor<uint8x2,             FMT_R8G8_USCALED>;
    using   Rg8Sscaled          =   TColor<int8x2,              FMT_R8G8_SSCALED>;
    using   Rg8Uint             =   TColor<uint8x2,             FMT_R8G8_UINT>;
    using   Rg8Sint             =   TColor<int8x2,              FMT_R8G8_SINT>;

    using   Rgba4Unorm          =   TColor<FER4g4b4a4Unorm,     FMT_R4G4B4A4_UNORM>;
    using   Bgra4Unorm          =   TColor<FEB4g4r4a4Unorm,     FMT_B4G4R4A4_UNORM>;
    using   Rgb656Unorm         =   TColor<FER5g6b5Unorm,       FMT_R5G6B5_UNORM>;
    using   Bgr656Unorm         =   TColor<FEB5g6r5Unorm,       FMT_B5G6R5_UNORM>;
    using   Rgba5551Unorm       =   TColor<FER5g5b5a1Unorm,     FMT_R5G5B5A1_UNORM>;
    using   Bgra5551Unorm       =   TColor<FEB5g5r5a1Unorm,     FMT_B5G5R5A1_UNORM>;
    using   Argb1555Unorm       =   TColor<FEA1r5g5b5Unorm,     FMT_A1R5G5B5_UNORM>;

    using   Rgb8Unorm           =   TColor<uint8x3,             FMT_R8G8B8_UNORM>;
    using   Rgb8Snorm           =   TColor<int8x3,              FMT_R8G8B8_SNORM>;
    using   Rgb8Uscaled         =   TColor<uint8x3,             FMT_R8G8B8_USCALED>;
    using   Rgb8Sscaled         =   TColor<int8x3,              FMT_R8G8B8_SSCALED>;
    using   Rgb8Uint            =   TColor<uint8x3,             FMT_R8G8B8_UINT>;
    using   Rgb8Sint            =   TColor<int8x3,              FMT_R8G8B8_SINT>;

    using   Rgba8Unorm          =   TColor<uint8x4,             FMT_R8G8B8A8_UNORM>;
    using   Rgba8Snorm          =   TColor<int8x4,              FMT_R8G8B8A8_SNORM>;
    using   Rgba8Uscaled        =   TColor<uint8x4,             FMT_R8G8B8A8_USCALED>;
    using   Rgba8Sscaled        =   TColor<int8x4,              FMT_R8G8B8A8_SSCALED>;
    using   Rgba8Uint           =   TColor<uint8x4,             FMT_R8G8B8A8_UINT>;
    using   Rgba8Sint           =   TColor<int8x4,              FMT_R8G8B8A8_SINT>;

    using   Rgba8               =   Rgba8Unorm;

    using   A2bgr10Snorm        =   TColor<FEA2b10g10r10Snorm,  FMT_A2B10G10R10_SNORM>;
    using   A2bgr10Unorm        =   TColor<FEA2b10g10r10Unorm,  FMT_A2B10G10R10_UNORM>;

    using   A2bgr10Sint         =   TColor<FEA2b10g10r10Sint,   FMT_A2B10G10R10_SINT>;
    using   A2bgr10Uint         =   TColor<FEA2b10g10r10Uint,   FMT_A2B10G10R10_UINT>;

    using   A2bgr10Sscaled      =   TColor<FEA2b10g10r10Sscaled,FMT_A2B10G10R10_SSCALED>;
    using   A2bgr10Uscaled      =   TColor<FEA2b10g10r10Uscaled,FMT_A2B10G10R10_USCALED>;

    using   R16Unorm            =   TColor<uint16,              FMT_R16_UNORM>;
    using   R16Snorm            =   TColor<int16 ,              FMT_R16_SNORM>;
    using   R16Uscaled          =   TColor<uint16,              FMT_R16_USCALED>;
    using   R16Sscaled          =   TColor<int16 ,              FMT_R16_SSCALED>;
    using   R16Uint             =   TColor<uint16,              FMT_R16_UINT>;
    using   R16Sint             =   TColor<int16 ,              FMT_R16_SINT>;

    using   Rg16Unorm           =   TColor<uint16x2,            FMT_R16G16_UNORM>;
    using   Rg16Snorm           =   TColor<int16x2 ,            FMT_R16G16_SNORM>;
    using   Rg16Uscaled         =   TColor<uint16x2,            FMT_R16G16_USCALED>;
    using   Rg16Sscaled         =   TColor<int16x2 ,            FMT_R16G16_SSCALED>;
    using   Rg16Uint            =   TColor<uint16x2,            FMT_R16G16_UINT>;
    using   Rg16Sint            =   TColor<int16x2 ,            FMT_R16G16_SINT>;

    using   Rgb16Unorm          =   TColor<uint16x3,            FMT_R16G16B16_UNORM>;
    using   Rgb16Snorm          =   TColor<int16x3 ,            FMT_R16G16B16_SNORM>;
    using   Rgb16Uscaled        =   TColor<uint16x3,            FMT_R16G16B16_USCALED>;
    using   Rgb16Sscaled        =   TColor<int16x3 ,            FMT_R16G16B16_SSCALED>;
    using   Rgb16Uint           =   TColor<uint16x3,            FMT_R16G16B16_UINT>;
    using   Rgb16Sint           =   TColor<int16x3 ,            FMT_R16G16B16_SINT>;

    using   Rgba16Unorm         =   TColor<uint16x4,            FMT_R16G16B16A16_UNORM>;
    using   Rgba16Snorm         =   TColor<int16x4 ,            FMT_R16G16B16A16_SNORM>;
    using   Rgba16Uscaled       =   TColor<uint16x4,            FMT_R16G16B16A16_USCALED>;
    using   Rgba16Sscaled       =   TColor<int16x4 ,            FMT_R16G16B16A16_SSCALED>;
    using   Rgba16Uint          =   TColor<uint16x4,            FMT_R16G16B16A16_UINT>;
    using   Rgba16Sint          =   TColor<int16x4 ,            FMT_R16G16B16A16_SINT>;

    using   R32Uint             =   TColor<uint32,              FMT_R32_UINT>;
    using   R32Sint             =   TColor<int32 ,              FMT_R32_SINT>;

    using   Rg32Uint            =   TColor<uint32x2,            FMT_R32G32_UINT>;
    using   Rg32Sint            =   TColor<int32x2 ,            FMT_R32G32_SINT>;

    using   Rgb32Uint           =   TColor<uint32x3,            FMT_R32G32B32_UINT>;
    using   Rgb32Sint           =   TColor<int32x3 ,            FMT_R32G32B32_SINT>;

    using   Rgba32Uint          =   TColor<uint32x4,            FMT_R32G32B32A32_UINT>;
    using   Rgba32Sint          =   TColor<int32x4 ,            FMT_R32G32B32A32_SINT>;

    using   R16Float            =   TColor<float16,             FMT_R16_FLOAT>;
    using   Rg16Float           =   TColor<tvec2<float16>,      FMT_R16G16_FLOAT>;
    using   Rgb16Float          =   TColor<tvec3<float16>,      FMT_R16G16B16_FLOAT>;
    using   Rgba16Float         =   TColor<tvec4<float16>,      FMT_R16G16B16A16_FLOAT>;

    using   R32Float            =   TColor<float,               FMT_R32_FLOAT>;
    using   Rg32Float           =   TColor<tvec2<float>,        FMT_R32G32_FLOAT>;
    using   Rgb32Float          =   TColor<tvec3<float>,        FMT_R32G32B32_FLOAT>;
    using   Rgba32Float         =   TColor<tvec4<float>,        FMT_R32G32B32A32_FLOAT>;

    using   D16Float            =   TColor<float16,             FMT_D16_UNORM>;
    using   D32Float            =   TColor<float,               FMT_D32_UNORM>;

    using   Rgba8s              =   std::vector<Rgba8>;

    /// <summary>
    /// 量化类别: 决定存储值与浮点颜色之间的转换语义
    /// Packed 为位打包格式(R4G4/565/10_10_10_2等,本次不支持转换,保持不可转换)
    /// </summary>
    enum class FEQuantCat : uint8
    {
        None,       /// 无/未知
        Unorm,      /// [0,max] 整数 ↔ [0,1] 浮点
        Snorm,      /// [-max,max] 整数 ↔ [-1,1] 浮点
        Uscaled,    /// 无符号整数,浮点语义(值保持)
        Sscaled,    /// 有符号整数,浮点语义(值保持)
        Uint,       /// 无符号整数原值
        Sint,       /// 有符号整数原值
        Float,      /// 浮点
        Half,       /// 半精度浮点
        Packed      /// 位打包格式
    };
    /// <summary>
    /// 格式 → 量化类别
    /// </summary>
    constexpr   FEQuantCat  quantCatOfFormat(FEFormat fmt)
    {
        switch (fmt)
        {
        case FMT_R8_UNORM            :
        case FMT_R8G8_UNORM          :
        case FMT_R8G8B8_UNORM        :
        case FMT_R8G8B8A8_UNORM      :
        case FMT_B8G8R8A8_UNORM      :
        case FMT_R16_UNORM           :
        case FMT_R16G16_UNORM        :
        case FMT_R16G16B16_UNORM     :
        case FMT_R16G16B16A16_UNORM  :  return  FEQuantCat::Unorm;

        case FMT_R8_SNORM            :
        case FMT_R8G8_SNORM          :
        case FMT_R8G8B8_SNORM        :
        case FMT_R8G8B8A8_SNORM      :
        case FMT_B8G8R8A8_SNORM      :
        case FMT_R16_SNORM           :
        case FMT_R16G16_SNORM        :
        case FMT_R16G16B16_SNORM     :
        case FMT_R16G16B16A16_SNORM  :  return  FEQuantCat::Snorm;

        case FMT_R8_USCALED          :
        case FMT_R8G8_USCALED        :
        case FMT_R8G8B8_USCALED      :
        case FMT_R8G8B8A8_USCALED    :
        case FMT_B8G8R8A8_USCALED    :
        case FMT_R16_USCALED         :
        case FMT_R16G16_USCALED      :
        case FMT_R16G16B16_USCALED   :
        case FMT_R16G16B16A16_USCALED:  return  FEQuantCat::Uscaled;

        case FMT_R8_SSCALED          :
        case FMT_R8G8_SSCALED        :
        case FMT_R8G8B8_SSCALED      :
        case FMT_R8G8B8A8_SSCALED    :
        case FMT_B8G8R8A8_SSCALED    :
        case FMT_R16_SSCALED         :
        case FMT_R16G16_SSCALED      :
        case FMT_R16G16B16_SSCALED   :
        case FMT_R16G16B16A16_SSCALED:  return  FEQuantCat::Sscaled;

        case FMT_R8_UINT             :
        case FMT_R8G8_UINT           :
        case FMT_R8G8B8_UINT         :
        case FMT_R8G8B8A8_UINT       :
        case FMT_B8G8R8A8_UINT       :
        case FMT_R16_UINT            :
        case FMT_R16G16_UINT         :
        case FMT_R16G16B16_UINT      :
        case FMT_R16G16B16A16_UINT   :
        case FMT_R32_UINT            :
        case FMT_R32G32_UINT         :
        case FMT_R32G32B32_UINT      :
        case FMT_R32G32B32A32_UINT   :  return  FEQuantCat::Uint;

        case FMT_R8_SINT             :
        case FMT_R8G8_SINT           :
        case FMT_R8G8B8_SINT         :
        case FMT_R8G8B8A8_SINT       :
        case FMT_B8G8R8A8_SINT       :
        case FMT_R16_SINT            :
        case FMT_R16G16_SINT         :
        case FMT_R16G16B16_SINT      :
        case FMT_R16G16B16A16_SINT   :
        case FMT_R32_SINT            :
        case FMT_R32G32_SINT         :
        case FMT_R32G32B32_SINT      :
        case FMT_R32G32B32A32_SINT   :  return  FEQuantCat::Sint;

        case FMT_R32_FLOAT           :
        case FMT_R32G32_FLOAT        :
        case FMT_R32G32B32_FLOAT     :
        case FMT_R32G32B32A32_FLOAT  :
        case FMT_D32_UNORM           :  return  FEQuantCat::Float;

        case FMT_R16_FLOAT           :
        case FMT_R16G16_FLOAT        :
        case FMT_R16G16B16_FLOAT     :
        case FMT_R16G16B16A16_FLOAT  :
        case FMT_D16_UNORM           :  return  FEQuantCat::Half;

        case FMT_R4G4_UNORM          :
        case FMT_R4G4B4A4_UNORM      :
        case FMT_B4G4R4A4_UNORM      :
        case FMT_R5G6B5_UNORM        :
        case FMT_B5G6R5_UNORM        :
        case FMT_R5G5B5A1_UNORM      :
        case FMT_B5G5R5A1_UNORM      :
        case FMT_A1R5G5B5_UNORM      :
        case FMT_A2B10G10R10_UNORM   :
        case FMT_A2B10G10R10_SNORM   :
        case FMT_A2B10G10R10_USCALED :
        case FMT_A2B10G10R10_SSCALED :
        case FMT_A2B10G10R10_UINT    :
        case FMT_A2B10G10R10_SINT    :
        case FMT_D32_S8_UNORM        :  return  FEQuantCat::Packed;
        default                        :  return  FEQuantCat::None;
        }
    }
    /// <summary>
    /// C++标量类型 → 默认量化类别(裸类型源未指明格式时的语义约定:
    /// uint8/uint16 按Unorm, int8/int16 按Snorm, 32位及以上整型按原值Uint/Sint)
    /// </summary>
    template<class T>
    struct  QuantOfScalar       {   static  constexpr FEQuantCat    value   =   FEQuantCat::None;   };
    template<>  struct  QuantOfScalar<uint8_t>  {   static  constexpr FEQuantCat    value   =   FEQuantCat::Unorm;  };
    template<>  struct  QuantOfScalar<uint16_t> {   static  constexpr FEQuantCat    value   =   FEQuantCat::Unorm;  };
    template<>  struct  QuantOfScalar<int8_t>   {   static  constexpr FEQuantCat    value   =   FEQuantCat::Snorm;  };
    template<>  struct  QuantOfScalar<int16_t>  {   static  constexpr FEQuantCat    value   =   FEQuantCat::Snorm;  };
    template<>  struct  QuantOfScalar<uint32_t> {   static  constexpr FEQuantCat    value   =   FEQuantCat::Uint;   };
    template<>  struct  QuantOfScalar<uint64_t> {   static  constexpr FEQuantCat    value   =   FEQuantCat::Uint;   };
    template<>  struct  QuantOfScalar<int32_t>  {   static  constexpr FEQuantCat    value   =   FEQuantCat::Sint;   };
    template<>  struct  QuantOfScalar<int64_t>  {   static  constexpr FEQuantCat    value   =   FEQuantCat::Sint;   };
    template<>  struct  QuantOfScalar<float>    {   static  constexpr FEQuantCat    value   =   FEQuantCat::Float;  };
    template<>  struct  QuantOfScalar<double>   {   static  constexpr FEQuantCat    value   =   FEQuantCat::Float;  };
    template<>  struct  QuantOfScalar<float16>  {   static  constexpr FEQuantCat    value   =   FEQuantCat::Half;   };

    /// <summary>
    /// 类型分解: 标量类型 + 分量数 + 分量访问
    /// 未列出的类型(如位打包结构体)保持为空,SFINAE探测为不可转换
    /// </summary>
    template<class T,class = void>
    struct  FEVecTraits
    {};
    /// 算术标量: 1分量
    template<class T>
    struct  FEVecTraits<T,std::enable_if_t<std::is_arithmetic_v<T>>>
    {
        using   Scalar              =   T;
        static  constexpr uint32    Count   =   1;
        static  const T&            get(const T& v,uint32)  {   return  v;  }
    };
    /// float16标量(非算术类型,单独列出)
    template<>
    struct  FEVecTraits<float16,void>
    {
        using   Scalar              =   float16;
        static  constexpr uint32    Count   =   1;
        static  const float16&      get(const float16& v,uint32)    {   return  v;  }
    };
    /// glm向量: N分量
    template<glm::length_t N,class T>
    struct  FEVecTraits<glm::vec<N,T,glm::defaultp>,void>
    {
        using   Scalar              =   T;
        static  constexpr uint32    Count   =   uint32(N);
        static  const T&            get(const glm::vec<N,T,glm::defaultp>& v,uint32 i)  {   return  v[i];   }
    };

    /// <summary>
    /// 标量量化值 → float
    /// </summary>
    template<FEQuantCat Cat,class T>
    struct  TScalarToFloat
    {
        static  float   convert(const T& v)
        {
            if constexpr (Cat == FEQuantCat::Unorm)
                return  float(v) / float((std::numeric_limits<T>::max)());
            else if constexpr (Cat == FEQuantCat::Snorm)
                return  (std::max)(float(v) / float((std::numeric_limits<T>::max)()),-1.0f);
            else
                /// Uscaled/Sscaled/Uint/Sint/Float/Half: 原值转浮点
                return  float(v);
        }
    };
    /// <summary>
    /// float → 标量量化值(统一 clamp + round 语义)
    /// </summary>
    template<FEQuantCat Cat,class T>
    struct  TScalarFromFloat
    {
        static  T       convert(float v)
        {
            if constexpr (Cat == FEQuantCat::Unorm)
            {
                const double    t   =   (std::min)((std::max)(double(v),0.0),1.0);
                return  T(std::round(t * double((std::numeric_limits<T>::max)())));
            }
            else if constexpr (Cat == FEQuantCat::Snorm)
            {
                const double    t   =   (std::min)((std::max)(double(v),-1.0),1.0);
                return  T(std::round(t * double((std::numeric_limits<T>::max)())));
            }
            else if constexpr (Cat == FEQuantCat::Uint || Cat == FEQuantCat::Uscaled)
            {
                const double    t   =   (std::min)((std::max)(double(v),0.0),double((std::numeric_limits<T>::max)()));
                return  T(std::round(t));
            }
            else if constexpr (Cat == FEQuantCat::Sint || Cat == FEQuantCat::Sscaled)
            {
                const double    t   =   (std::min)((std::max)(double(v),double((std::numeric_limits<T>::min)())),double((std::numeric_limits<T>::max)()));
                return  T(std::round(t));
            }
            else if constexpr (Cat == FEQuantCat::Half)
                return  T(float16(v));
            else
                /// Float: 直转
                return  T(v);
        }
    };
    /// <summary>
    /// 分量级量化转换内核,规则:
    /// 1. 同型直通
    /// 2. Unorm↔Unorm 按位宽缩放(8→16: x*257; 16→8: (x*255+32895)>>16,与既有实现位精确一致)
    /// 3. 任一端为浮点/半浮点: 经float枢纽(含clamp+round)
    /// 4. 其余整型↔整型: 值保持 + 宿范围钳制
    /// </summary>
    template<FEQuantCat FromCat,class TFrom,FEQuantCat ToCat,class TTo>
    struct  TQuantConvert
    {
        static  TTo     convert(const TFrom& v)
        {
            if constexpr (std::is_same_v<TFrom,TTo>)
                return  v;
            else if constexpr (FromCat == FEQuantCat::Unorm && ToCat == FEQuantCat::Unorm)
            {
                if constexpr (sizeof(TFrom) < sizeof(TTo))
                    return  TTo(TTo(v) * TTo(257));
                else if constexpr (sizeof(TFrom) > sizeof(TTo))
                    return  TTo((uint32(v) * 255 + 32895) >> 16);
                else
                    return  TTo(v);
            }
            else if constexpr (   FromCat == FEQuantCat::Float || FromCat == FEQuantCat::Half
                               || ToCat   == FEQuantCat::Float || ToCat   == FEQuantCat::Half)
            {
                return  TScalarFromFloat<ToCat,TTo>::convert(TScalarToFloat<FromCat,TFrom>::convert(v));
            }
            else
            {
                const double    t   =   (std::min)((std::max)(double(v),double((std::numeric_limits<TTo>::min)())),double((std::numeric_limits<TTo>::max)()));
                return  TTo(t);
            }
        }
    };
    /// <summary>
    /// 向量 → float4 枢纽(缺失分量补0,缺失alpha补1)
    /// </summary>
    template<class T,FEQuantCat Cat>
    struct  TToFloat4
    {
        using   S       =   typename FEVecTraits<T>::Scalar;
        static  float4  convert(const T& v)
        {
            constexpr uint32    N   =   FEVecTraits<T>::Count;
            float4  out(0.0f,0.0f,0.0f,1.0f);
            out[0]  =   TScalarToFloat<Cat,S>::convert(FEVecTraits<T>::get(v,0));
            if constexpr (N >= 2)   out[1]  =   TScalarToFloat<Cat,S>::convert(FEVecTraits<T>::get(v,1));
            if constexpr (N >= 3)   out[2]  =   TScalarToFloat<Cat,S>::convert(FEVecTraits<T>::get(v,2));
            if constexpr (N >= 4)   out[3]  =   TScalarToFloat<Cat,S>::convert(FEVecTraits<T>::get(v,3));
            return  out;
        }
    };
    /// <summary>
    /// float4 → 向量枢纽(按宿分量数截取)
    /// </summary>
    template<class T,FEQuantCat Cat>
    struct  TFromFloat4
    {
        using   S       =   typename FEVecTraits<T>::Scalar;
        static  T       convert(const float4& v)
        {
            constexpr uint32    N   =   FEVecTraits<T>::Count;
            const S     c[4]    =   {   TScalarFromFloat<Cat,S>::convert(v[0])
                                    ,   TScalarFromFloat<Cat,S>::convert(v[1])
                                    ,   TScalarFromFloat<Cat,S>::convert(v[2])
                                    ,   TScalarFromFloat<Cat,S>::convert(v[3])  };
            if constexpr (N == 1)       return  T(c[0]);
            else if constexpr (N == 2)  return  T(c[0],c[1]);
            else if constexpr (N == 3)  return  T(c[0],c[1],c[2]);
            else                        return  T(c[0],c[1],c[2],c[3]);
        }
    };
    /// <summary>
    /// 整型向量直通: 逐分量TQuantConvert,缺失分量补0,缺失alpha补宿表示下的1
    /// </summary>
    template<class TFrom,FEQuantCat FromCat,class TTo,FEQuantCat ToCat>
    struct  TIntVecConvert
    {
        using   FS      =   typename FEVecTraits<TFrom>::Scalar;
        using   TS      =   typename FEVecTraits<TTo>::Scalar;
        static  TTo     convert(const TFrom& v)
        {
            constexpr uint32    FN  =   FEVecTraits<TFrom>::Count;
            constexpr uint32    TN  =   FEVecTraits<TTo>::Count;
            TS  c[4]    =   {   TS(0),TS(0),TS(0),alphaOne()    };
            c[0]    =   TQuantConvert<FromCat,FS,ToCat,TS>::convert(FEVecTraits<TFrom>::get(v,0));
            if constexpr (FN >= 2 && TN >= 2)   c[1]    =   TQuantConvert<FromCat,FS,ToCat,TS>::convert(FEVecTraits<TFrom>::get(v,1));
            if constexpr (FN >= 3 && TN >= 3)   c[2]    =   TQuantConvert<FromCat,FS,ToCat,TS>::convert(FEVecTraits<TFrom>::get(v,2));
            if constexpr (FN >= 4 && TN >= 4)   c[3]    =   TQuantConvert<FromCat,FS,ToCat,TS>::convert(FEVecTraits<TFrom>::get(v,3));
            if constexpr (TN == 1)      return  TTo(c[0]);
            else if constexpr (TN == 2) return  TTo(c[0],c[1]);
            else if constexpr (TN == 3) return  TTo(c[0],c[1],c[2]);
            else                        return  TTo(c[0],c[1],c[2],c[3]);
        }
    private:
        /// 宿表示下的 alpha=1: Unorm取满量程,其余取整型1
        static  TS      alphaOne()
        {
            if constexpr (ToCat == FEQuantCat::Unorm)   return  (std::numeric_limits<TS>::max)();
            else                                        return  TS(1);
        }
    };

    /// <summary>
    /// 颜色对象 → 自身格式; 非颜色对象 → FMT_NONE
    /// </summary>
    template<class T>
    struct  ColorFormatOf       {   static  constexpr FEFormat  value   =   FMT_NONE;   };
    template<class Type,FEFormat FMT>
    struct  ColorFormatOf<TColor<Type,FMT>> {   static  constexpr FEFormat  value   =   FMT;      };

    /// <summary>
    /// 解析类型的量化类别: 显式格式优先,否则按标量类型默认约定
    /// </summary>
    template<class T,FEFormat FMT>
    constexpr   FEQuantCat  catOfType()
    {
        if constexpr (FMT != FMT_NONE)
            return  quantCatOfFormat(FMT);
        else
            return  QuantOfScalar<typename FEVecTraits<T>::Scalar>::value;
    }
    constexpr   bool        isConvertibleCat(FEQuantCat cat)
    {
        return  cat != FEQuantCat::None && cat != FEQuantCat::Packed;
    }
    /// <summary>
    /// 探测类型对是否可转换: 同型恒可(拷贝),异型要求类型可分解且量化语义有效
    /// </summary>
    template<class TFrom,class TTo,FEFormat ToFMT,FEFormat FromFMT,class = void>
    struct  IsConvertPair       :   std::bool_constant<std::is_same_v<TFrom,TTo>>
    {};
    template<class TFrom,class TTo,FEFormat ToFMT,FEFormat FromFMT>
    struct  IsConvertPair<TFrom,TTo,ToFMT,FromFMT,
            std::void_t<typename FEVecTraits<TFrom>::Scalar,typename FEVecTraits<TTo>::Scalar>>
        :   std::bool_constant<    std::is_same_v<TFrom,TTo>
                                || (isConvertibleCat(catOfType<TFrom,FromFMT>()) && isConvertibleCat(catOfType<TTo,ToFMT>()))>
    {};

    template<class TFrom,class TTo,FEFormat ToFMT = FMT_NONE,FEFormat FromFMT = FMT_NONE>
    struct  TConvert 
    {
        /// 成员模板使enable_if依赖函数模板参数,不支持的转换对走SFINAE替换失败,
        /// 配合IsTConvertible探测,避免类实例化时产生硬错误
        /// ToFMT: 宿格式(决定宿量化语义,如uint8x4在UNORM下按[0,1]缩放,在UINT下按原值取整钳制)
        /// FromFMT: 源格式(TColor源自动携带;裸类型源按QuantOfScalar默认约定)
        template<class F = TFrom,class T = TTo>
        static  auto    convert(const TFrom& data)
            ->  std::enable_if_t<IsConvertPair<F,T,ToFMT,FromFMT>::value,TTo>
        {
            if constexpr (std::is_same_v<F,T>)
                return  data;
            else
            {
                constexpr FEQuantCat    fromCat =   catOfType<F,FromFMT>();
                constexpr FEQuantCat    toCat   =   catOfType<T,ToFMT>();
                if constexpr (   fromCat == FEQuantCat::Float || fromCat == FEQuantCat::Half
                              || toCat   == FEQuantCat::Float || toCat   == FEQuantCat::Half)
                    /// 任一端浮点: float4枢纽
                    return  TFromFloat4<T,toCat>::convert(TToFloat4<F,fromCat>::convert(data));
                else
                    /// 两端整型: 逐分量直转
                    return  TIntVecConvert<F,fromCat,T,toCat>::convert(data);
            }
        }
    };
    /// <summary>
    /// 探测TConvert<TFrom,TTo,ToFMT,FromFMT>是否存在可用的convert实现
    /// </summary>
    template<class TFrom,class TTo,FEFormat ToFMT = FMT_NONE,FEFormat FromFMT = FMT_NONE,class = void>
    struct  IsTConvertible      :   std::false_type
    {};
    template<class TFrom,class TTo,FEFormat ToFMT,FEFormat FromFMT>
    struct  IsTConvertible<TFrom,TTo,ToFMT,FromFMT,std::void_t<decltype(TConvert<TFrom,TTo,ToFMT,FromFMT>::convert(std::declval<const TFrom&>()))>>   :   std::true_type
    {};

    /// 既有A2B10G10R10_SNORM打包结构的手工转换,任意格式参数下保持既有行为
    template<FEFormat ToFMT,FEFormat FromFMT>
    struct  TConvert<float4,FEA2b10g10r10Snorm,ToFMT,FromFMT>
    {
        static  FEA2b10g10r10Snorm    convert(const float4& data)
        {
            return  FEA2b10g10r10Snorm(data.r,data.g,data.b,data.a);
        }
    };
    template<FEFormat ToFMT,FEFormat FromFMT>
    struct  TConvert<FEA2b10g10r10Snorm,float4,ToFMT,FromFMT>
    {
        static  float4    convert(const FEA2b10g10r10Snorm& data)
        {
            return  float4(  float(data.red()     * oneOver511F)
                            ,float(data.green()   * oneOver511F)
                            ,float(data.blue()    * oneOver511F)
                            ,float(data.alpha()));
        }
    };
    template<FEFormat ToFMT,FEFormat FromFMT>
    struct  TConvert<FEA2b10g10r10Snorm,int4,ToFMT,FromFMT>
    {
        static  int4    convert(const FEA2b10g10r10Snorm& data)
        {
            return  int4( data.red()  ,data.green(),data.blue() ,data.alpha());
        }
    };
    template<FEFormat ToFMT,FEFormat FromFMT>
    struct  TConvert<int4,FEA2b10g10r10Snorm,ToFMT,FromFMT>
    {
        static  FEA2b10g10r10Snorm    convert(const int4& data)
        {
            return  FEA2b10g10r10Snorm(data.r,data.g,data.b,data.a);
        }
    };

    template <typename T>
    struct  IsColorFromat : std::false_type 
    {};

    /// <summary>
    /// 特化版本：若 T::format() 可调用，则继承 true_type
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template <typename Type, FEFormat Fmt>
    struct  IsColorFromat<TColor<Type, Fmt>> : std::true_type 
    {};
    /// <summary>
    /// 提取类型,通用类定义
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <typeparam name="IsTColor"></typeparam>
    template <typename T, bool IsTColor = IsColorFromat<T>::value>
    struct  ExtractColorValueTypeImpl 
    {
        using   type    =   T;
    };
    /// <summary>
    /// 特化 IsTColor == true,会走到该模板
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template <typename T>
    struct  ExtractColorValueTypeImpl<T, true> 
    {
        using   type    =   typename T::VType;
    };

    template <typename T>
    using   ExtractColorValueType   =   typename ExtractColorValueTypeImpl<T>::type;


    template<typename From, typename To, FEFormat FMT = FMT_NONE>
    To  castTo(const From& data) 
    {
        using   VFrom   =   ExtractColorValueType<From>;
        using   VTo     =   ExtractColorValueType<To>;
        /// 宿格式: 显式参数优先,其次取宿颜色对象自身格式; 源格式取源颜色对象自身格式
        constexpr FEFormat    toFMT   =   (FMT != FMT_NONE) ? FMT : ColorFormatOf<To>::value;
        return  TConvert<VFrom,VTo,toFMT,ColorFormatOf<From>::value>::convert(data);
    }
}

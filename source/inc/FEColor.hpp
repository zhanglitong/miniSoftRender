#pragma     once

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

    using   A2bgr10Sint         =   TColor<FEA2b10g10r10Snorm,  FMT_A2B10G10R10_SINT>;
    using   A2bgr10Uint         =   TColor<FEA2b10g10r10Unorm,  FMT_A2B10G10R10_UINT>;

    using   A2bgr10Sscaled      =   TColor<FEA2b10g10r10Snorm,  FMT_A2B10G10R10_SSCALED>;
    using   A2bgr10Uscaled      =   TColor<FEA2b10g10r10Unorm,  FMT_A2B10G10R10_USCALED>;

    using   R16Unorm            =   TColor<uint16,              FMT_R16G16_UNORM>;
    using   R16Snorm            =   TColor<int16 ,              FMT_R16G16_SNORM>;
    using   R16Uscaled          =   TColor<uint16,              FMT_R16G16_USCALED>;
    using   R16Sscaled          =   TColor<int16 ,              FMT_R16G16_SSCALED>;
    using   R16Uint             =   TColor<uint16,              FMT_R16G16_UINT>;
    using   R16Sint             =   TColor<int16 ,              FMT_R16G16_SINT>;

    using   Rg16Unorm           =   TColor<uint16x2,            FMT_R16G16_UNORM>;
    using   Rg16Snorm           =   TColor<int16x2 ,            FMT_R16G16_SNORM>;
    using   Rg16Uscaled         =   TColor<uint16x2,            FMT_R16G16_USCALED>;
    using   Rg16Sscaled         =   TColor<int16x2 ,            FMT_R16G16_SSCALED>;
    using   Rg16Uint            =   TColor<uint16x2,            FMT_R16G16_UINT>;
    using   Rg16Sint            =   TColor<int16x2 ,            FMT_R16G16_SINT>;

    using   Rgb16Unorm          =   TColor<uint16x2,            FMT_R16G16B16_UNORM>;
    using   Rgb16Snorm          =   TColor<int16x2 ,            FMT_R16G16B16_SNORM>;
    using   Rgb16Uscaled        =   TColor<uint16x2,            FMT_R16G16B16_USCALED>;
    using   Rgb16Sscaled        =   TColor<int16x2 ,            FMT_R16G16B16_SSCALED>;
    using   Rgb16Uint           =   TColor<uint16x2,            FMT_R16G16B16_UINT>;
    using   Rgb16Sint           =   TColor<int16x2 ,            FMT_R16G16B16_SINT>;

    using   Rgba16Unorm         =   TColor<uint16x2,            FMT_R16G16B16A16_UNORM>;
    using   Rgba16Snorm         =   TColor<int16x2 ,            FMT_R16G16B16A16_SNORM>;
    using   Rgba16Uscaled       =   TColor<uint16x2,            FMT_R16G16B16A16_USCALED>;
    using   Rgba16Sscaled       =   TColor<int16x2 ,            FMT_R16G16B16A16_SSCALED>;
    using   Rgba16Uint          =   TColor<uint16x2,            FMT_R16G16B16A16_UINT>;
    using   Rgba16Sint          =   TColor<int16x2 ,            FMT_R16G16B16A16_SINT>;

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

    template<class TFrom, class TTo>
    struct  TConvert 
    {
        static  TTo   convert(const TFrom& data) 
        {
            if constexpr (std::is_same_v<TFrom, TTo>) 
                return data;
            else 
                return static_cast<TTo>(data);
        }
    };
    template<>
    struct  TConvert<float4,    uint8> 
    {
        static  uint8   convert(const float4& data) 
        {
            return  (uint8)(data.r * _255F);
        }
    };
    template<>
    struct  TConvert<float4,    uint8x2> 
    {
        static uint8x2 convert(const float4& data) 
        {
            return  uint8x2( data.r * _255F
                            ,data.g * _255F);
        }
    };
    template<>
    struct  TConvert<float4,    uint8x3> 
    {
        static uint8x3 convert(const float4& data) 
        {
            return  uint8x3( data.r * _255F
                            ,data.g * _255F
                            ,data.b * _255F);
        }
    };
    template<>
    struct  TConvert<float4,    uint8x4> 
    {
        static uint8x4  convert(const float4& data) 
        {
            return  uint8x4(     data.r * _255F
                                ,data.g * _255F
                                ,data.b * _255F
                                ,data.a * _255F);
        }
    };

    template<>
    struct  TConvert<uint8,     float4> 
    {
        static float4 convert(uint8 data) 
        {
            return  float4( data * oneOver255F,0.0f,0.0f,1.0f);
        }
    };
    template<>
    struct  TConvert<uint8x2,   float4> 
    {
        static float4 convert(const Rgba8& data) 
        {
            return  float4(  data._value.r * oneOver255F
                            ,data._value.g * oneOver255F
                            ,0.0f
                            ,1.0f);
        }
    };
    template<>
    struct  TConvert<uint8x3,   float4> 
    {
        static float4 convert(const Rgba8& data) 
        {
            return  float4(  data._value.r * oneOver255F
                            ,data._value.g * oneOver255F
                            ,data._value.b * oneOver255F
                            ,1.0f);
        }
    };
    template<>
    struct  TConvert<uint8x4,   float4> 
    {
        static float4 convert(const Rgba8& data) 
        {
            return  float4(  data._value.r * oneOver255F
                            ,data._value.g * oneOver255F
                            ,data._value.b * oneOver255F
                            ,data._value.a * oneOver255F);
        }
    };

    template<>
    struct  TConvert<uint16,    uint8x4> 
    {
        static uint8x4 convert(const uint16& data) 
        {
            return  uint8x4(((data * 255 + 32895) >> 16),0,0,255);
        }
    };
    template<>
    struct  TConvert<uint16x2,  uint8x4> 
    {
        static uint8x4 convert(const uint16x3& data) 
        {
            return  uint8x4( ((data.r * 255 + 32895) >> 16)
                            ,((data.g * 255 + 32895) >> 16)
                            ,0
                            ,255);
        }
    };
    template<>
    struct  TConvert<uint16x3,  uint8x4> 
    {
        static uint8x4 convert(const uint16x3& data) 
        {
            return  uint8x4( ((data.r * 255 + 32895) >> 16)
                            ,((data.g * 255 + 32895) >> 16)
                            ,((data.b * 255 + 32895) >> 16)
                            ,255);
        }
    };
    template<>
    struct  TConvert<uint16x4,  uint8x4> 
    {
        static uint8x4 convert(const uint16x4& data) 
        {
            return  uint8x4( ((data.r * 255 + 32895) >> 16)
                            ,((data.g * 255 + 32895) >> 16)
                            ,((data.b * 255 + 32895) >> 16)
                            ,((data.a * 255 + 32895) >> 16));
        }
    };
    
    template<>
    struct  TConvert<uint8,     uint16x4> 
    {
        static uint16x4 convert(const uint8& data) 
        {
            return  uint16x4((data  * 257),0,0,65535);
        }
    };
    template<>
    struct  TConvert<uint8x2,   uint16x4> 
    {
        static uint16x4 convert(const uint8x4& data) 
        {
            return  uint16x4(    (data.r * 257)
                                ,(data.g * 257)
                                ,0
                                ,65535);
        }
    };
    template<>
    struct  TConvert<uint8x3,   uint16x4> 
    {
        static uint16x4 convert(const uint8x3& data) 
        {
            return  uint16x4(    (data.r * 257)
                                ,(data.g * 257)
                                ,(data.b * 257)
                                ,65535);
        }
    };
    template<>
    struct  TConvert<uint8x4,   uint16x4> 
    {
        static uint16x4 convert(const uint8x4& data) 
        {
            return  uint16x4(    (data.r * 257)
                                ,(data.g * 257)
                                ,(data.b * 257)
                                ,(data.a * 257));
        }
    };
    template<>
    struct  TConvert<uint8,     uint8x4> 
    {
        static uint8x4  convert(const uint8& data) 
        {
            return  uint8x4(data,0,0,255);
        }
    };
    template<>
    struct  TConvert<uint8x2,   uint8x4> 
    {
        static uint8x4 convert(const uint8x4& data) 
        {
            return  uint8x4(     (data.r)
                                ,(data.g)
                                ,0
                                ,255);
        }
    };
    template<>
    struct  TConvert<uint8x3,   uint8x4> 
    {
        static uint8x4  convert(const uint8x3& data) 
        {
            return  uint8x4(     (data.r)
                                ,(data.g)
                                ,(data.b),255);
        }
    };
    template<>
    struct  TConvert<uint8x4,   uint8x4> 
    {
        static uint8x4  convert(const uint8x4& data) 
        {
            return  data;
        }
    };

    template<>
    struct  TConvert<uint8x4,   uint8> 
    {
        static uint8    convert(const uint8x4& data) 
        {
            return  data.r;
        }
    };
    template<>
    struct  TConvert<uint8x4,   uint8x2> 
    {
        static uint8x2  convert(const uint8x4& data) 
        {
            return  uint8x2(data.r,data.g);
        }
    };
    template<>
    struct  TConvert<uint8x4,   uint8x3> 
    {
        static uint8x3  convert(const uint8x4& data) 
        {
            return  uint8x3(data.r,data.g,data.b);
        }
    };

    template<>
    struct  TConvert<uint16,    float4> 
    {
        static float4   convert(const uint16& data) 
        {
            return  float4(  (data * oneOver65535F )
                            ,0.0f
                            ,0.0f
                            ,1.0f);
        }
    };
    template<>
    struct  TConvert<uint16x2,  float4> 
    {
        static float4   convert(const uint16x2& data) 
        {
            return  float4(  (data.r * oneOver65535F )
                            ,(data.g * oneOver65535F )
                            ,0.0f
                            ,1.0f);
        }
    };
    template<>
    struct  TConvert<uint16x3,  float4> 
    {
        static float4   convert(const uint16x3& data) 
        {
            return  float4(  (data.r * oneOver65535F )
                            ,(data.g * oneOver65535F )
                            ,(data.b * oneOver65535F )
                            ,1.0f);
        }
    };
    template<>
    struct  TConvert<uint16x4,  float4> 
    {
        static float4   convert(const uint16x4& data) 
        {
            return  float4(  (data.r * oneOver65535F )
                            ,(data.g * oneOver65535F )
                            ,(data.b * oneOver65535F )
                            ,(data.a * oneOver65535F ));
        }
    };
    template<>
    struct  TConvert<float,     uint16x4> 
    {
        static uint16x4 convert(const float & data) 
        {
            return  uint16x4(  uint16(data  * _65535F),0,0,65535);
        }
    };
    template<>
    struct  TConvert<float2,    uint16x4> 
    {
        static uint16x4 convert(const float2& data) 
        {
            return  uint16x4(uint16((data.r * _65535F))
                            ,uint16((data.g * _65535F))
                            ,uint16(0)
                            ,uint16(65535));
        }
    };
    template<>
    struct  TConvert<float3,    uint16x4> 
    {
        static uint16x4 convert(const float3& data) 
        {
            return  uint16x4(uint16(data.r * _65535F)
                            ,uint16(data.g * _65535F)
                            ,uint16(data.b * _65535F)
                            ,uint16(65535));
        }
    };
    template<>
    struct  TConvert<float4,    uint16x4> 
    {
        static uint16x4 convert(const float4& data) 
        {
            return  uint16x4(uint16(data.r * _65535F)
                            ,uint16(data.g * _65535F)
                            ,uint16(data.b * _65535F)
                            ,uint16(data.a * _65535F));
        }
    };

    template<>
    struct  TConvert<float,     uint8x4> 
    {
        static uint8x4 convert(const float & data) 
        {
            return  uint8x4(  (data  * _255F),0.0f,0.0f,_255F);
        }
    };
    template<>
    struct  TConvert<float2,    uint8x4> 
    {
        static uint8x4 convert(const float2& data) 
        {
            return  uint8x4( uint8((data.r * _255F))
                            ,uint8((data.g * _255F))
                            ,uint8(0)
                            ,uint8(255));
        }
    };
    template<>
    struct  TConvert<float3,    uint8x4> 
    {
        static uint8x4  convert(const float3& data) 
        {
            return  uint8x4( uint8((data.r * _255F))
                            ,uint8((data.g * _255F))
                            ,uint8((data.b * _255F))
                            ,uint8(255));
        }
    };

    template<>
    struct  TConvert<int,       uint8x4> 
    {
        static uint8x4 convert(const int & data) 
        {
            return  uint8x4(  data,0,0,255);
        }
    };
    template<>
    struct  TConvert<int2,      uint8x4> 
    {
        static uint8x4 convert(const int2& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(0)
                            ,uint8(255));
        }
    };
    template<>
    struct  TConvert<int3,      uint8x4> 
    {
        static uint8x4  convert(const int3& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(data.b)
                            ,uint8(255));
        }
    };
    template<>
    struct  TConvert<int4,      uint8x4> 
    {
        static uint8x4  convert(const int4& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(data.b)
                            ,uint8(data.a));
        }
    };
    template<>
    struct  TConvert<uint32_t,  uint8x4> 
    {
        static uint8x4 convert(const uint32_t & data) 
        {
            return  uint8x4(  data,0,0,255);
        }
    };
    template<>
    struct  TConvert<uint32x2,  uint8x4> 
    {
        static uint8x4 convert(const uint32x2& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(0)
                            ,uint8(255));
        }
    };
    template<>
    struct  TConvert<uint32x3,  uint8x4> 
    {
        static uint8x4  convert(const uint32x3& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(data.b)
                            ,uint8(255));
        }
    };
    template<>
    struct  TConvert<uint32x4,  uint8x4> 
    {
        static uint8x4  convert(const uint32x4& data) 
        {
            return  uint8x4( uint8(data.r)
                            ,uint8(data.g)
                            ,uint8(data.b)
                            ,uint8(data.a));
        }
    };
    template<>
    struct  TConvert<float,     half4> 
    {
        static  half4   convert(const float & data) 
        {
            return  half4(half(data),half(0),half(0),half(1.0f));
        }
    };
    template<>
    struct  TConvert<float2,    half4> 
    {
        static  half4   convert(const float2& data) 
        {
            return  half4(   half(data.r)
                            ,half(data.g)
                            ,half(0)
                            ,half(1.0f));
        }
    };
    template<>
    struct  TConvert<float3,    half4> 
    {
        static  half4   convert(const float3& data) 
        {
            return  half4(   half(data.r)
                            ,half(data.g)
                            ,half(data.b)
                            ,half(1.0f));
        }
    };
    template<>
    struct  TConvert<float4,    half4> 
    {
        static  half4   convert(const float4& data) 
        {
            return  half4(   half(data.r)
                            ,half(data.g)
                            ,half(data.b)
                            ,half(data.a));
        }
    };
    template<>
    struct  TConvert<float,     half3> 
    {
        static  half3   convert(const float & data) 
        {
            return  half3(half(data),half(0),half(1.0f));
        }
    };
    template<>
    struct  TConvert<float2,    half3> 
    {
        static  half3   convert(const float2& data) 
        {
            return  half3(   half(data.r)
                            ,half(data.g)
                            ,half(0));
        }
    };
    template<>
    struct  TConvert<float3,    half3> 
    {
        static  half3   convert(const float3& data) 
        {
            return  half3(   half(data.r)
                            ,half(data.g)
                            ,half(data.b));
        }
    };
    template<>
    struct  TConvert<float4,    half3> 
    {
        static  half3   convert(const float4& data) 
        {
            return  half3(   half(data.r)
                            ,half(data.g)
                            ,half(data.b));
        }
    };

    template<>
    struct  TConvert<float4,    FEA2b10g10r10Snorm> 
    {
        static  FEA2b10g10r10Snorm   convert(const float4& data) 
        {
            return  FEA2b10g10r10Snorm(data.r,data.g,data.b,data.a);
        }
    };
    template<>
    struct  TConvert<FEA2b10g10r10Snorm,float4> 
    {
      
        static  float4   convert(const FEA2b10g10r10Snorm& data) 
        {
            return  float4(  float(data.red()     * oneOver511F)
                            ,float(data.green()   * oneOver511F)
                            ,float(data.blue()    * oneOver511F)
                            ,float(data.alpha()));
        }
    };
    template<>
    struct  TConvert<FEA2b10g10r10Snorm,int4> 
    {
        static  int4   convert(const FEA2b10g10r10Snorm& data) 
        {
            return  int4( data.red()  ,data.green(),data.blue() ,data.alpha());
        }
    };
    template<>
    struct  TConvert<int4,      FEA2b10g10r10Snorm> 
    {
        static  FEA2b10g10r10Snorm   convert(const int4& data) 
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


    template<typename From, typename To>
    To  castTo(const From& data) 
    {
        using   VFrom   =   ExtractColorValueType<From>;
        using   VTo     =   ExtractColorValueType<To>;
        return  TConvert<VFrom, VTo>::convert(data);
    }
}


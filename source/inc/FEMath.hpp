#pragma     once

#include    "FEDefine.h"
#include    "FEPlatform.h"

namespace   FE
{
    template<class T>
    class    tvec2
    {
    public:
        union   
        {
            struct { T x, y; };
            struct { T r, g; };
            struct { T s, t; };
            std::array<T,2> _data;
        };

        tvec2( T x = 0, T y = 0)
        {
            this->x =   x;
            this->y =   y;
        }
    };

    template<class T>
    class    tvec3
    {
    public:
        union   
        {
            struct { T x, y, z; };
            struct { T r, g, b; };
            struct { T s, t, p; };
            std::array<T,3> _data;
        };

        tvec3( T x = 0, T y = 0,T z = 0)
        {
            this->x =   x;
            this->y =   y;
            this->z =   z;
        }
    };

    template<class T>
    class    tvec4
    {
    public:
        union   
        {
            struct { T x, y, z, w; };
            struct { T r, g, b, a; };
            struct { T s, t, p, q; };
            std::array<T,4> _data;
        };
        tvec4( T x = 0, T y = 0,T z = 0, T w = 0)
        {
            this->x =   x;
            this->y =   y;
            this->z =   z;
            this->w =   w;
        }
    };

    /// <summary>
    /// 
    /// </summary>
    using   int8x2      =   tvec2<int8>;
    using   uint8x2     =   tvec2<uint8>;
   
    using   int16x2     =   tvec2<int16>;
    using   uint16x2    =   tvec2<uint16>;

    using   short2      =   tvec2<int16>;
    using   ushort2     =   tvec2<uint16>;

    using   int32x2     =   tvec2<int32>;
    using   uint32x2    =   tvec2<uint32>;

    using   int2        =   tvec2<int32>;
    using   uint2       =   tvec2<uint32>;

    using   int64x2     =   tvec2<int64>;
    using   uint64x2    =   tvec2<uint64>;

    using   float2      =   tvec2<float>;
    using   real2       =   tvec2<real>;

    /// <summary>
    /// 
    /// </summary>
    using   int8x3      =   tvec3<int8>;
    using   uint8x3     =   tvec3<uint8>;

    using   char3       =   tvec3<int8>;
    using   byte3       =   tvec3<uint8>;

    using   int16x3     =   tvec3<int16>;
    using   uint16x3    =   tvec3<uint16>;

    using   short3      =   tvec3<int16>;
    using   ushort3     =   tvec3<uint16>;

    using   int32x3     =   tvec3<int32>;
    using   uint32x3    =   tvec3<uint32>;

    using   int3        =   tvec3<int32>;
    using   uint3       =   tvec3<uint32>;

    using   int64x3     =   tvec3<int64>;
    using   uint64x3    =   tvec3<uint64>;

    using   half3       =   tvec3<float16>;
    using   float3      =   tvec3<float>;
    using   real3       =   tvec3<real>;

    /// <summary>
    /// 
    /// </summary>
    using   int8x4      =   tvec4<int8>;
    using   uint8x4     =   tvec4<uint8>;

    using   char4       =   tvec4<int8>;
    using   byte4       =   tvec4<uint8>;

    using   int16x4     =   tvec4<int16>;
    using   uint16x4    =   tvec4<uint16>;

    using   short4      =   tvec4<int16>;
    using   ushort4     =   tvec4<uint16>;

    using   int32x4     =   tvec4<int32>;
    using   uint32x4    =   tvec4<uint32>;

    using   int4        =   tvec4<int32>;
    using   uint4       =   tvec4<uint32>;

    using   int64x4     =   tvec4<int64>;
    using   uint64x4    =   tvec4<uint64>;

    using   half4       =   tvec4<float16>;
    using   float4      =   tvec4<float>;
    using   real4       =   tvec4<real>;


    using   bytes       =   std::vector<uint8>;
    using   byte2s      =   std::vector<uint8x2>;
    using   byte3s      =   std::vector<uint8x3>;
    using   byte4s      =   std::vector<uint8x4>;

    using   ushorts     =   std::vector<uint16>;
    using   ushort2s    =   std::vector<uint16x2>;
    using   ushort3s    =   std::vector<uint16x3>;
    using   ushort4s    =   std::vector<uint16x4>;

    using   ints        =   std::vector<int>;
    using   int2s       =   std::vector<int2>;
    using   int3s       =   std::vector<int3>;
    using   int4s       =   std::vector<int4>;

    using   uints       =   std::vector<uint32>;
    using   uint2s      =   std::vector<uint2>;
    using   uint3s      =   std::vector<uint3>;
    using   uint4s      =   std::vector<uint4>;

    using   floats       =   std::vector<float>;
    using   float2s      =   std::vector<float2>;
    using   float3s      =   std::vector<float3>;
    using   float4s      =   std::vector<float4>;
}


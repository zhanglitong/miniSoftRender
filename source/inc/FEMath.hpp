#pragma     once


#include    "math/FEVec2.hpp"
#include    "math/FEVec3.hpp"
#include    "math/FEVec4.hpp"
#include    "math/FEMat2.hpp"
#include    "math/FEMat3.hpp"
#include    "math/FEMat4.hpp"
#include    "math/FEQuat.hpp"
#include    "math/FEAabb3.hpp"
#include    "math/FERect.hpp"
#include    "math/FEPlane.hpp"
#include    "math/FEFrustum.hpp"
#include    "math/FERay.hpp"

#include    "FEVector.hpp"

namespace   FE
{
    template<class T>
    using   TVector     =   FEVector<T>;

    template<class T>
    using   SVector     =   FEVector<T,uint32_t>;

    /// <summary>
    /// 
    /// </summary>
    using   half        =   float16;
    using   int8x2      =   tvec2<int8>;
    using   uint8x2     =   tvec2<uint8>;
   
    using   int16x2     =   tvec2<int16>;
    using   uint16x2    =   tvec2<uint16>;

    using   short2      =   tvec2<int16>;
    using   ushort2     =   tvec2<uint16>;

    using   half2       =   tvec2<float16>;

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

    using   quatf       =   tquat<float>;
    using   quatr       =   tquat<real>;

    using   aabb3f      =   taabb3<float>;
    using   aabb3r      =   taabb3<real>;
    

    using   mat3        =   tmat3<float>;
    using   dmat3       =   tmat3<double>;
    using   mat3r       =   tmat3<double>;

    using   mat4        =   tmat4<float>;
    using   dmat4       =   tmat4<double>;
    using   mat4r       =   tmat4<real>;

    using   int8s       =   std::vector<int8>;
    using   uint8s      =   std::vector<uint8>;
    using   bytes       =   std::vector<uint8>;
    using   byte2s      =   std::vector<uint8x2>;
    using   byte3s      =   std::vector<uint8x3>;
    using   byte4s      =   std::vector<uint8x4>;
    using   uint8x4s    =   std::vector<uint8x4>;

    using   int16s      =   std::vector<int16>;
    using   uint16s     =   std::vector<uint16>;
    using   ushorts     =   std::vector<uint16>;
    using   ushort2s    =   std::vector<uint16x2>;
    using   ushort3s    =   std::vector<uint16x3>;
    using   ushort4s    =   std::vector<uint16x4>;

    using   ints        =   std::vector<int>;
    using   int32s      =   std::vector<int>;
                           
    using   int2s       =   std::vector<int2>;
    using   int3s       =   std::vector<int3>;
    using   int4s       =   std::vector<int4>;
                           
    using   uints       =   std::vector<uint32>;
    using   uint32s     =   std::vector<uint32>;

    using   uint2s      =   std::vector<uint2>;
    using   uint3s      =   std::vector<uint3>;
    using   uint4s      =   std::vector<uint4>;
                            
    using   floats      =   std::vector<float>;
    using   float2s     =   std::vector<float2>;
    using   float3s     =   std::vector<float3>;
    using   float4s     =   std::vector<float4>;
                            
    using   reals       =   std::vector<real>;
    using   real2s      =   std::vector<real2>;
    using   real3s      =   std::vector<real3>;
    using   real4s      =   std::vector<real4>;

    using   mat4s       =   std::vector<mat4>;
    using   mat4rs      =   std::vector<mat4r>;

    using   mat3s       =   std::vector<mat3>;
    using   mat3rs      =   std::vector<mat3r>;

    using   uint64s     =   std::vector<uint64>;

    using   uint64ss    =   std::vector<uint64s>;

    using   int64s      =   std::vector<int64>;


    using   uint8ptrs   =   std::vector<uint8*>;
}


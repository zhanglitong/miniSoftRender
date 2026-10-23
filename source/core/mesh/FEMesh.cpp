
#include    "../inc/mesh/FEMesh.hpp"

constexpr const float   constScreenThreshold    =   4;
constexpr const float   constScreenThreshold2   =   constScreenThreshold * constScreenThreshold;
constexpr const float   constDistance2          =   4;

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 拾取支持的顶点位置格式:32bit FLOAT、16bit FLOAT 与 16bit 整型 RGB/RGBA 族
        /// </summary>
        static  bool    isPickupFormat(FEFormat format)
        {
            switch (format)
            {
            case FMT_R32G32B32_FLOAT:
            case FMT_R32G32B32A32_FLOAT:
            case FMT_R16G16B16_FLOAT:
            case FMT_R16G16B16A16_FLOAT:
            case FMT_R16G16B16_UNORM:
            case FMT_R16G16B16_SNORM:
            case FMT_R16G16B16_USCALED:
            case FMT_R16G16B16_SSCALED:
            case FMT_R16G16B16_UINT:
            case FMT_R16G16B16_SINT:
            case FMT_R16G16B16A16_UNORM:
            case FMT_R16G16B16A16_SNORM:
            case FMT_R16G16B16A16_USCALED:
            case FMT_R16G16B16A16_SSCALED:
            case FMT_R16G16B16A16_UINT:
            case FMT_R16G16B16A16_SINT:
                return  true;
            default:
                return  false;
            }
        }

        /// <summary>
        /// 16bit 整型位置向量读取:按转换器逐分量还原,忽略 A 分量
        /// </summary>
        template<typename V,typename Conv>
        static  real3   readVec3(const uint8* pData,Conv conv)
        {
            const   V*  p   =   (const V*)pData;
            return  real3(conv(p->x),conv(p->y),conv(p->z));
        }

        /// <summary>
        /// 按顶点格式读取位置,统一转换为 real3(忽略 A 分量)
        /// _UNORM/_SNORM 归一化到[0,1]/[-1,1];_USCALED/_UINT、_SSCALED/_SINT 按原值
        /// </summary>
        static  real3   readPosition(const uint8* pData,FEFormat format)
        {
            switch (format)
            {
            case FMT_R32G32B32_FLOAT:
                {
                    const   float3* p   =   (const float3*)pData;
                    return  real3(p->x,p->y,p->z);
                }
            case FMT_R32G32B32A32_FLOAT:
                {
                    const   float4* p   =   (const float4*)pData;
                    return  real3(p->x,p->y,p->z);
                }
            case FMT_R16G16B16_FLOAT:
                {
                    const   half3*  p   =   (const half3*)pData;
                    return  real3(float(p->x),float(p->y),float(p->z));
                }
            case FMT_R16G16B16A16_FLOAT:
                {
                    const   half4*  p   =   (const half4*)pData;
                    return  real3(float(p->x),float(p->y),float(p->z));
                }
            /// 16bit 无符号归一化:[0,65535] -> [0,1]
            case FMT_R16G16B16_UNORM:
                return  readVec3<ushort3>(pData,[](uint16 v) { return  real(v) / 65535.0; });
            case FMT_R16G16B16A16_UNORM:
                return  readVec3<ushort4>(pData,[](uint16 v) { return  real(v) / 65535.0; });
            /// 16bit 有符号归一化:[-32767,32767] -> [-1,1]
            case FMT_R16G16B16_SNORM:
                return  readVec3<short3>(pData,[](int16 v) { real r = real(v) / 32767.0; return  r < -1.0 ? -1.0 : r; });
            case FMT_R16G16B16A16_SNORM:
                return  readVec3<short4>(pData,[](int16 v) { real r = real(v) / 32767.0; return  r < -1.0 ? -1.0 : r; });
            /// 16bit 无符号整数族(USCALED/UINT):原值
            case FMT_R16G16B16_USCALED:
            case FMT_R16G16B16_UINT:
                return  readVec3<ushort3>(pData,[](uint16 v) { return  real(v); });
            case FMT_R16G16B16A16_USCALED:
            case FMT_R16G16B16A16_UINT:
                return  readVec3<ushort4>(pData,[](uint16 v) { return  real(v); });
            /// 16bit 有符号整数族(SSCALED/SINT):原值
            case FMT_R16G16B16_SSCALED:
            case FMT_R16G16B16_SINT:
                return  readVec3<short3>(pData,[](int16 v) { return  real(v); });
            case FMT_R16G16B16A16_SSCALED:
            case FMT_R16G16B16A16_SINT:
                return  readVec3<short4>(pData,[](int16 v) { return  real(v); });
            default:
                return  real3(0);
            }
        }
    }

    int     FEMesh::drawCmd(FECmdIndex& cmd,uint idx,uint instId)
    {
        assert(idx < _primitives.size());
        if (_primitives.empty() || idx >= _primitives.size())
            return  false;
        auto&   pri     =   _primitives[idx];
        switch(pri->type())
        {
        case EDrawType::DRAW_ARRAY:
            {
                /// 注意: DrawElement 比 DrawArray 大，所以这里统一用DrawElement 存储数据
                /// 在绘制的时候区分是 DrawArray/DrawElement
                auto        pDraw   =   (FEDrawArray*)(pri.get());
                cmd.count           =   pDraw->count();
                cmd.primCount       =   1;
                cmd.firstIndex      =   pDraw->start() + pri->vertexOffset();
                cmd.baseVertex      =   instId;
                /// cs 拾取中会使用到 /compute shader 已经处理
                cmd.baseInstance    =   instId;
                return  2;
            }
            break;
        case EDrawType::DRAW_ELEMENT_UINT8:
            {
                auto    pDraw       =   pri->as<FEDrawElementUint8>();
                cmd.baseInstance    =   instId;
                cmd.baseVertex      =   pri->vertexOffset();
                cmd.count           =   (uint)pDraw->count();
                cmd.firstIndex      =   pDraw->indexOffset();
                cmd.primCount       =   1;
                return  1;
            }
            break;
        case EDrawType::DRAW_ELEMENT_UINT16:
            {
                auto    pDraw       =   pri->as<FEDrawElementUint16>();
                cmd.baseInstance    =   instId;
                cmd.baseVertex      =   pri->vertexOffset();
                cmd.count           =   (uint)pDraw->count();
                cmd.firstIndex      =   pDraw->indexOffset();
                cmd.primCount       =   1;
                return  1;
            }
            break;
        case EDrawType::DRAW_ELEMENT_UINT32:
            {
                auto    pDraw       =   pri->as<FEDrawElementUint32>();
                cmd.baseInstance    =   instId;
                cmd.baseVertex      =   pri->vertexOffset();
                cmd.count           =   (uint)pDraw->count();
                cmd.firstIndex      =   pDraw->indexOffset();
                cmd.primCount       =   1;
                return  1;
            }
            break;
        }
        return  0;
    }

    bool    FEMesh::intersect(const Ray& raySrc,const mat4r& matSrc,FEPickup& result) const
    {
        auto        mat     =   FE::inverse(matSrc);
        Ray         ray     =   raySrc.transformed(mat);

        FEMesh*     pSelf   =   const_cast<FEMesh*>(this);
        for(auto& var : _primitives)
        {
            switch(var->type())
            {
            case DRAW_ARRAY          :
                if(pSelf->intersect(ray,var->as<FEDrawArray>(),mat,result))
                    return  true;
                break;
            case DRAW_ELEMENT_UINT8  :
                if(pSelf->intersect(ray,var->as<FEDrawElementUint8>(),mat,result))
                    return  true;
                break;
            case DRAW_ELEMENT_UINT16 :
                if(pSelf->intersect(ray,var->as<FEDrawElementUint16>(),mat,result))
                    return  true;
                break;
            case DRAW_ELEMENT_UINT32 :
                if(pSelf->intersect(ray,var->as<FEDrawElementUint32>(),mat,result))
                    return  true;
                break;
            }
        }
        return  false;
    }

    bool    FEMesh::intersect(const Ray& ray, const FEDrawArray* pri,       const mat4r& , FEPickup& result) 
    {
        auto    pSubMesh    =   get(IS_VERTEX_POS);
        if (pSubMesh == nullptr)
            return  false;
        auto    attr        =   pSubMesh->attr();

        /// 顶点格式校验:32bit FLOAT、16bit FLOAT 与 16bit 整型 RGB/RGBA 族
        auto    format      =   attr.format();
        if (!isPickupFormat(format))
            return  false;
        auto    stride      =   attr.stride();
        auto    pData       =   pSubMesh->buffer()->data();
        if (pData == nullptr)
            return  false;
        uint    nStart      =   pri->start();
        uint    nEnd        =   pri->start() + pri->count();

        real    minDist2    =   FLT_MAX;
        real    time        =   result.time;
        real3   point;
        bool    pickup      =   false;
        switch(pri->primitive())
        {
        case PRI_POINTS          :
            {}
            break;
        case PRI_LINES:
            {
                for (uint i = nStart; i < nEnd; i+= 2)
                {
                    real3   dpos0   =   readPosition(pData + stride * (i + 0),format);
                    real3   dpos1   =   readPosition(pData + stride * (i + 1),format);

                    real3   seg[]   =   {dpos0,dpos1};
                    real3   ptRay;
                    real3   ptSet;
                    auto    dist    =   ray.distance2(seg,&ptSet,&ptRay);
                    if (dist.x > constDistance2)
                        continue;
                    if (dist.x > minDist2)
                        continue;
                    minDist2        =   dist.x;
                    time            =   dist.y; 
                    point           =   ptSet;
                    pickup          =   true;
                    result.pickup   =   FEPickup::PR_Line;
                }
            }
            break;
        case PRI_LINE_STRIP:
            {
                real3   dpos0   =   readPosition(pData + stride,format);
                for (uint i = nStart + 1; i < nEnd; ++i)
                {
                    real3   dpos1   =   readPosition(pData + stride * i,format);
                    real3   seg[]   =   {dpos0,dpos1};
                    real3   ptRay;
                    real3   ptSet;
                    auto    dist    =   ray.distance2(seg,&ptSet,&ptRay);
                    /// 距离符合要求,time 符合要求
                    if (dist.x < constDistance2 && dist.y < time)
                    {
                        time            =   dist.y;
                        point           =   ptSet;
                        pickup          =   true;
                        result.pickup   =   FEPickup::PR_Line;
                    }
                    dpos0   =   dpos1;
                }
            }
            break;
        case PRI_TRIANGLES:
            {
                for (uint i = nStart; i < nEnd; i+= 3)
                {
                    real3   dpos0   =   readPosition(pData + stride * (i + 0),format);
                    real3   dpos1   =   readPosition(pData + stride * (i + 1),format);
                    real3   dpos2   =   readPosition(pData + stride * (i + 2),format);

                    real   t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                        continue;
                    /// 相机在被拾取对象的背面
                    if (t < 0)
                        continue;
                    if (t > time)
                        continue;
                    pickup          =   true;
                    time            =   t;
                    point           =   ray.getPoint(time);
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        case PRI_TRIANGLE_STRIP  :
            {
                real3   dpos0   =   readPosition(pData,format);
                real3   dpos1   =   readPosition(pData + stride,format);

                for (uint i = nStart + 2; i < nEnd; ++ i)
                {
                    real3   dpos2   =   readPosition(pData + stride * i,format);

                    real   t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                    {
                        dpos0   =   dpos1;
                        dpos1   =   dpos2;
                        continue;
                    }
                    /// 相机在被拾取对象的背面
                    if (t < 0)
                    {
                        dpos0   =   dpos1;
                        dpos1   =   dpos2;
                        continue;
                    }
                    dpos0   =   dpos1;
                    dpos1   =   dpos2;
                    if (time < t)
                        continue;
                    pickup          =   true;
                    time            =   t;
                    point           =   ray.getPoint(time);
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        case PRI_TRIANGLE_FAN    : 
            {
                real3   dpos0   =   readPosition(pData,format);
                real3   dpos1   =   readPosition(pData + stride,format);

                for (uint i = nStart + 2; i < nEnd; ++ i)
                {
                    real3   dpos2   =   readPosition(pData + stride * i,format);

                    real   t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                    {
                        dpos1   =   dpos2;
                        continue;
                    }
                    dpos1   =   dpos2;
                    /// 相机在被拾取对象的背面
                    if (t < 0)
                        continue;
                    if (time < t)
                        continue;
                    pickup          =   true;
                    time            =   t;
                    dpos1           =   dpos2;
                    point           =   ray.getPoint(time);
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        }
        if (pickup)
        {
            result.object   =   this;
            result.distance =   time;
            result.point    =   point;
            result.time     =   time;
        }
        return  pickup;
    }
    bool    FEMesh::intersect(const Ray& ray, const FEDrawElementUint8*pri, const mat4r& , FEPickup& result) 
    {
        /// 获取顶点缓冲区数据
        auto    pBuffer =   get(IS_VERTEX_POS);
        if (pBuffer == nullptr)
            return  false;
        /// 获取顶点属性
        auto    attr        =   pBuffer->attr();
        /// 顶点格式校验:32bit FLOAT、16bit FLOAT 与 16bit 整型 RGB/RGBA 族
        auto    format      =   attr.format();
        if (!isPickupFormat(format))
            return  false;
        auto    stride      =   attr.stride();
        auto    pData       =   pBuffer->buffer()->data();
        if (pData == nullptr)
            return  false;
        real    time        =   FLT_MAX;
        real    dist        =   FLT_MAX;
        real3   point;
        bool    pickup      =   false;
        auto&   pIndex      =   pri->index();
        uint    nCount      =   (uint)pIndex.size();
        switch(pri->primitive())
        {
        case PRI_POINTS          :
            break;
        case PRI_LINES           :
            {
                for (size_t i = 0; i < nCount; i += 2)
                {
                    uint    index0  =   pIndex[i + 0];
                    uint    index1  =   pIndex[i + 1];

                    real3   dpos0   =   readPosition(pData + stride * index0,format);
                    real3   dpos1   =   readPosition(pData + stride * index1,format);
                    real3   seg[]   =   {dpos0,dpos1};
                    real3   ptRay;
                    real3   ptSet;

                    if (dpos0 == dpos1)
                        continue;
                    auto    res    =   ray.distance2(seg,&ptSet,&ptRay);
                    if (res.x > constDistance2)
                        continue;
                    if (res.y > time)
                        continue;
                    time            =   res.y;
                    point           =   ptSet;
                    pickup          =   true;
                    dist            =   FE::distance(ptSet,ray.origin());
                    result.pickup   =   FEPickup::PR_Line;
                }
            }
            break;
        case PRI_TRIANGLES       :
            {
                for (size_t i = 0; i < nCount; i += 3)
                {
                    byte3   index   =   byte3(pIndex[i+0],pIndex[i+1],pIndex[i+2]);
                    real3   dpos0   =   readPosition(pData +index.x * stride,format);
                    real3   dpos1   =   readPosition(pData +index.y * stride,format);
                    real3   dpos2   =   readPosition(pData +index.z * stride,format);
                    real    t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                        continue;
                    if (t < 0)
                        continue;
                    if (time < t)
                        continue;
                    time            =   t;
                    point           =   ray.getPoint(t);
                    pickup          =   true;
                    dist            =   t;    
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        }
        if (pickup)
        {
            result.object   =   this;
            result.distance =   dist;
            result.point    =   point;
            result.time     =   time;
        }
        return  pickup;
    }
    bool    FEMesh::intersect(const Ray& ray, const FEDrawElementUint16*pri,const mat4r& , FEPickup& result) 
    {
        /// 获取顶点缓冲区数据
        auto    pBuffer =   get(IS_VERTEX_POS);
        if (pBuffer == nullptr)
            return  false;
        /// 获取顶点属性
        auto    attr        =   pBuffer->attr();
        /// 顶点格式校验:32bit FLOAT、16bit FLOAT 与 16bit 整型 RGB/RGBA 族
        auto    format      =   attr.format();
        if (!isPickupFormat(format))
            return  false;
        auto    stride      =   attr.stride();
        auto    pData       =   pBuffer->buffer()->data();
        if (pData == nullptr)
            return  false;
        real    time        =   FLT_MAX;
        real    dist        =   FLT_MAX;
        real3   point;
        bool    pickup      =   false;
        auto&   pIndex      =   pri->index();
        uint    nCount      =   (uint)pIndex.size();
        switch(pri->primitive())
        {
        case PRI_POINTS          :
            break;
        case PRI_LINES           :
            {
                for (size_t i = 0; i < nCount; i += 2)
                {
                    uint    index0  =   pIndex[i + 0];
                    uint    index1  =   pIndex[i + 1];

                    real3   dpos0   =   readPosition(pData + stride * index0,format);
                    real3   dpos1   =   readPosition(pData + stride * index1,format);
                    real3   seg[]   =   {dpos0,dpos1};
                    real3   ptRay;
                    real3   ptSet;

                    if (dpos0 == dpos1)
                        continue;
                    auto    res    =   ray.distance2(seg,&ptSet,&ptRay);
                    if (res.x > constDistance2)
                        continue;
                    if (res.y > time)
                        continue;
                    time            =   res.y;
                    point           =   ptSet;
                    pickup          =   true;
                    dist            =   FE::distance(ptSet,ray.origin());
                    result.pickup   =   FEPickup::PR_Line;
                }
            }
            break;
        case PRI_TRIANGLES       :
            {
                for (size_t i = 0; i < nCount; i += 3)
                {
                    ushort3 index   =   ushort3(pIndex[i+0],pIndex[i+1],pIndex[i+2]);
                    real3   dpos0   =   readPosition(pData +index.x * stride,format);
                    real3   dpos1   =   readPosition(pData +index.y * stride,format);
                    real3   dpos2   =   readPosition(pData +index.z * stride,format);
                    real    t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                        continue;
                    if (t < 0)
                        continue;
                    if (time < t)
                        continue;
                    time            =   t;
                    point           =   ray.getPoint(t);
                    pickup          =   true;
                    dist            =   t;  
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        }
        if (pickup)
        {
            result.object   =   this;
            result.distance =   dist;
            result.point    =   point;
            result.time     =   time;
        }
        return  pickup;
    }
    bool    FEMesh::intersect(const Ray& ray, const FEDrawElementUint32*pri,const mat4r& , FEPickup& result) 
    {
        /// 获取顶点缓冲区数据
        auto    pBuffer =   get(IS_VERTEX_POS);
        if (pBuffer == nullptr)
            return  false;
        /// 获取顶点属性
        auto    attr        =   pBuffer->attr();
        /// 顶点格式校验:32bit FLOAT、16bit FLOAT 与 16bit 整型 RGB/RGBA 族
        auto    format      =   attr.format();
        if (!isPickupFormat(format))
            return  false;
        auto    stride      =   attr.stride();
        auto    pData       =   pBuffer->buffer()->data();
        if (pData == nullptr)
            return  false;
        real    time        =   FLT_MAX;
        real    dist        =   FLT_MAX;
        real3   point;
        bool    pickup      =   false;
        auto&   pIndex      =   pri->index();
        uint    nCount      =   (uint)pIndex.size();
        switch(pri->primitive())
        {
        case PRI_POINTS          :
            break;
        case PRI_LINES           :
            {
                for (size_t i = 0; i < nCount; i += 2)
                {
                    uint    index0  =   pIndex[i + 0];
                    uint    index1  =   pIndex[i + 1];

                    real3   dpos0   =   readPosition(pData + stride * index0,format);
                    real3   dpos1   =   readPosition(pData + stride * index1,format);
                    real3   seg[]   =   {dpos0,dpos1};
                    real3   ptRay;
                    real3   ptSet;

                    if (dpos0 == dpos1)
                        continue;
                    auto    res    =   ray.distance2(seg,&ptSet,&ptRay);
                    if (res.x > constDistance2)
                        continue;
                    if (res.y > time)
                        continue;
                    time            =   res.y;
                    point           =   ptSet;
                    pickup          =   true;
                    dist            =   FE::distance(ptSet,ray.origin());
                    result.pickup   =   FEPickup::PR_Line;
                }
            }
            break;
        case PRI_TRIANGLES       :
            {
                for (size_t i = 0; i < nCount; i += 3)
                {
                    ushort3 index   =   ushort3(pIndex[i+0],pIndex[i+1],pIndex[i+2]);
                    real3   dpos0   =   readPosition(pData +index.x * stride,format);
                    real3   dpos1   =   readPosition(pData +index.y * stride,format);
                    real3   dpos2   =   readPosition(pData +index.z * stride,format);
                    real    t(0), u(0), v(0);
                    if (!ray.intersect(dpos0, dpos1, dpos2, &t, &u, &v))
                        continue;
                    if (t < 0)
                        continue;
                    if (time < t)
                        continue;
                    time            =   t;
                    point           =   ray.getPoint(t);
                    pickup          =   true;
                    dist            =   t;  
                    result.pickup   =   FEPickup::PR_Triangle;
                }
            }
            break;
        }
        if (pickup)
        {
            result.object   =   this;
            result.distance =   dist;
            result.point    =   point;
            result.time     =   time;
        }
        return  pickup;
    }
}


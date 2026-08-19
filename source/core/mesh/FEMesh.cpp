
#include    "../inc/mesh/FEMesh.hpp"

constexpr const float   constScreenThreshold    =   4;
constexpr const float   constScreenThreshold2   =   constScreenThreshold * constScreenThreshold;
constexpr const float   constDistance2          =   4;

namespace   FE
{
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
                cmd.baseInstance    =   0xFFFFFFFF;
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

        /// 顶点格式必须是以下两种
        switch (attr.format())
        {
        case FMT_R32G32B32_FLOAT:
        case FMT_R32G32B32A32_FLOAT:
            break;
        default:
            return  false;
        }
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
                    float3* p0      =   (float3*)(pData + stride * (i + 0));
                    float3* p1      =   (float3*)(pData + stride * (i + 1));

                    real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                    real3   dpos1   =   real3(p1->x,p1->y,p1->z);
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
                float3* p0      =   (float3*)(pData + stride);
                real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                for (uint i = nStart + 1; i < nEnd; ++i)
                {
                    float3* p1      =   (float3*)(pData + stride * i);
                    real3   dpos1   =   real3(p1->x,p1->y,p1->z);
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
                    float3* p0      =   (float3*)(pData + stride * (i + 0));
                    float3* p1      =   (float3*)(pData + stride * (i + 1));
                    float3* p2      =   (float3*)(pData + stride * (i + 2));

                    real3  dpos0    =   real3(p0->x,p0->y,p0->z);
                    real3  dpos1    =   real3(p1->x,p1->y,p1->z);
                    real3  dpos2    =   real3(p2->x,p2->y,p2->z);

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
                float3* p0      =   (float3*)(pData + stride * 0);
                float3* p1      =   (float3*)(pData + stride * 1);

                real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                real3   dpos1   =   real3(p1->x,p1->y,p1->z);

                for (uint i = nStart + 2; i < nEnd; ++ i)
                {
                    float3* p2      =   (float3*)(pData + stride * i);
                    real3   dpos2   =   real3(p2->x,p2->y,p2->z);

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
                float3* p0      =   (float3*)(pData + stride * 0);
                float3* p1      =   (float3*)(pData + stride * 1);

                real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                real3   dpos1   =   real3(p1->x,p1->y,p1->z);

                for (uint i = nStart + 2; i < nEnd; ++ i)
                {
                    float3* p2      =   (float3*)(pData + stride * i);
                    real3   dpos2   =   real3(p2->x,p2->y,p2->z);

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
        /// 顶点格式必须是以下两种
        switch (attr.format())
        {
        case FMT_R32G32B32_FLOAT:
        case FMT_R32G32B32A32_FLOAT:
            break;
        default:
            return  false;
        }
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

                    float3* p0      =   (float3*)(pData + stride * index0 );
                    float3* p1      =   (float3*)(pData + stride * index1 );

                    real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                    real3   dpos1   =   real3(p1->x,p1->y,p1->z);
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
                    float3  pos0    =   *(float3*)(pData +index.x * stride);
                    float3  pos1    =   *(float3*)(pData +index.y * stride);
                    float3  pos2    =   *(float3*)(pData +index.z * stride);
                    real3   dpos0   =   real3(pos0.x, pos0.y, pos0.z);
                    real3   dpos1   =   real3(pos1.x, pos1.y, pos1.z);
                    real3   dpos2   =   real3(pos2.x, pos2.y, pos2.z);
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
        /// 顶点格式必须是以下两种
        switch (attr.format())
        {
        case FMT_R32G32B32_FLOAT:
        case FMT_R32G32B32A32_FLOAT:
            break;
        default:
            return  false;
        }
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

                    float3* p0      =   (float3*)(pData + stride * index0 );
                    float3* p1      =   (float3*)(pData + stride * index1 );

                    real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                    real3   dpos1   =   real3(p1->x,p1->y,p1->z);
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
                    float3  pos0    =   *(float3*)(pData +index.x * stride);
                    float3  pos1    =   *(float3*)(pData +index.y * stride);
                    float3  pos2    =   *(float3*)(pData +index.z * stride);
                    real3   dpos0   =   real3(pos0.x, pos0.y, pos0.z);
                    real3   dpos1   =   real3(pos1.x, pos1.y, pos1.z);
                    real3   dpos2   =   real3(pos2.x, pos2.y, pos2.z);
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
        /// 顶点格式必须是以下两种
        switch (attr.format())
        {
        case FMT_R32G32B32_FLOAT:
        case FMT_R32G32B32A32_FLOAT:
            break;
        default:
            return  false;
        }
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

                    float3* p0      =   (float3*)(pData + stride * index0 );
                    float3* p1      =   (float3*)(pData + stride * index1 );

                    real3   dpos0   =   real3(p0->x,p0->y,p0->z);
                    real3   dpos1   =   real3(p1->x,p1->y,p1->z);
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
                    float3  pos0    =   *(float3*)(pData +index.x * stride);
                    float3  pos1    =   *(float3*)(pData +index.y * stride);
                    float3  pos2    =   *(float3*)(pData +index.z * stride);
                    real3   dpos0   =   real3(pos0.x, pos0.y, pos0.z);
                    real3   dpos1   =   real3(pos1.x, pos1.y, pos1.z);
                    real3   dpos2   =   real3(pos2.x, pos2.y, pos2.z);
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


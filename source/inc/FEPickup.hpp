#pragma     once
#include    "FEObject.h"
namespace FE
{
    /// <summary>
    /// 内存管理
    /// </summary>
    struct  FEPickup
    {
        enum  Result :uint32
        {
            PR_None         =   0,
            PR_Point        =   1,
            PR_Line         =   1,
            PR_Triangle     =   2,
        };
        /// <summary>
        /// 是否是拾取到了box,其他的是点线面
        /// 1: 包围盒,2,实体
        /// </summary>
        Result      pickup      =   PR_None;
        /// <summary>
        /// 被拾取对象,如果== null 没有拾取到
        /// </summary>
        Object      object;
        /// <summary>
        /// 从射线起点到拾取点的距离
        /// </summary>
        real        distance    =   FLT_MAX;
        /// <summary>
        /// 拾取到的世界坐标点
        /// </summary>
        real3       point;
        /// <summary>
        /// 射线的时间T
        /// </summary>
        real        time        =   FLT_MAX;
        
    };
    using   Pickups     =   std::vector<FEPickup>; 
}



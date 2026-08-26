#pragma     once

#include    "FEPlane.hpp"

namespace   FE
{
    template<class T>
    class   tfrustum
    {
    public:
        enum
        {
            FRUSTUM_LEFT    =   0,
            FRUSTUM_RIGHT   =   1,
            FRUSTUM_BOTTOM  =   2,
            FRUSTUM_TOP     =   3,
            FRUSTUM_NEAR    =   4,
            FRUSTUM_FAR     =   5,
        };
    public:
        /// <summary>
        /// 根据vp计算视锥
        /// Gribb-Hartmann 方法，用于从视图-投影矩阵（View-Projection Matrix）中提取视锥体平面
        /// </summary>
        /// <param name="mvp"></param>
        inline  void    loadFrustum(const tmat4<T> &vp)
        {
            const T*  dataPtr = (T*)&vp[0];
            _planes[FRUSTUM_LEFT]   =   tplane<T>(dataPtr[3] + dataPtr[0], dataPtr[7] + dataPtr[4], dataPtr[11] + dataPtr[8], dataPtr[15] + dataPtr[12]);
            _planes[FRUSTUM_RIGHT]  =   tplane<T>(dataPtr[3] - dataPtr[0], dataPtr[7] - dataPtr[4], dataPtr[11] - dataPtr[8], dataPtr[15] - dataPtr[12]);

            _planes[FRUSTUM_BOTTOM] =   tplane<T>(dataPtr[3] + dataPtr[1], dataPtr[7] + dataPtr[5], dataPtr[11] + dataPtr[9], dataPtr[15] + dataPtr[13]);
            _planes[FRUSTUM_TOP]    =   tplane<T>(dataPtr[3] - dataPtr[1], dataPtr[7] - dataPtr[5], dataPtr[11] - dataPtr[9], dataPtr[15] - dataPtr[13]);

            _planes[FRUSTUM_NEAR]   =   tplane<T>(dataPtr[3] + dataPtr[2], dataPtr[7] + dataPtr[6], dataPtr[11] + dataPtr[10], dataPtr[15] + dataPtr[14]);
            _planes[FRUSTUM_FAR]    =   tplane<T>(dataPtr[3] - dataPtr[2], dataPtr[7] - dataPtr[6], dataPtr[11] - dataPtr[10], dataPtr[15] - dataPtr[14]);

        }
        /// <summary>
        /// 判断一个点在视锥内
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        inline  bool    pointInFrustum(const tvec3<T> &pos) const
        {
            for (int i = 0; i < 6; i++)
            {
                if (_planes[i].distance(pos) <= 0)
                    return false;
            }
            return true;
        }
        /// <summary>
        /// 判断球与视锥是否相交
        /// </summary>
        /// <param name="pos"></param>
        /// <param name="radius"></param>
        /// <returns></returns>
        inline  bool    sphereInFrustum(const tvec3<T> &pos, const T radius) const
        {
            for (int i = 0; i < 6; i++)
            {
                if (_planes[i].distance(pos) <= -radius)
                    return false;
            }
            return true;
        }
        /// <summary>
        /// 判断轴向包围盒是否与视锥相交
        /// </summary>
        /// <param name="vMin"></param>
        /// <param name="vMax"></param>
        /// <returns></returns>
        inline  bool    boxInFrustum(const tvec3<T> &vMin,const tvec3<T> &vMax) const
        {
#if 0
            for (int i = 0; i < 6; i++)
            {
                if (_planes[i].distance(tvec3<T>(vMin.x, vMin.y, vMin.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMin.x, vMin.y, vMax.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMin.x, vMax.y, vMin.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMin.x, vMax.y, vMax.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMax.x, vMin.y, vMin.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMax.x, vMin.y, vMax.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMax.x, vMax.y, vMin.z)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(vMax.x, vMax.y, vMax.z)) > 0) continue;
                return false;
            }
            return true;
#else
            /// 
            /// 原理说明:
            /// 为了快速判断，不需要测试 AABB 的所有 8 个顶点。
            /// 只需要找到在法线方向上偏移最大的顶点(即 positive 点)
            /// 如果法线的 x 分量是正的，说明沿着 x 轴正方向走，点会越来越靠近平面正面。所以选 vMax.x。
            /// 反之，如果 x 分量是负的，说明沿着 x 轴负方向走，点反而更靠近平面正面。所以选 vMin.x。
            /// 对 y 和 z 轴同理
            /// 通过这种方式选出的 positive 顶点，是整个 AABB 中沿着法线方向走得最远、最深入平面内部的点
            /// 逻辑：如果连这个“最靠近内部”的点都在平面的背面(即距离 < 0 )，那么包围盒的其他 7 个点必然也在背面。
            /// 
            for (const auto& plane : _planes)
            {
                /// 测试包围盒最靠近平面正方向的点
                auto  positive  = vMin;
                if (plane._normal.x >= 0) positive.x = vMax.x;
                if (plane._normal.y >= 0) positive.y = vMax.y;
                if (plane._normal.z >= 0) positive.z = vMax.z;

                if (plane.distance(positive) < 0)
                    return  false;
            }
            return  true;
#endif
        }
        /// <summary>
        /// 同上
        /// </summary>
        /// <param name="minX"></param>
        /// <param name="maxX"></param>
        /// <param name="minY"></param>
        /// <param name="maxY"></param>
        /// <param name="minZ"></param>
        /// <param name="maxZ"></param>
        /// <returns></returns>
        inline  bool    cubeInFrustum(T minX, T maxX, T minY, T maxY, T minZ, T maxZ) const
        {
            for (int i = 0; i < 6; i++)
            {
                if (_planes[i].distance(tvec3<T>(minX, minY, minZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(minX, minY, maxZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(minX, maxY, minZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(minX, maxY, maxZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(maxX, minY, minZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(maxX, minY, maxZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(maxX, maxY, minZ)) > 0) continue;
                if (_planes[i].distance(tvec3<T>(maxX, maxY, maxZ)) > 0) continue;
                return false;
            }
            return true;
        }
        template<class TT>
        inline  void    convertTo(tvec4<TT> out[6])
        {
            for (int i = 0; i < 6; ++i)
            {
                out[i].x = (TT)_planes[i]._normal.x;
                out[i].y = (TT)_planes[i]._normal.y;
                out[i].z = (TT)_planes[i]._normal.z;
                out[i].w = (TT)_planes[i]._distance;
            }
        }
        /// <summary>
        /// 获取面
        /// </summary>
        /// <param name="plane"></param>
        /// <returns></returns>
        const   auto&   getPlane(const int plane) const
        {
            return _planes[plane];
        }
        /// <summary>
        /// 从变换矩阵提取视锥体 8 个顶点
        /// </summary>
        /// <param name="invVP">视图-投影矩阵的逆矩阵 (inverse(Projection * View)</param>
        /// <param name="outCorners">输出的世界空间 8 个顶点</param>
        inline  void    extractCorners(const tmat4<T>& invVP, tvec3<T> outCorners[8]) const
        {
            /// 1. 定义 NDC 空间中的 8 个立方体顶点 (注意 Z 的范围)
            /// 顺序通常为：近平面 4 点 -> 远平面 4 点
            tvec4<T> ndcCorners[8] =
            {
                {-1, -1, -1, 1}, { 1, -1, -1, 1}, { 1,  1, -1, 1}, {-1,  1, -1, 1}, /// 近平面 (Z = -1)
                {-1, -1,  1, 1}, { 1, -1,  1, 1}, { 1,  1,  1, 1}, {-1,  1,  1, 1}  /// 远平面 (Z = +1)
            };
            for (int i = 0; i < 8; ++i)
            {
                /// 2. 变换到裁剪空间/世界空间
                const tvec4<T>  worldPt =   invVP * ndcCorners[i];
                const T         wInv    =   T(1)/worldPt.w;
                /// 3. 必须进行透视除法 (Perspective Divide)
                /// 这是将齐次坐标转回三维空间坐标的关键
                outCorners[i]           =   worldPt.xyz() * wInv;
            }
        }
        /// <summary>
        /// 获取边的索引数据
        /// </summary>
        /// <param name="edges"></param>
        inline  void    extractEdgeIndex(uint8 edges[]) const
        {
            byte    datas[]  =
            {
                1 ,0, // 近
                2 ,1, // 近
                3 ,2, // 近
                0 ,3, // 近

                5 ,4, // 远
                6 ,5, // 远
                7 ,6, // 远
                4 ,7, // 远

                4 ,0, // 侧棱1
                5 ,1, // 侧棱2
                6 ,2, // 侧棱3
                7 ,3  // 侧棱4
            };
            memcpy(edges,datas,sizeof(datas));
        }
        /// <summary>
        /// 根据视锥的八个角点获取边的，目的是做分离轴算法
        /// </summary>
        /// <param name="corners">视锥体的八个点</param>
        /// <param name="edgeDirs">输出12个边(24个顶点)</param>
        inline  void    extractEdges(const tvec3<T> corners[8],tvec3<T> edges[24]) const
        {
            tvec3<T>    datas[] =
            {
                corners[1]  ,corners[0], // 近
                corners[2]  ,corners[1], // 近
                corners[3]  ,corners[2], // 近
                corners[0]  ,corners[3], // 近

                corners[5]  ,corners[4], // 远
                corners[6]  ,corners[5], // 远
                corners[7]  ,corners[6], // 远
                corners[4]  ,corners[7], // 远

                corners[4]  ,corners[0], // 侧棱1
                corners[5]  ,corners[1], // 侧棱2
                corners[6]  ,corners[2], // 侧棱3
                corners[7]  ,corners[3]  // 侧棱4
            };
            memcpy(edges,datas,sizeof(datas));
        }
    public:
        tplane<T>    _planes[6];
    };

    using   FrustumF    =   tfrustum<float>;
    using   FrustumR    =   tfrustum<real>;
}

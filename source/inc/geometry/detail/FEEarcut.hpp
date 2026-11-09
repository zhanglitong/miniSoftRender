#pragma     once

#include    "../../fileFormat/svg/FESVGTypes.hpp"
#include    <algorithm>
#include    <cmath>
#include    <cfloat>

namespace   FE
{
    /// <summary>
    /// 多边形三角化(带洞),算法参考 three.js 所使用的 earcut:
    /// 1. 统一外轮廓为逆时针、洞为顺时针;
    /// 2. 依次为每个洞找到与外轮廓的可见连接点,把洞"桥接"进外轮廓;
    /// 3. 使用耳切法(ear clipping)输出三角形索引。
    /// 输出索引指向外部传入点集的拼接结果,面朝向由调用方按世界坐标再修正。
    /// </summary>
    class   FEEarcut
    {
    public:
        /// <summary>
        /// 三角化多边形
        /// </summary>
        /// <param name="outer">外轮廓(首尾不重复)</param>
        /// <param name="holes">洞集合(每个首尾不重复)</param>
        /// <param name="outPoints">可选,输出索引对应的点集(外轮廓 + 按处理顺序的洞)</param>
        /// <returns>三角形索引,每 3 个一组</returns>
        static  uints   triangulate(const float2s& outer,const std::vector<float2s>& holes,
                                    float2s* outPoints = nullptr)
        {
            uints   result;
            if (outer.size() < 3)
                return  result;

            /// 拼接所有点到统一数组,ring 保存索引环
            float2s         points;
            std::vector<int>    ring;
            points.reserve(outer.size() + 64);
            for (const auto& p : outer)
            {
                ring.push_back((int)points.size());
                points.push_back(p);
            }
            if (fesvgSignedArea2(outer) < 0.0f)
                std::reverse(ring.begin(),ring.end());

            /// 洞按最右点从大到小处理,减少桥接交叉的概率
            std::vector<int>    order(holes.size());
            for (size_t i = 0; i < holes.size(); ++i)
                order[i]    =   (int)i;
            std::sort(order.begin(),order.end(),[&](int a,int b)
            {
                return  maxX(holes[a]) > maxX(holes[b]);
            });

            for (int holeIndex : order)
            {
                const   float2s&    hole    =   holes[holeIndex];
                if (hole.size() < 3)
                    continue;

                std::vector<int>    holeRing;
                for (const auto& p : hole)
                {
                    holeRing.push_back((int)points.size());
                    points.push_back(p);
                }
                if (fesvgSignedArea2(hole) > 0.0f)
                    std::reverse(holeRing.begin(),holeRing.end());

                bridgeHole(points,ring,holeRing);
            }

            earClip(points,ring,result);

            if (outPoints != nullptr)
                *outPoints   =   std::move(points);
            return  result;
        }

    private:
        static  float   maxX(const float2s& points)
        {
            float   v   =   -FLT_MAX;
            for (const auto& p : points)
                v   =   std::max(v,p.x);
            return  v;
        }
        static  float   cross2(const float2& a,const float2& b)
        {
            return  a.x * b.y - a.y * b.x;
        }
        /// <summary>
        /// 把洞桥接进外环
        /// </summary>
        static  void    bridgeHole(const float2s& points,std::vector<int>& ring,
                                   const std::vector<int>& hole)
        {
            /// 找洞的最右点作为桥接起点
            size_t  holeStart   =   0;
            for (size_t i = 1; i < hole.size(); ++i)
            {
                if (points[hole[i]].x > points[hole[holeStart]].x)
                    holeStart   =   i;
            }

            const   float2  holePoint   =   points[hole[holeStart]];

            /// 在当前外环中找最近的可见顶点
            int     bestIndex   =   -1;
            float   bestDist    =   FLT_MAX;
            for (size_t k = 0; k < ring.size(); ++k)
            {
                const   float2& outerPoint  =   points[ring[k]];
                const   float   dist        =   glm::distance2(holePoint,outerPoint);
                if (dist >= bestDist)
                    continue;
                if (!visible(points,ring,holePoint,ring[k],k))
                    continue;
                bestDist    =   dist;
                bestIndex   =   (int)k;
            }
            if (bestIndex < 0)
                bestIndex   =   0;

            /// 插入顺序:O -> H -> 洞环 -> H -> O,其中 O 与 H 各复制一次形成桥接缝
            std::vector<int>    inserted;
            inserted.reserve(hole.size() + 2);
            for (size_t i = 0; i < hole.size(); ++i)
                inserted.push_back(hole[(holeStart + i) % hole.size()]);
            inserted.push_back(hole[holeStart]);
            inserted.push_back(ring[bestIndex]);

            ring.insert(ring.begin() + bestIndex + 1,inserted.begin(),inserted.end());
        }
        /// <summary>
        /// 判断桥接线段是否与外环其他边严格相交
        /// </summary>
        static  bool    visible(const float2s& points,const std::vector<int>& ring,
                                const float2& from,int target,size_t targetPos)
        {
            const   float2  to  =   points[target];
            const   size_t  n   =   ring.size();
            for (size_t i = 0; i < n; ++i)
            {
                if (i == targetPos || (i + 1) % n == targetPos)
                    continue;
                const   float2& a   =   points[ring[i]];
                const   float2& b   =   points[ring[(i + 1) % n]];
                if (segmentIntersect(from,to,a,b))
                    return  false;
            }
            return  true;
        }
        /// <summary>
        /// 两线段是否严格相交(端点接触不算)
        /// </summary>
        static  bool    segmentIntersect(const float2& a,const float2& b,
                                         const float2& c,const float2& d)
        {
            const   float   d1  =   cross2(b - a,c - a);
            const   float   d2  =   cross2(b - a,d - a);
            const   float   d3  =   cross2(d - c,a - c);
            const   float   d4  =   cross2(d - c,b - c);
            const   float   eps =   1e-7f;
            return  ((d1 > eps && d2 < -eps) || (d1 < -eps && d2 > eps))
                 && ((d3 > eps && d4 < -eps) || (d3 < -eps && d4 > eps));
        }
        /// <summary>
        /// 耳切法
        /// </summary>
        static  void    earClip(const float2s& points,std::vector<int>& ring,uints& out)
        {
            const   float   eps     =   1e-9f;
            size_t          guard   =   0;
            size_t          maxStep =   ring.size() * ring.size() + 64;

            while (ring.size() > 3 && guard++ < maxStep)
            {
                bool    clipped =   false;
                const   size_t  n   =   ring.size();

                for (size_t i = 0; i < n; ++i)
                {
                    const   int     ip  =   ring[(i + n - 1) % n];
                    const   int     ic  =   ring[i];
                    const   int     in  =   ring[(i + 1) % n];
                    const   float2& a   =   points[ip];
                    const   float2& b   =   points[ic];
                    const   float2& c   =   points[in];

                    /// 只处理凸顶点,共线/凹点跳过
                    if (cross2(b - a,c - a) <= eps)
                        continue;

                    bool    ear =   true;
                    for (size_t j = 0; j < n; ++j)
                    {
                        const   int     idx =   ring[j];
                        if (idx == ip || idx == ic || idx == in)
                            continue;
                        const   float2& p   =   points[idx];
                        /// 桥接产生的重复点不参与阻塞判断
                        if (samePoint(p,a) || samePoint(p,b) || samePoint(p,c))
                            continue;
                        if (pointInTriangle(p,a,b,c))
                        {
                            ear =   false;
                            break;
                        }
                    }
                    if (!ear)
                        continue;

                    out.push_back(ip);
                    out.push_back(ic);
                    out.push_back(in);
                    ring.erase(ring.begin() + i);
                    clipped =   true;
                    break;
                }

                /// 找不到耳朵时强制切掉最凸的顶点,保证算法收敛
                if (!clipped)
                {
                    size_t  best    =   0;
                    float   bestVal =   -FLT_MAX;
                    for (size_t i = 0; i < n; ++i)
                    {
                        const   float2& a   =   points[ring[(i + n - 1) % n]];
                        const   float2& b   =   points[ring[i]];
                        const   float2& c   =   points[ring[(i + 1) % n]];
                        const   float   v   =   cross2(b - a,c - a);
                        if (v > bestVal)
                        {
                            bestVal =   v;
                            best    =   i;
                        }
                    }
                    out.push_back(ring[(best + n - 1) % n]);
                    out.push_back(ring[best]);
                    out.push_back(ring[(best + 1) % n]);
                    ring.erase(ring.begin() + best);
                }
            }

            if (ring.size() == 3)
            {
                out.push_back(ring[0]);
                out.push_back(ring[1]);
                out.push_back(ring[2]);
            }
        }
        /// <summary>
        /// 点是否在三角形内(含边界)
        /// </summary>
        static  bool    pointInTriangle(const float2& p,const float2& a,
                                        const float2& b,const float2& c)
        {
            const   float   eps =   1e-9f;
            const   float   d1  =   cross2(b - a,p - a);
            const   float   d2  =   cross2(c - b,p - b);
            const   float   d3  =   cross2(a - c,p - c);
            const   bool    neg =   (d1 < -eps) || (d2 < -eps) || (d3 < -eps);
            const   bool    pos =   (d1 > eps) || (d2 > eps) || (d3 > eps);
            return  !(neg && pos);
        }
        static  bool    samePoint(const float2& a,const float2& b)
        {
            return  glm::distance2(a,b) < 1e-10f;
        }
    };
}

#pragma     once

#include    "FEEarcut.hpp"
#include    "../../FEColor.hpp"
#include    "../../mesh/FEMeshBuilder.hpp"
#include    "../../fileFormat/svg/FESVGTypes.hpp"
#include    <algorithm>
#include    <cmath>
#include    <cfloat>
#include    <functional>

namespace   FE
{
    /// <summary>
    /// SVG 转三角网格选项
    /// </summary>
    struct  FESVGGeometryOptions
    {
        /// 曲线/圆弧细分数量
        uint    _curveSegments      =   16;
        /// SVG 坐标 y 轴向下,是否翻转为世界坐标 y 轴向上
        bool    _flipY              =   true;
        /// 是否把图形包围盒中心平移到原点
        bool    _center             =   true;
        /// 是否生成填充面
        bool    _fill               =   true;
        /// 是否生成描边面
        bool    _stroke             =   true;
        /// 描边宽度缩放
        float   _strokeScale        =   1.0f;
        /// 描边关节/端帽的圆角细分
        uint    _strokeSegments     =   8;
    };

    /// <summary>
    /// SVG 几何数据(位置/法线/UV/顶点色/索引)
    /// </summary>
    struct  FESVGGeometryData
    {
        floats      vertices;
        floats      normals;
        floats      uvs;
        Rgba8s      colors;
        uints       indices;
    };

    /// <summary>
    /// 参考 three.js SVGGeometry 的实现,把解析后的 SVG 路径转为可直接渲染的三角形:
    /// 填充部分使用 earcut 三角化(支持洞),描边部分沿折线生成四边形并补圆角关节。
    /// </summary>
    class   FESVGGeometryImpl
    {
    public:
        /// <summary>
        /// 构建几何数据
        /// </summary>
        /// <param name="doc">SVG 文档</param>
        /// <param name="opt">构建选项</param>
        /// <param name="out">输出几何数据</param>
        static  void    build(const FESVGDocument& doc,const FESVGGeometryOptions& opt,
                              FESVGGeometryData& out)
        {
            float2  minP;
            float2  maxP;
            if (!doc.bounds(minP,maxP))
                return;

            const   float2  center  =   (minP + maxP) * 0.5f;
            const   float   width   =   std::max(1e-6f,maxP.x - minP.x);
            const   float   height  =   std::max(1e-6f,maxP.y - minP.y);

            /// 用户坐标 -> 世界坐标(可选翻转 y、居中)
            auto    world   =   [&](const float2& p) -> float2
            {
                float2  q   =   opt._center ? (p - center) : p;
                if (opt._flipY)
                    q.y =   -q.y;
                return  q;
            };
            /// 用户坐标 -> UV
            auto    uvOf    =   [&](const float2& p) -> float2
            {
                return  float2((p.x - minP.x) / width,(p.y - minP.y) / height);
            };

            if (opt._fill)
            {
                for (const auto& path : doc.paths)
                {
                    if (!path.style.fill)
                        continue;
                    buildFill(path,world,uvOf,out);
                }
            }
            if (opt._stroke)
            {
                for (const auto& path : doc.paths)
                {
                    if (!path.style.stroke || path.style.strokeWidth <= 0.0f)
                        continue;
                    buildStroke(path,opt,world,uvOf,out);
                }
            }
        }

    private:
        /// <summary>
        /// 加入一个三角形,自动修正为逆时针(正面 +Z)
        /// </summary>
        static  void    addTriangle(FESVGGeometryData& out,const float2& a,const float2& b,
                                    const float2& c,const Rgba8& color,const float2& uvA,
                                    const float2& uvB,const float2& uvC)
        {
            const   float   area    =   (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (std::fabs(area) < 1e-10f)
                return;

            if (area < 0.0f)
            {
                addVertex(out,b,color,uvB);
                addVertex(out,a,color,uvA);
                addVertex(out,c,color,uvC);
            }
            else
            {
                addVertex(out,a,color,uvA);
                addVertex(out,b,color,uvB);
                addVertex(out,c,color,uvC);
            }
        }
        /// <summary>
        /// 加入一个顶点
        /// </summary>
        static  void    addVertex(FESVGGeometryData& out,const float2& p,
                                  const Rgba8& color,const float2& uv)
        {
            out.indices.push_back((uint)(out.vertices.size() / 3));
            out.vertices.push_back(p.x);
            out.vertices.push_back(p.y);
            out.vertices.push_back(0.0f);
            out.normals.push_back(0.0f);
            out.normals.push_back(0.0f);
            out.normals.push_back(1.0f);
            out.uvs.push_back(uv.x);
            out.uvs.push_back(uv.y);
            out.colors.push_back(color);
        }
        /// <summary>
        /// 生成填充面
        /// </summary>
        static  void    buildFill(const FESVGPath& path,
                                  const std::function<float2(const float2&)>& world,
                                  const std::function<float2(const float2&)>& uvOf,
                                  FESVGGeometryData& out)
        {
            /// 1. 按包围关系把子路径分成"外轮廓 + 洞"
            const   size_t  count   =   path.subPaths.size();
            if (count == 0)
                return;

            std::vector<int>    depth(count,0);
            std::vector<float>  area(count,0.0f);
            for (size_t i = 0; i < count; ++i)
            {
                area[i] =   std::fabs(fesvgSignedArea2(path.subPaths[i].points));
                if (path.subPaths[i].points.size() < 3)
                    continue;
                for (size_t j = 0; j < count; ++j)
                {
                    if (i == j || path.subPaths[j].points.size() < 3)
                        continue;
                    if (fesvgPointInPolygon(path.subPaths[i].points.front(),
                                            path.subPaths[j].points))
                    {
                        ++depth[i];
                    }
                }
            }

            /// 2. 每个偶数层为外轮廓,奇数层洞挂到包含它且面积最小的外轮廓上
            struct  Group
            {
                size_t              outer   =   (size_t)-1;
                std::vector<size_t> holes;
            };
            std::vector<Group>  groups;
            std::vector<size_t> outerMap(count,(size_t)-1);
            for (size_t i = 0; i < count; ++i)
            {
                if (path.subPaths[i].points.size() < 3 || (depth[i] & 1) != 0)
                    continue;
                Group   g;
                g.outer =   i;
                outerMap[i] =   groups.size();
                groups.push_back(std::move(g));
            }
            for (size_t i = 0; i < count; ++i)
            {
                if (path.subPaths[i].points.size() < 3 || (depth[i] & 1) == 0)
                    continue;

                size_t  best    =   (size_t)-1;
                float   bestArea    =   FLT_MAX;
                for (size_t g = 0; g < groups.size(); ++g)
                {
                    const   size_t  o   =   groups[g].outer;
                    if (area[o] < bestArea
                     && fesvgPointInPolygon(path.subPaths[i].points.front(),
                                            path.subPaths[o].points))
                    {
                        bestArea    =   area[o];
                        best        =   g;
                    }
                }
                if (best != (size_t)-1)
                    groups[best].holes.push_back(i);
            }

            /// 3. 三角化并输出
            const   Rgba8   color   =   toRgba(path.style.fillColor,path.style.opacity);
            for (const auto& g : groups)
            {
                const   float2s&    outerPts    =   path.subPaths[g.outer].points;
                std::vector<float2s>    holes;
                holes.reserve(g.holes.size());
                for (size_t h : g.holes)
                    holes.push_back(path.subPaths[h].points);

                float2s     allPoints;
                const   uints   tris    =   FEEarcut::triangulate(outerPts,holes,&allPoints);
                for (size_t i = 0; i + 2 < tris.size(); i += 3)
                {
                    const   float2& p0  =   allPoints[tris[i + 0]];
                    const   float2& p1  =   allPoints[tris[i + 1]];
                    const   float2& p2  =   allPoints[tris[i + 2]];
                    addTriangle(out,world(p0),world(p1),world(p2),color,
                                uvOf(p0),uvOf(p1),uvOf(p2));
                }
            }
        }
        /// <summary>
        /// 生成描边面(四边形 + 圆角关节)
        /// </summary>
        static  void    buildStroke(const FESVGPath& path,const FESVGGeometryOptions& opt,
                                    const std::function<float2(const float2&)>& world,
                                    const std::function<float2(const float2&)>& uvOf,
                                    FESVGGeometryData& out)
        {
            const   float   halfWidth   =   path.style.strokeWidth * 0.5f
                                        *   std::max(0.0f,opt._strokeScale);
            if (halfWidth <= 0.0f)
                return;

            const   Rgba8   color   =   toRgba(path.style.strokeColor,path.style.opacity);

            for (const auto& sub : path.subPaths)
            {
                const   size_t  n   =   sub.points.size();
                if (n < 2)
                    continue;

                const   size_t  segCount    =   sub.closed ? n : n - 1;
                for (size_t i = 0; i < segCount; ++i)
                {
                    const   float2  a   =   world(sub.points[i]);
                    const   float2  b   =   world(sub.points[(i + 1) % n]);
                    const   float2  dir =   b - a;
                    const   float   len =   glm::length(dir);
                    if (len < 1e-8f)
                        continue;

                    const   float2  normal  =   float2(-dir.y / len,dir.x / len) * halfWidth;
                    const   float2  a0      =   a - normal;
                    const   float2  a1      =   a + normal;
                    const   float2  b0      =   b - normal;
                    const   float2  b1      =   b + normal;

                    addTriangle(out,a0,b0,b1,color,uvOf(sub.points[i]),
                                uvOf(sub.points[(i + 1) % n]),uvOf(sub.points[(i + 1) % n]));
                    addTriangle(out,a0,b1,a1,color,uvOf(sub.points[i]),
                                uvOf(sub.points[(i + 1) % n]),uvOf(sub.points[i]));
                }

                /// 关节与端帽:在每个顶点加一个圆盘,使转折处不出现缺口
                const   uint    segments    =   std::max<uint>(6,opt._strokeSegments);
                for (size_t i = 0; i < n; ++i)
                {
                    const   float2  c   =   world(sub.points[i]);
                    for (uint s = 0; s < segments; ++s)
                    {
                        const   float   a0  =   float(s) * float(TWO_PI) / float(segments);
                        const   float   a1  =   float(s + 1) * float(TWO_PI) / float(segments);
                        const   float2  p0  =   c + float2(std::cos(a0),std::sin(a0)) * halfWidth;
                        const   float2  p1  =   c + float2(std::cos(a1),std::sin(a1)) * halfWidth;
                        addTriangle(out,c,p0,p1,color,uvOf(sub.points[i]),
                                    uvOf(sub.points[i]),uvOf(sub.points[i]));
                    }
                }
            }
        }
        /// <summary>
        /// float4 颜色 -> Rgba8,alpha 乘入累计不透明度
        /// </summary>
        static  Rgba8   toRgba(const float4& color,float opacity)
        {
            auto    toU8    =   [](float v) -> uint8
            {
                v   =   std::max(0.0f,std::min(1.0f,v));
                return  (uint8)(v * 255.0f + 0.5f);
            };
            return  Rgba8(toU8(color.r),toU8(color.g),toU8(color.b),
                          toU8(color.a * std::max(0.0f,std::min(1.0f,opacity))));
        }
    };
}

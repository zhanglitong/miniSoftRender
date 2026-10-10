#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"
#include    "../curve/FECurve.hpp"
#include    "../curve/FELineCurve3.hpp"
#include    "../curve/FEQuadraticBezierCurve3.hpp"
#include    "../curve/FECubicBezierCurve3.hpp"
#include    "../curve/FECatmullRomCurve3.hpp"
#include    <memory>

namespace   FE
{
    /// <summary>
    /// 管道参数: 沿一条 3D 曲线挤出圆形截面
    /// </summary>
    struct  FEParamTube
    {
        /// <summary>曲线类型</summary>
        FECurveType         type            =   FECurveType::CatmullRom;
        /// <summary>控制点: Line=2, QuadraticBezier=3, CubicBezier=4, CatmullRom>=2</summary>
        float3s             points;
        /// <summary>管道半径</summary>
        float               radius          =   0.1f;
        /// <summary>沿曲线方向分段数</summary>
        uint                tubularSegments =   64;
        /// <summary>截面圆周分段数</summary>
        uint                radialSegments  =   8;
        /// <summary>曲线是否闭合</summary>
        bool                closed          =   false;
        /// <summary>Catmull-Rom 参数化类型</summary>
        FECatmullRomType    catmullType     =   FECatmullRomType::Centripetal;
        /// <summary>Catmull-Rom 张力(uniform 模式有效)</summary>
        float               tension         =   0.5f;
    };

    class   FETubeImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        /// <summary>
        /// 依据参数创建曲线对象
        /// </summary>
        static  std::unique_ptr<FECurve>  createCurve(const FEParamTube& params)
        {
            const auto& p   =   params.points;
            switch (params.type)
            {
            case FECurveType::Line:
                if (p.size() >= 2)
                    return  std::make_unique<FELineCurve3>(p[0], p[1]);
                break;
            case FECurveType::QuadraticBezier:
                if (p.size() >= 3)
                    return  std::make_unique<FEQuadraticBezierCurve3>(p[0], p[1], p[2]);
                break;
            case FECurveType::CubicBezier:
                if (p.size() >= 4)
                    return  std::make_unique<FECubicBezierCurve3>(p[0], p[1], p[2], p[3]);
                break;
            case FECurveType::CatmullRom:
            default:
                if (p.size() >= 2)
                    return  std::make_unique<FECatmullRomCurve3>(p, params.closed, params.catmullType, params.tension);
                break;
            }
            return  nullptr;
        }

        template<bool bNormal,bool bUV>
        static  void    build(const FEParamTube& params,FETubeImpl& result)
        {
            auto    path    =   createCurve(params);
            if (!path)
                return;

            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const uint  tubularSegments =   std::max(params.tubularSegments, 1u);
            const uint  radialSegments  =   std::max(params.radialSegments, 3u);
            const float radius          =   params.radius;
            const bool  closed          =   params.closed;

            /// Frenet 标架
            FEFrenetFrames frames   =   path->computeFrenetFrames(tubularSegments, closed);

            /// 生成第 i 个截面环 (移植自 three.js TubeGeometry.generateSegment)
            auto    generateSegment = [&](uint i)
            {
                float3  P   =   path->getPointAt((float)i / (float)tubularSegments);
                const float3&   N   =   frames.normals[i];
                const float3&   B   =   frames.binormals[i];

                for (uint j = 0; j <= radialSegments; ++j)
                {
                    float   v   =   (float)j / (float)radialSegments * float(TWO_PI);
                    float   sinV=   std::sin(v);
                    float   cosV=   -std::cos(v);

                    float3  normal(
                        cosV * N.x + sinV * B.x,
                        cosV * N.y + sinV * B.y,
                        cosV * N.z + sinV * B.z);
                    normal  =   FE::normalize(normal);

                    if constexpr (bNormal)
                        ns.insert(ns.end(), {normal.x, normal.y, normal.z});

                    float3  vertex  =   P + radius * normal;
                    vs.insert(vs.end(), {vertex.x, vertex.y, vertex.z});
                }
            };

            /// 截面环
            for (uint i = 0; i < tubularSegments; ++i)
                generateSegment(i);
            /// 非闭合: 末端常规环; 闭合: 复制首环(uv 不同)
            generateSegment(closed ? 0 : tubularSegments);

            /// UV
            if constexpr (bUV)
            {
                for (uint i = 0; i <= tubularSegments; ++i)
                    for (uint j = 0; j <= radialSegments; ++j)
                        us.insert(us.end(), {
                            (float)i / (float)tubularSegments,
                            (float)j / (float)radialSegments });
            }

            /// 索引 (移植自 three.js TubeGeometry.generateIndices)
            for (uint j = 1; j <= tubularSegments; ++j)
            {
                for (uint i = 1; i <= radialSegments; ++i)
                {
                    uint    a   =   (radialSegments + 1) * (j - 1) + (i - 1);
                    uint    b   =   (radialSegments + 1) * j       + (i - 1);
                    uint    c   =   (radialSegments + 1) * j       + i;
                    uint    d   =   (radialSegments + 1) * (j - 1) + i;
                    ids.insert(ids.end(), {a, b, d});
                    ids.insert(ids.end(), {b, c, d});
                }
            }
        }
    };
}

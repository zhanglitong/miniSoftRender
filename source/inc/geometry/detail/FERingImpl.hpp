#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamRing
    {
        /// <summary>
        /// 内圆半径，圆环中心孔洞的半径，默认值为 0.5
        /// </summary>
        float   innerRadius     =   0.5f;
        /// <summary>
        /// 外圆半径，圆环边缘到中心的距离，默认值为 1
        /// </summary>
        float   outerRadius     =   1.0f;
        /// <summary>
        /// 起始角度，默认值为 0
        /// </summary>
        float   thetaStart      =   0;
        /// <summary>
        /// 圆心角大小，默认值为 TWO_PI(360度)
        /// </summary>
        float   thetaLength     =   float(TWO_PI);
        /// <summary>
        /// 圆周分段数(圆周方向的面数)，默认值为 16
        /// </summary>
        uint    thetaSegments   =   16;
        /// <summary>
        /// 环径分段数(半径方向的面数)，默认值为 1
        /// </summary>
        uint    phiSegments     =   1;
    };

    class   FERingImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamRing& params,FERingImpl& result)
        {
            auto&   indices         =   result.indices;
            auto&   vs              =   result.vertices;
            auto&   ns              =   result.normals;
            auto&   us              =   result.uvs;

            uint    thetaSegments   =   (std::max)(3u, params.thetaSegments);
            uint    phiSegments     =   (std::max)(1u, params.phiSegments);
            float   radius          =   params.innerRadius;
            const auto radiusStep   =   ((params.outerRadius - params.innerRadius) / (float)(phiSegments));
            float3  vertex;
            float2  uv;

            // generate vertices, normals and uvs
            for (uint j = 0; j <= phiSegments; j++)
            {
                for (uint i = 0; i <= thetaSegments; i++)
                {
                    // values are generate from the inside of the ring to the outside
                    const auto segment = params.thetaStart + (float)(i) / (float)(thetaSegments) * params.thetaLength;
                    // vertex
                    vertex.x    =   radius * std::cos(segment);
                    vertex.y    =   radius * std::sin(segment);
                    vs.insert(vs.end(), {vertex.x, vertex.y, 0.0f});

                    // normal
                    if constexpr (bNormal)
                    {
                        ns.insert(ns.end(), {0.0f, 0.0f, 1.0f});
                    }
                    // uv
                    if constexpr (bUV)
                    {
                        uv.x = (vertex.x / params.outerRadius + 1) / 2;
                        uv.y = (vertex.y / params.outerRadius + 1) / 2;
                        us.insert(us.end(), {uv.x, uv.y});
                    }
                }
                // increase the radius for next row of vertices
                radius += radiusStep;
            }
            for (uint j = 0; j < phiSegments; j++)
            {
                const auto thetaSegmentLevel = j * (thetaSegments + 1);

                for (uint i = 0; i < thetaSegments; i++)
                {
                    const auto segment = i + thetaSegmentLevel;

                    const auto a = segment;
                    const auto b = segment + thetaSegments + 1;
                    const auto c = segment + thetaSegments + 2;
                    const auto d = segment + 1;
                    // faces
                    indices.insert(indices.end(), {a, b, d});
                    indices.insert(indices.end(), {b, c, d});
                }
            }
        }
    };
}

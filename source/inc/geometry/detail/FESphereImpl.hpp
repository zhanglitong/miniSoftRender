#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamSphere
    {
        /// <summary>
        /// 球体的半径
        /// </summary>
        float   radius          =   1;
        /// <summary>
        /// 水平分段数(经度方向)
        /// 默认值为 16。数值越大，球体在水平圆周上越圆滑
        /// </summary>
        uint    widthSegments   =   16;
        /// <summary>
        /// 垂直分段数(纬度方向)
        /// 默认值为 12。数值越大，球体从极点到极点的弧线越圆滑
        /// </summary>
        uint    heightSegments  =   12;
        /// <summary>
        /// 水平(经度)起始角度，默认值为 0
        /// </summary>
        float   phiStart        =   0;
        /// <summary>
        /// 水平(经度)扫描角度的大小，默认值为 TWO_PI(360度)
        /// </summary>
        float   phiLength       =   float(TWO_PI);
        /// <summary>
        /// 垂直(纬度)起始角度，默认值为 0(北极点)
        /// </summary>
        float   thetaStart      =   0;
        /// <summary>
        /// 垂直(纬度)扫描角度的大小，默认值为 PI(180度)
        /// </summary>
        float   thetaLength     =   float(PI);
    };

    class   FESphereImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamSphere& params,FESphereImpl& result)
        {
            auto&   indices     =   result.indices;
            auto&   vs          =   result.vertices;
            auto&   ns          =   result.normals;
            auto&   us          =   result.uvs;

            uint    wSegs       =   (std::max)(3u, params.widthSegments);
            uint    hSegs       =   (std::max)(2u, params.heightSegments);
            float   thetaEnd    =   (std::min)(params.thetaStart + params.thetaLength, float(PI));

            uint    index       =   0;
            std::vector<std::vector<uint>> grid;

            float3  vertex;
            float3  normal;
            // generate vertices, normals and uvs
            for (uint iy = 0; iy <= hSegs; iy++)
            {
                std::vector<uint> verticesRow;
                float v         =   (float)(iy) / (float)(hSegs);
                // special case for the poles
                float uOffset   =   0;

                if (iy == 0 && params.thetaStart == 0)
                    uOffset =   0.5f / (float)(wSegs);
                else if (iy == hSegs && thetaEnd == float(PI))
                    uOffset =   -0.5f / (float)(wSegs);

                for (uint ix = 0; ix <= wSegs; ix++)
                {
                    const float u = (float)(ix) / (float)(wSegs);

                    vertex.x = -params.radius * std::cos(params.phiStart   + u * params.phiLength) * std::sin(params.thetaStart + v * params.thetaLength);
                    vertex.y =  params.radius * std::cos(params.thetaStart + v * params.thetaLength);
                    vertex.z =  params.radius * std::sin(params.phiStart   + u * params.phiLength) * std::sin(params.thetaStart + v * params.thetaLength);

                    vs.insert(vs.end(), {vertex.x, vertex.y, vertex.z});

                    // normal
                    if constexpr (bNormal)
                    {
                        normal  =   FE::normalize(vertex);
                        ns.insert(ns.end(), {normal.x, normal.y, normal.z});
                    }
                    // uv
                    if constexpr (bUV)
                    {
                        us.insert(us.end(), {u + uOffset, 1.0f - v});
                    }
                    verticesRow.emplace_back(index++);
                }

                grid.emplace_back(verticesRow);
            }
            // indices
            for (uint iy = 0; iy < hSegs; iy++)
            {
                for (uint ix = 0; ix < wSegs; ix++)
                {
                    const auto a = grid[iy][ix + 1];
                    const auto b = grid[iy][ix];
                    const auto c = grid[iy + 1][ix];
                    const auto d = grid[iy + 1][ix + 1];

                    if (iy != 0 || params.thetaStart > 0)   indices.insert(indices.end(), {a, b, d});
                    if (iy != hSegs - 1 || thetaEnd < float(PI))  indices.insert(indices.end(), {b, c, d});
                }
            }
        }
    };
}

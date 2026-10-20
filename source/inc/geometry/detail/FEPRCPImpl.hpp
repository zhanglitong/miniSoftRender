#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 球冠体参数 (Spherical Cap)
    /// </summary>
    struct  FEParamPRCP
    {
        /// <summary>球的半径</summary>
        float   radius          =   1.0f;
        /// <summary>球冠高度(从球顶到切割面的距离)</summary>
        float   height          =   0.5f;
        /// <summary>圆周分段数</summary>
        uint    radialSegments  =   32;
        /// <summary>高度分段数</summary>
        uint    heightSegments  =   8;
    };

    class   FEPRCPImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamPRCP& params,FEPRCPImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const uint  rSegs  =   std::max(params.radialSegments, 3u);
            const uint  hSegs  =   std::max(params.heightSegments, 1u);
            const float R      =   params.radius;
            const float h      =   std::min(params.height, 2.0f * R);

            /// <summary>
            /// 球冠从北极 (theta=0) 到 theta_max
            /// cos(theta_max) = (R - h) / R
            /// </summary>
            float   cosThetaMax   =   (R - h) / R;
            cosThetaMax   =   std::max(-1.0f, std::min(1.0f, cosThetaMax));
            float   thetaMax     =   std::acos(cosThetaMax);

            /// <summary>
            /// 球心位于 (0, -(R-h), 0), 球冠顶点在 (0, h, 0)
            /// </summary>
            float   cy  =   -(R - h);

            uint    index   =   0;
            std::vector<std::vector<uint>> grid;
            grid.resize(hSegs + 1);

            for (uint y = 0; y <= hSegs; ++y)
            {
                float   v       =   (float)y / (float)hSegs;
                float   theta   =   v * thetaMax;
                float   sinTh   =   std::sin(theta);
                float   cosTh   =   std::cos(theta);
                float   ringR   =   R * sinTh;
                float   ringY   =   R * cosTh + cy;

                for (uint x = 0; x <= rSegs; ++x)
                {
                    float   u       =   (float)x / (float)rSegs;
                    float   phi     =   u * float(TWO_PI);
                    float   sinPhi  =   std::sin(phi);
                    float   cosPhi  =   std::cos(phi);

                    float   px  =   ringR * cosPhi;
                    float   py  =   ringY;
                    float   pz  =   ringR * sinPhi;
                    vs.insert(vs.end(),{px, py, pz});

                    if constexpr (bNormal)
                    {
                        float3  n   =   FE::normalize(float3(px, py - cy, pz));
                        ns.insert(ns.end(),{n.x, n.y, n.z});
                    }
                    if constexpr (bUV)
                    {
                        us.insert(us.end(),{u, v});
                    }
                    grid[y].push_back(index++);
                }
            }

            /// 侧面
            for (uint y = 0; y < hSegs; ++y)
            {
                for (uint x = 0; x < rSegs; ++x)
                {
                    uint    a   =   grid[y][x];
                    uint    b   =   grid[y][x+1];
                    uint    c   =   grid[y+1][x];
                    uint    d   =   grid[y+1][x+1];
                    ids.insert(ids.end(),{a, b, c});
                    ids.insert(ids.end(),{b, d, c});
                }
            }

            /// <summary>
            /// 底面封口
            /// </summary>
            uint    baseCenter  = index;
            float   baseY       =   R * cosThetaMax + cy;
            vs.insert(vs.end(),{0, baseY, 0});
            if constexpr (bNormal) ns.insert(ns.end(),{0, -1, 0});
            if constexpr (bUV)     us.insert(us.end(),{0.5f, 0.5f});
            index++;

            for (uint x = 0; x < rSegs; ++x)
            {
                ids.insert(ids.end(),{baseCenter, grid[hSegs][x], grid[hSegs][x+1]});
            }
        }
    };
}

#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 椭球体参数 (Spheroid)
    /// </summary>
    struct  FEParamPRDH
    {
        /// <summary>赤道半径 (X/Z 方向)</summary>
        float   radius          =   1.0f;
        /// <summary>极轴高度 (Y 方向半轴长)</summary>
        float   height          =   1.0f;
        /// <summary>经度分段数</summary>
        uint    widthSegments   =   32;
        /// <summary>纬度分段数</summary>
        uint    heightSegments  =   16;
    };

    class   FEPRDHImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamPRDH& params,FEPRDHImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const uint  wSegs   =   std::max(params.widthSegments,  3u);
            const uint  hSegs   =   std::max(params.heightSegments, 2u);
            const float r       =   params.radius;
            const float h       =   params.height;

            uint    index   =   0;
            std::vector<std::vector<uint>> grid;
            grid.resize(hSegs + 1);

            for (uint y = 0; y <= hSegs; ++y)
            {
                float   v       =   (float)y / (float)hSegs;
                float   theta   =   v * float(PI);
                float   sinTh   =   std::sin(theta);
                float   cosTh   =   std::cos(theta);

                for (uint x = 0; x <= wSegs; ++x)
                {
                    float   u       =   (float)x / (float)wSegs;
                    float   phi     =   u * float(TWO_PI);
                    float   sinPhi  =   std::sin(phi);
                    float   cosPhi  =   std::cos(phi);

                    float   px  =   -r * sinTh * cosPhi;
                    float   py  =   h * cosTh;
                    float   pz  =   r * sinTh * sinPhi;
                    vs.insert(vs.end(),{px, py, pz});

                    if constexpr (bNormal)
                    {
                        /// 椭球法线: normalize(x/r², y/h², z/r²) — px 已含负号, 方向自动正确
                        float   nx  =   px / (r * r);
                        float   ny  =   py / (h * h);
                        float   nz  =   pz / (r * r);
                        float3  n   =   FE::normalize(float3(nx, ny, nz));
                        ns.insert(ns.end(),{n.x, n.y, n.z});
                    }
                    if constexpr (bUV)
                    {
                        us.insert(us.end(),{u, v});
                    }
                    grid[y].push_back(index++);
                }
            }

            for (uint y = 0; y < hSegs; ++y)
            {
                for (uint x = 0; x < wSegs; ++x)
                {
                    /// 与 FESphereImpl 完全一致的 winding
                    const auto a = grid[y][x + 1];
                    const auto b = grid[y][x];
                    const auto c = grid[y + 1][x];
                    const auto d = grid[y + 1][x + 1];
                    ids.insert(ids.end(),{a, b, d});
                    ids.insert(ids.end(),{b, c, d});
                }
            }
        }
    };
}

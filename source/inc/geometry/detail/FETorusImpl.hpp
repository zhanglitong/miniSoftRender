#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamTorus
    {
        float   radius          =   1.0f;
        float   tube            =   0.4f;
        uint    radialSegments  =   20;
        uint    tubularSegments =   64;
        float   arc             =   float(TWO_PI);
    };

    class   FETorusImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamTorus& params,FETorusImpl& result)
        {
            auto&   indices =   result.indices;
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;

            float   radius          =   params.radius;
            float   tube            =   params.tube;
            uint    radialSegments  =   params.radialSegments;
            uint    tubularSegments =   params.tubularSegments;
            float   arc             =   params.arc;

            float3  center;
            float3  vertex;
            float3  normal;

            // generate vertices, normals and uvs
            for (uint j = 0; j <= radialSegments; j++)
            {
                for (uint i = 0; i <= tubularSegments; i++)
                {
                    const auto u = (float)(i) / (float)(tubularSegments) * arc;
                    const auto v = (float)(j) / (float)(radialSegments) * float(TWO_PI);
                    // vertex
                    vertex.x = (radius + tube * std::cos(v)) * std::cos(u);
                    vertex.y = (radius + tube * std::cos(v)) * std::sin(u);
                    vertex.z = tube * std::sin(v);
                    vs.insert(vs.end(), {vertex.x, vertex.y, vertex.z});
                    // normal
                    if constexpr (bNormal)
                    {
                        center.x = radius * std::cos(u);
                        center.y = radius * std::sin(u);
                        normal   = FE::normalize(vertex - center);
                        ns.insert(ns.end(), {normal.x, normal.y, normal.z});
                    }
                    // uv
                    if constexpr (bUV)
                    {
                        float u2 = (float)(i) / (float)(tubularSegments);
                        float v2 = (float)(j) / (float)(radialSegments);
                        us.insert(us.end(), {u2, v2});
                    }
                }
            }
            for (uint j = 1; j <= radialSegments; j++)
            {
                for (uint i = 1; i <= tubularSegments; i++)
                {
                    // indices
                    const auto a = (tubularSegments + 1) * j + i - 1;
                    const auto b = (tubularSegments + 1) * (j - 1) + i - 1;
                    const auto c = (tubularSegments + 1) * (j - 1) + i;
                    const auto d = (tubularSegments + 1) * j + i;
                    // faces
                    indices.insert(indices.end(), {a, b, d});
                    indices.insert(indices.end(), {b, c, d});
                }
            }
        }
    };
}

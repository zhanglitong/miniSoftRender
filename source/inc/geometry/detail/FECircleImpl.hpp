#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamCircle
    {
        float   radius      =   1.0f;
        float   thetaStart  =   0;
        float   thetaLength =   float(TWO_PI);
        uint    segments    =   16;
    };

    class   FECircleImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamCircle& params,FECircleImpl& result)
        {
            auto&   indices =   result.indices;
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;

            float3  vertex;
            float2  uv;
            // center point
            vs.insert(vs.end(), {0.0f, 0.0f, 0.0f});
            if constexpr (bNormal)
                ns.insert(ns.end(), {0.0f, 0.0f, 1.0f});
            if constexpr (bUV)
                us.insert(us.end(), {0.5f, 0.5f});

            for (uint s = 0; s <= params.segments; s++)
            {
                const auto segment = params.thetaStart + (float)(s) / (float)(params.segments) * params.thetaLength;

                // vertex
                vertex.x    =   params.radius * std::cos(segment);
                vertex.y    =   params.radius * std::sin(segment);

                vs.insert(vs.end(), {vertex.x, vertex.y, 0.0f});
                // normal
                if constexpr (bNormal)
                {
                    ns.insert(ns.end(), {0.0f, 0.0f, 1.0f});
                }
                // uvs
                if constexpr (bUV)
                {
                    uv.x = (vertex.x / params.radius + 1.0f) * 0.5f;
                    uv.y = (vertex.y / params.radius + 1.0f) * 0.5f;
                    us.insert(us.end(), {uv.x, uv.y});
                }
            }
            // indices
            for (uint i = 1; i <= params.segments; i++)
            {
                indices.insert(indices.end(), {i, i + 1, 0});
            }
        }
    };
}

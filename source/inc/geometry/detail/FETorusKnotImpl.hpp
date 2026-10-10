#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamTorusKnot
    {
        /// <summary>
        /// 整个扭结所在的圆环(Torus)的主半径
        /// </summary>
        float   radius          =   1;
        /// <summary>
        /// 管道半径，构成扭结的管道本身的粗细
        /// </summary>
        float   tube            =   0.4f;
        /// <summary>
        /// 管段分段数，沿着整根管子长度方向的分段数
        /// </summary>
        uint    tubularSegments =   64;
        /// <summary>
        /// 径向分段数，管道横截面的分段数(圆周方向)
        /// </summary>
        uint    radialSegments  =   16;
        /// <summary>
        /// 控制管子围绕其旋转轴绕了多少圈
        /// </summary>
        uint    p               =   2;
        /// <summary>
        /// 控制管子绕着圆环内部绕了多少圈
        /// </summary>
        uint    q               =   3;
    };

    class   FETorusKnotImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamTorusKnot& params,FETorusKnotImpl& result)
        {
            auto&   indices =   result.indices;
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;

            float   radius          =   params.radius;
            float   tube            =   params.tube;
            uint    radialSegments  =   params.radialSegments;
            uint    tubularSegments =   params.tubularSegments;
            uint    p               =   params.p;
            uint    q               =   params.q;
            float3  vertex;
            float3  normal;

            float3  P1;
            float3  P2;
            float3  B;
            float3  T;
            float3  N;

            // generate vertices, normals and uvs
            for (uint i = 0; i <= tubularSegments; ++i)
            {
                // the radian "u" is used to calculate the position on the torus curve of the current tubular segment
                const auto u = (float)(i) / (float)(tubularSegments) * (float)(p) * float(TWO_PI);
                // now we calculate two points. P1 is our current position on the curve, P2 is a little farther ahead.
                // these points are used to create a special "coordinate space"
                // which is necessary to calculate the correct vertex positions
                calculatePositionOnCurve(u, p, q, radius, P1);
                calculatePositionOnCurve(u + 0.01f, p, q, radius, P2);

                // calculate orthonormal basis
                T   =   (P2 - P1);
                N   =   (P2 + P1);
                B   =   FE::cross(T, N);
                N   =   FE::cross(B, T);
                // normalize B, N. T can be ignored, we don't use it
                B   =   FE::normalize(B);
                N   =   FE::normalize(N);

                for (uint j = 0; j <= radialSegments; ++j)
                {
                    // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
                    // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.
                    const auto v    =   (float)(j) / (float)(radialSegments) * float(TWO_PI);
                    const auto cx   =   -tube * std::cos(v);
                    const auto cy   =   tube * std::sin(v);

                    // now calculate the final vertex position.
                    // first we orient the extrusion with our basis vectors, then we add it to the current position on the curve
                    vertex.x = P1.x + (cx * N.x + cy * B.x);
                    vertex.y = P1.y + (cx * N.y + cy * B.y);
                    vertex.z = P1.z + (cx * N.z + cy * B.z);

                    vs.insert(vs.end(), {vertex.x, vertex.y, vertex.z});
                    // normal (P1 is always the center/origin of the extrusion;
                    // thus we can use it to calculate the normal)
                    if constexpr (bNormal)
                    {
                        normal  =   FE::normalize(vertex - P1);
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
            // generate indices
            for (uint j = 1; j <= tubularSegments; j++)
            {
                for (uint i = 1; i <= radialSegments; i++)
                {
                    // indices
                    const auto a = (radialSegments + 1) * (j - 1) + (i - 1);
                    const auto b = (radialSegments + 1) * j + (i - 1);
                    const auto c = (radialSegments + 1) * j + i;
                    const auto d = (radialSegments + 1) * (j - 1) + i;
                    // faces
                    indices.insert(indices.end(), {a, b, d});
                    indices.insert(indices.end(), {b, c, d});
                }
            }
        }
    protected:
        static  void    calculatePositionOnCurve(float u, unsigned int p, unsigned int q, float radius, float3& pos)
        {
            float   cu      =   std::cos(u);
            float   su      =   std::sin(u);
            float   quOverP =   float(q) / float(p) * u;
            float   cs      =   std::cos(quOverP);

            pos.x           =   radius * (2 + cs) * 0.5f * cu;
            pos.y           =   radius * (2 + cs) * su * 0.5f;
            pos.z           =   radius * std::sin(quOverP) * 0.5f;
        }
    };
}

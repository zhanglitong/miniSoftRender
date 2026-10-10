#pragma     once
#include    "../../FEObject.h"
#include    "../../FEColor.hpp"

namespace   FE
{
    struct  FEParamAabb
    {
        /// <summary>
        /// 立方体的大小
        /// </summary>
        float3  _size   =   {1,1,1};
        /// <summary>
        /// 线框颜色
        /// </summary>
        Rgba8   _color  =   Rgba8(128,128,128,255);
    };

    class   FEAabbImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
        Rgba8s      colors;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamAabb& params,FEAabbImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   indices =   result.indices;
            auto&   colors  =   result.colors;

            const auto  half    =   params._size * 0.5f;
            const float hx = half.x, hy = half.y, hz = half.z;

            // 8 corner vertices
            vs.insert(vs.end(), {-hx, -hy, -hz});   // 0
            vs.insert(vs.end(), { hx, -hy, -hz});   // 1
            vs.insert(vs.end(), { hx,  hy, -hz});   // 2
            vs.insert(vs.end(), {-hx,  hy, -hz});   // 3
            vs.insert(vs.end(), {-hx, -hy,  hz});   // 4
            vs.insert(vs.end(), { hx, -hy,  hz});   // 5
            vs.insert(vs.end(), { hx,  hy,  hz});   // 6
            vs.insert(vs.end(), {-hx,  hy,  hz});   // 7

            // 8 vertex colors
            colors.insert(colors.end(), 8, params._color);

            // 12 edges (line indices)
            indices.insert(indices.end(), {0,1, 1,2, 2,3, 3,0});        // bottom face
            indices.insert(indices.end(), {4,5, 5,6, 6,7, 7,4});        // top face
            indices.insert(indices.end(), {0,4, 1,5, 2,6, 3,7});        // vertical edges
        }
    };
}

#pragma     once

#include    "../../FEObject.h"

namespace   FE
{
    class   FEPlaneImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;

    public:
        template<bool bNormal,bool bUV>
        static  void    build(   float      width
                                ,float      height
                                ,uint       wSegs
                                ,uint       hSegs
                                ,FEPlaneImpl& result)
        {
            build<bNormal,bUV>(width,height,wSegs,hSegs,result.vertices,result.normals,result.uvs,result.indices);
        }

        template<bool bNormal,bool bUV>
        static  void    build(   float      width
                                ,float      height
                                ,uint       wSegs
                                ,uint       hSegs
                                ,floats&    vs
                                ,floats&    ns
                                ,floats&    us
                                ,uints&     ids)
        {
            const auto  wHalf   =   width  * 0.5f;
            const auto  hHalf   =   height * 0.5f;

            const auto  gridX   =   wSegs;
            const auto  gridY   =   hSegs;

            const auto  gridX1  =   gridX + 1;
            const auto  gridY1  =   gridY + 1;

            const auto  segW    =   width  / static_cast<float>(gridX);
            const auto  segH    =   height / static_cast<float>(gridY);

            vs.reserve(gridX1 * gridY1 * 3);
            ns.reserve(gridX1 * gridY1 * 3);
            us.reserve(gridX1 * gridY1 * 2);
            ids.reserve(gridX * gridY * 6);

            const   auto    fGridX  =   static_cast<float>(gridX);
            const   auto    fGridY  =   static_cast<float>(gridY);

            for (unsigned iy = 0; iy < gridY1; iy++)
            {
                const auto fy   =   static_cast<float>(iy);
                const auto y    =   fy * segH - hHalf;
                for (unsigned ix = 0; ix < gridX1; ix++)
                {
                    const auto fx   =   static_cast<float>(ix);
                    const auto x    =   fx * segW - wHalf;

                    vs.insert(vs.end(),{x,-y,0});
                    if constexpr(bNormal)
                    {
                        ns.insert(ns.end(),{0,0,1});
                    }
                    if constexpr(bUV)
                    {
                        float   u   =   fx / fGridX;
                        float   v   =   1.0f - fy / fGridY;
                        us.insert(us.end(),{u,v});
                    }
                }
            }

            for (unsigned iy = 0; iy < gridY; iy++)
            {
                for (unsigned ix = 0; ix < gridX; ix++)
                {
                    const auto a = (ix + gridX1 * iy);
                    const auto b = (ix + gridX1 * (iy + 1));
                    const auto c = ((ix + 1) + gridX1 * (iy + 1));
                    const auto d = ((ix + 1) + gridX1 * iy);

                    ids.push_back(a);
                    ids.push_back(b);
                    ids.push_back(d);
                    ids.push_back(b);
                    ids.push_back(c);
                    ids.push_back(d);
                }
            }
        }
    };
}

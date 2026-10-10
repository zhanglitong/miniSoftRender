#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 棱台体参数 (Rectangular Frustum)
    /// bottomXlen / bottomYlen / topXlen / topYlen / xOffset / yOffset / height
    /// </summary>
    struct  FEParamPRPY
    {
        /// <summary>下表面 X 长度</summary>
        float   bottomXlen   =   2.0f;
        /// <summary>下表面 Y 长度</summary>
        float   bottomYlen   =   2.0f;
        /// <summary>上表面 X 长度</summary>
        float   topXlen      =   1.0f;
        /// <summary>上表面 Y 长度</summary>
        float   topYlen      =   1.0f;
        /// <summary>上表面中心 X 偏移</summary>
        float   xOffset      =   0.0f;
        /// <summary>上表面中心 Y 偏移</summary>
        float   yOffset      =   0.0f;
        /// <summary>高度</summary>
        float   height        =   1.0f;
    };

    class   FEPRPYImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamPRPY& params,FEPRPYImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const float  halfH   =   params.height * 0.5f;
            const float  bx      =   params.bottomXlen * 0.5f;
            const float  by      =   params.bottomYlen * 0.5f;
            const float  tx      =   params.topXlen    * 0.5f;
            const float  ty      =   params.topYlen    * 0.5f;
            const float  ox      =   params.xOffset;
            const float  oy      =   params.yOffset;

            uint    index   =   0;
            auto    fnPushV =   [&](float x,float y,float z,
                                   float nx,float ny,float nz,
                                   float u,float v)
            {
                vs.insert(vs.end(),{x,y,z});
                if constexpr (bNormal) ns.insert(ns.end(),{nx,ny,nz});
                if constexpr (bUV)     us.insert(us.end(),{u,v});
                return  index++;
            };

            /// 底面 4 顶点 (法线朝下 -Y)
            /// 顺序: (-bx,-by), (bx,-by), (bx,by), (-bx,by)
            uint    b0 = fnPushV(-bx, -halfH, -by, 0,-1,0, 0,0);
            uint    b1 = fnPushV( bx, -halfH, -by, 0,-1,0, 1,0);
            uint    b2 = fnPushV( bx, -halfH,  by, 0,-1,0, 1,1);
            uint    b3 = fnPushV(-bx, -halfH,  by, 0,-1,0, 0,1);

            /// 顶面 4 顶点 (法线朝上 +Y)
            uint    t0 = fnPushV(-tx+ox, halfH, -ty+oy, 0,1,0, 0,0);
            uint    t1 = fnPushV( tx+ox, halfH, -ty+oy, 0,1,0, 1,0);
            uint    t2 = fnPushV( tx+ox, halfH,  ty+oy, 0,1,0, 1,1);
            uint    t3 = fnPushV(-tx+ox, halfH,  ty+oy, 0,1,0, 0,1);

            /// 底面 (与 FEBoxImpl 底面 winding 一致: {a,b,d}+{b,c,d})
            /// a=b3(front-left), b=b0(back-left), c=b1(back-right), d=b2(front-right)
            ids.insert(ids.end(),{b3, b0, b2});
            ids.insert(ids.end(),{b0, b1, b2});

            /// 顶面 (与 FEBoxImpl 顶面 winding 一致)
            /// a=t0(back-left), b=t3(front-left), c=t2(front-right), d=t1(back-right)
            ids.insert(ids.end(),{t0, t3, t1});
            ids.insert(ids.end(),{t3, t2, t1});

            /// 4 个侧面, 每面需要独立法线
            /// winding 与 FEBoxImpl 一致: {a,b,d}+{b,c,d}
            /// a=s3(top-left), b=s0(bottom-left), c=s1(bottom-right), d=s2(top-right)
            auto    fnSide  = [&](float x0,float z0, float x1,float z1,
                                 float x2,float z2, float x3,float z3,
                                 float nx,float ny, float nz)
            {
                uint    s0 = fnPushV(x0, -halfH, z0, nx,ny,nz, 0,0);
                uint    s1 = fnPushV(x1, -halfH, z1, nx,ny,nz, 1,0);
                uint    s2 = fnPushV(x2,  halfH, z2, nx,ny,nz, 1,1);
                uint    s3 = fnPushV(x3,  halfH, z3, nx,ny,nz, 0,1);
                ids.insert(ids.end(),{s3, s0, s2});
                ids.insert(ids.end(),{s0, s1, s2});
            };

            /// 前面 (z=by/ty, 法线 +Z)
            fnSide(-bx, by,  bx, by,  tx+ox, ty+oy, -tx+ox, ty+oy,  0,0,1);
            /// 右面 (x=bx/tx, 法线 +X)
            fnSide( bx, by,  bx,-by,  tx+ox,-ty+oy,  tx+ox, ty+oy,  1,0,0);
            /// 后面 (z=-by/-ty, 法线 -Z)
            fnSide( bx,-by, -bx,-by, -tx+ox,-ty+oy,  tx+ox,-ty+oy,  0,0,-1);
            /// 左面 (x=-bx/-tx, 法线 -X)
            fnSide(-bx,-by, -bx, by, -tx+ox, ty+oy, -tx+ox,-ty+oy, -1,0,0);
        }
    };
}

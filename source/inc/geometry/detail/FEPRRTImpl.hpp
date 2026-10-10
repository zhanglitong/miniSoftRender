#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 方环体参数 (Rectangular Ring Torus)
    /// </summary>
    struct  FEParamPRRT
    {
        /// <summary>内环半径</summary>
        float   innerRadius     =   0.5f;
        /// <summary>外环半径</summary>
        float   outerRadius    =   1.0f;
        /// <summary>高度</summary>
        float   height          =   0.5f;
        /// <summary>环弧角(弧度, 2π=完整圆环)</summary>
        float   arcAngle        =   float(TWO_PI);
        /// <summary>圆周分段数</summary>
        uint    radialSegments  =   32;
    };

    class   FEPRRTImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamPRRT& params,FEPRRTImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const uint  segs    =   std::max(params.radialSegments, 3u);
            const float halfH   =   params.height * 0.5f;
            const float rIn     =   params.innerRadius;
            const float rOut    =   params.outerRadius;
            const float arc     =   params.arcAngle;
            const bool  bFull   =   (arc >= float(TWO_PI) - 1e-4f);

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

            /// 每个角度位置生成 8 个顶点 (4面各2顶点, 法线独立)
            /// 外壁: 下/上, 法线 (ct,0,st)
            /// 内壁: 下/上, 法线 (-ct,0,-st)
            /// 顶面: 内/外, 法线 (0,1,0)
            /// 底面: 内/外, 法线 (0,-1,0)
            struct  PrevIdx
            {
                uint    ob, ot;     /// 外壁
                uint    ib, it;     /// 内壁
                uint    ti, to;     /// 顶面
                uint    bi, bo;     /// 底面
            };
            PrevIdx prev{};

            for (uint i = 0; i <= segs; ++i)
            {
                float   t       =   (float)i / (float)segs;
                float   theta   =   t * arc;
                float   ct      =   std::cos(theta);
                float   st      =   std::sin(theta);

                float   xIn    =   rIn  * ct;
                float   zIn    =   rIn  * st;
                float   xOut   =   rOut * ct;
                float   zOut   =   rOut * st;

                /// 外壁下/上 (法线朝外)
                uint    ob  =   fnPushV(xOut, -halfH, zOut, ct, 0, st, 0, 0);
                uint    ot  =   fnPushV(xOut,  halfH, zOut, ct, 0, st, 0, 1);
                /// 内壁下/上 (法线朝内)
                uint    ib  =   fnPushV(xIn,  -halfH, zIn, -ct, 0, -st, 1, 0);
                uint    it  =   fnPushV(xIn,   halfH, zIn, -ct, 0, -st, 1, 1);
                /// 顶面 内/外 (法线朝上)
                uint    ti  =   fnPushV(xIn,   halfH, zIn,  0, 1, 0, 0, 1);
                uint    to  =   fnPushV(xOut,  halfH, zOut, 0, 1, 0, 1, 1);
                /// 底面 内/外 (法线朝下)
                uint    bi  =   fnPushV(xIn,  -halfH, zIn,  0, -1, 0, 0, 0);
                uint    bo  =   fnPushV(xOut, -halfH, zOut, 0, -1, 0, 1, 0);

                if (i > 0)
                {
                    /// 外侧面 (CCW 从外看)
                    ids.insert(ids.end(),{prev.ob, prev.ot, ob});
                    ids.insert(ids.end(),{ob, prev.ot, ot});
                    /// 内侧面 (CCW 从内看)
                    ids.insert(ids.end(),{prev.ib, ib, prev.it});
                    ids.insert(ids.end(),{ib, it, prev.it});
                    /// 顶面 (CCW 从上看)
                    ids.insert(ids.end(),{prev.ti, ti, prev.to});
                    ids.insert(ids.end(),{ti, to, prev.to});
                    /// 底面 (CCW 从下看)
                    ids.insert(ids.end(),{prev.bi, prev.bo, bi});
                    ids.insert(ids.end(),{bi, prev.bo, bo});
                }
                prev = {ob, ot, ib, it, ti, to, bi, bo};
            }

            /// 部分弧: 两端封口
            if (!bFull && segs > 0)
            {
                float   theta0  =   0;
                float   ct0     =   std::cos(theta0);
                float   st0     =   std::sin(theta0);
                float   theta1  =   arc;
                float   ct1     =   std::cos(theta1);
                float   st1     =   std::sin(theta1);

                /// 起始端面 (法线 -st0, 0, ct0)
                uint    b0ib =   fnPushV(rIn*ct0,  -halfH, rIn*st0,  -st0,0,ct0, 0,0);
                uint    b0ob =   fnPushV(rOut*ct0, -halfH, rOut*st0, -st0,0,ct0, 0,0);
                uint    b0ot =   fnPushV(rOut*ct0,  halfH, rOut*st0, -st0,0,ct0, 0,0);
                uint    b0it =   fnPushV(rIn*ct0,   halfH, rIn*st0,  -st0,0,ct0, 0,0);
                ids.insert(ids.end(),{b0ib, b0ot, b0ob});
                ids.insert(ids.end(),{b0ib, b0it, b0ot});

                /// 终止端面 (法线 st1, 0, -ct1)
                uint    b1ib =   fnPushV(rIn*ct1,  -halfH, rIn*st1,  st1,0,-ct1, 0,0);
                uint    b1ob =   fnPushV(rOut*ct1, -halfH, rOut*st1, st1,0,-ct1, 0,0);
                uint    b1ot =   fnPushV(rOut*ct1,  halfH, rOut*st1, st1,0,-ct1, 0,0);
                uint    b1it =   fnPushV(rIn*ct1,   halfH, rIn*st1,  st1,0,-ct1, 0,0);
                ids.insert(ids.end(),{b1ib, b1ob, b1ot});
                ids.insert(ids.end(),{b1ib, b1ot, b1it});
            }
        }
    };
}

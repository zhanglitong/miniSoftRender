#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    namespace   detail
    {
        /// <summary>
        /// 三次多项式 p(s) = c0 + c1 s + c2 s^2 + c3 s^3
        /// 移植自 three.js r160 CatmullRomCurve3.js 内部 CubicPoly
        /// </summary>
        struct  FECubicPoly
        {
            float   c0 = 0, c1 = 0, c2 = 0, c3 = 0;

            /// p(0)=x0, p(1)=x1, p'(0)=t0, p'(1)=t1
            void    init(float x0, float x1, float t0, float t1)
            {
                c0  =   x0;
                c1  =   t0;
                c2  =   -3.0f * x0 + 3.0f * x1 - 2.0f * t0 - t1;
                c3  =   2.0f * x0 - 2.0f * x1 + t0 + t1;
            }

            void    initCatmullRom(float x0, float x1, float x2, float x3, float tension)
            {
                init(x1, x2, tension * (x2 - x0), tension * (x3 - x1));
            }

            void    initNonuniformCatmullRom(float x0, float x1, float x2, float x3,
                                             float dt0, float dt1, float dt2)
            {
                /// [t1,t2] 参数化下的端点切线
                float   t1  =   (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
                float   t2  =   (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;
                /// 缩放回 [0,1] 参数域
                t1  *=  dt1;
                t2  *=  dt1;
                init(x1, x2, t1, t2);
            }

            float   calc(float t) const
            {
                float   t2  =   t * t;
                float   t3  =   t2 * t;
                return  c0 + c1 * t + c2 * t2 + c3 * t3;
            }
        };
    }

    /// <summary>
    /// 3D Catmull-Rom 样条, 移植自 three.js r160 extras/curves/CatmullRomCurve3.js
    /// 支持向心(centripetal)/弦长(chordal)/均匀(uniform)参数化
    /// </summary>
    class   FECatmullRomCurve3 : public FECurve
    {
    public:
        float3s             points;
        bool                closed      =   false;
        FECatmullRomType    curveType   =   FECatmullRomType::Centripetal;
        float               tension     =   0.5f;

        FECatmullRomCurve3() =   default;
        FECatmullRomCurve3(const float3s& controlPoints,
                           bool closeCurve = false,
                           FECatmullRomType type = FECatmullRomType::Centripetal,
                           float ten = 0.5f)
            : points(controlPoints), closed(closeCurve), curveType(type), tension(ten)
        {}

        virtual float3  getPoint(float t) const override
        {
            const float3s&  pts =   points;
            const size_t    l   =   pts.size();

            float   p           =   (float)(l - (closed ? 0 : 1)) * t;
            int     intPoint    =   (int)std::floor(p);
            float   weight      =   p - (float)intPoint;

            if (closed)
            {
                if (intPoint < 0)
                    intPoint   +=   (int)(((size_t)std::floor(std::abs((float)intPoint) / (float)l) + 1) * l);
            }
            else if (weight == 0.0f && intPoint == (int)l - 1)
            {
                intPoint    =   (int)l - 2;
                weight      =   1.0f;
            }

            float3  p0, p3;
            if (closed || intPoint > 0)
            {
                p0  =   pts[((size_t)intPoint - 1) % l];
            }
            else
            {
                /// 首点外推
                p0  =   pts[0] + (pts[0] - pts[1]);
            }

            float3  p1  =   pts[(size_t)intPoint % l];
            float3  p2  =   pts[((size_t)intPoint + 1) % l];

            if (closed || (size_t)intPoint + 2 < l)
            {
                p3  =   pts[((size_t)intPoint + 2) % l];
            }
            else
            {
                /// 末点外推
                p3  =   pts[l - 1] + (pts[l - 1] - pts[l - 2]);
            }

            detail::FECubicPoly px, py, pz;

            if (curveType == FECatmullRomType::Centripetal || curveType == FECatmullRomType::Chordal)
            {
                float   powv    =   (curveType == FECatmullRomType::Chordal) ? 0.5f : 0.25f;

                float   dt0 =   std::pow(FE::distance2(p0, p1), powv);
                float   dt1 =   std::pow(FE::distance2(p1, p2), powv);
                float   dt2 =   std::pow(FE::distance2(p2, p3), powv);

                /// 重复点保护
                if (dt1 < 1e-4f) dt1 =   1.0f;
                if (dt0 < 1e-4f) dt0 =   dt1;
                if (dt2 < 1e-4f) dt2 =   dt1;

                px.initNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2);
                py.initNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2);
                pz.initNonuniformCatmullRom(p0.z, p1.z, p2.z, p3.z, dt0, dt1, dt2);
            }
            else
            {
                px.initCatmullRom(p0.x, p1.x, p2.x, p3.x, tension);
                py.initCatmullRom(p0.y, p1.y, p2.y, p3.y, tension);
                pz.initCatmullRom(p0.z, p1.z, p2.z, p3.z, tension);
            }

            return  float3(px.calc(weight), py.calc(weight), pz.calc(weight));
        }
    };
}

#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    namespace   detail
    {
        /// <summary>
        /// 三次多项式模板 p(s) = c0 + c1 s + c2 s^2 + c3 s^3
        /// 移植自 three.js r160 CatmullRomCurve3.js 内部 CubicPoly
        /// 模板参数 T: 浮点类型(float/double)
        /// </summary>
        template<typename T>
        struct  FECubicPolyT
        {
            T   c0 = T(0), c1 = T(0), c2 = T(0), c3 = T(0);

            /// p(0)=x0, p(1)=x1, p'(0)=t0, p'(1)=t1
            void    init(T x0, T x1, T t0, T t1)
            {
                c0  =   x0;
                c1  =   t0;
                c2  =   T(-3) * x0 + T(3) * x1 - T(2) * t0 - t1;
                c3  =   T(2) * x0 - T(2) * x1 + t0 + t1;
            }

            void    initCatmullRom(T x0, T x1, T x2, T x3, T tension)
            {
                init(x1, x2, tension * (x2 - x0), tension * (x3 - x1));
            }

            void    initNonuniformCatmullRom(T x0, T x1, T x2, T x3,
                                             T dt0, T dt1, T dt2)
            {
                /// [t1,t2] 参数化下的端点切线
                T   t1  =   (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
                T   t2  =   (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;
                /// 缩放回 [0,1] 参数域
                t1  *=  dt1;
                t2  *=  dt1;
                init(x1, x2, t1, t2);
            }

            T   calc(T t) const
            {
                T   t2  =   t * t;
                T   t3  =   t2 * t;
                return  c0 + c1 * t + c2 * t2 + c3 * t3;
            }
        };
    }

    /// <summary>
    /// 3D Catmull-Rom 样条模板, 移植自 three.js r160 extras/curves/CatmullRomCurve3.js
    /// 支持向心(centripetal)/弦长(chordal)/均匀(uniform)参数化
    /// 模板参数 T: 浮点类型(float/double)
    /// </summary>
    template<typename T>
    class   FECatmullRomCurve3T : public FECurveT<T>
    {
    public:
        using   Point   =   tvec3<T>;
        using   Points  =   std::vector<Point>;

        Points              points;
        bool                closed      =   false;
        FECatmullRomType    curveType   =   FECatmullRomType::Centripetal;
        T                   tension     =   T(0.5);

        FECatmullRomCurve3T() =   default;
        FECatmullRomCurve3T(const Points& controlPoints,
                           bool closeCurve = false,
                           FECatmullRomType type = FECatmullRomType::Centripetal,
                           T ten = T(0.5))
            : points(controlPoints), closed(closeCurve), curveType(type), tension(ten)
        {}

        virtual Point   getPoint(T t) const override
        {
            const Points&   pts =   points;
            const size_t    l   =   pts.size();

            T   p           =   static_cast<T>(l - (closed ? 0 : 1)) * t;
            int intPoint    =   static_cast<int>(std::floor(p));
            T   weight      =   p - static_cast<T>(intPoint);

            if (closed)
            {
                if (intPoint < 0)
                    intPoint   +=   static_cast<int>(((size_t)std::floor(std::abs(static_cast<T>(intPoint)) / static_cast<T>(l)) + 1) * l);
            }
            else if (weight == T(0) && intPoint == static_cast<int>(l) - 1)
            {
                intPoint    =   static_cast<int>(l) - 2;
                weight      =   T(1);
            }

            Point   p0, p3;
            if (closed || intPoint > 0)
            {
                p0  =   pts[((size_t)intPoint - 1) % l];
            }
            else
            {
                /// 首点外推
                p0  =   pts[0] + (pts[0] - pts[1]);
            }

            Point   p1  =   pts[(size_t)intPoint % l];
            Point   p2  =   pts[((size_t)intPoint + 1) % l];

            if (closed || (size_t)intPoint + 2 < l)
            {
                p3  =   pts[((size_t)intPoint + 2) % l];
            }
            else
            {
                /// 末点外推
                p3  =   pts[l - 1] + (pts[l - 1] - pts[l - 2]);
            }

            detail::FECubicPolyT<T> px, py, pz;

            if (curveType == FECatmullRomType::Centripetal || curveType == FECatmullRomType::Chordal)
            {
                T   powv    =   (curveType == FECatmullRomType::Chordal) ? T(0.5) : T(0.25);

                T   dt0 =   std::pow(FE::distance2(p0, p1), powv);
                T   dt1 =   std::pow(FE::distance2(p1, p2), powv);
                T   dt2 =   std::pow(FE::distance2(p2, p3), powv);

                /// 重复点保护
                if (dt1 < T(1e-4)) dt1 =   T(1);
                if (dt0 < T(1e-4)) dt0 =   dt1;
                if (dt2 < T(1e-4)) dt2 =   dt1;

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

            return  Point(px.calc(weight), py.calc(weight), pz.calc(weight));
        }
    };

    /// 单精度别名 (向后兼容)
    using   FECatmullRomCurve3   =   FECatmullRomCurve3T<float>;
    /// 双精度别名
    using   FECatmullRomCurve3D  =   FECatmullRomCurve3T<double>;
}

#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 三次贝塞尔曲线模板, 移植自 three.js r160 extras/curves/CubicBezierCurve3.js
    /// B(t) = (1-t)^3 v0 + 3(1-t)^2 t v1 + 3(1-t)t^2 v2 + t^3 v3
    /// 模板参数 T: 浮点类型(float/double)
    /// </summary>
    template<typename T>
    class   FECubicBezierCurve3T : public FECurveT<T>
    {
    public:
        using   Point   =   tvec3<T>;

        Point   v0  =   Point(T(0), T(0), T(0));
        Point   v1  =   Point(T(0), T(0), T(0));
        Point   v2  =   Point(T(0), T(0), T(0));
        Point   v3  =   Point(T(0), T(0), T(0));

        FECubicBezierCurve3T() =   default;
        FECubicBezierCurve3T(const Point& p0, const Point& p1, const Point& p2, const Point& p3)
            : v0(p0), v1(p1), v2(p2), v3(p3)
        {}

        virtual Point   getPoint(T t) const override
        {
            T   it  =   T(1) - t;
            return  it * it * it * v0
                 + T(3) * it * it * t * v1
                 + T(3) * it * t * t * v2
                 + t * t * t * v3;
        }
    };

    /// <summary>
    /// 单精度别名 (向后兼容)
    /// </summary>
    using   FECubicBezierCurve3   =   FECubicBezierCurve3T<float>;
    /// <summary>
    /// 双精度别名
    /// </summary>
    using   FECubicBezierCurve3D  =   FECubicBezierCurve3T<double>;
}

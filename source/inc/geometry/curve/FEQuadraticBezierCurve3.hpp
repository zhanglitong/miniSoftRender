#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 二次贝塞尔曲线模板, 移植自 three.js r160 extras/curves/QuadraticBezierCurve3.js
    /// B(t) = (1-t)^2 v0 + 2(1-t)t v1 + t^2 v2
    /// 模板参数 T: 浮点类型(float/double)
    /// </summary>
    template<typename T>
    class   FEQuadraticBezierCurve3T : public FECurveT<T>
    {
    public:
        using   Point   =   tvec3<T>;

        Point   v0  =   Point(T(0), T(0), T(0));
        Point   v1  =   Point(T(0), T(0), T(0));
        Point   v2  =   Point(T(0), T(0), T(0));

        FEQuadraticBezierCurve3T() =   default;
        FEQuadraticBezierCurve3T(const Point& p0, const Point& p1, const Point& p2)
            : v0(p0), v1(p1), v2(p2)
        {}

        virtual Point   getPoint(T t) const override
        {
            T   it  =   T(1) - t;
            return  it * it * v0 + T(2) * it * t * v1 + t * t * v2;
        }
    };

    /// 单精度别名 (向后兼容)
    using   FEQuadraticBezierCurve3   =   FEQuadraticBezierCurve3T<float>;
    /// 双精度别名
    using   FEQuadraticBezierCurve3D  =   FEQuadraticBezierCurve3T<double>;
}

#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 直线段模板, 移植自 three.js r160 extras/curves/LineCurve3.js
    /// 模板参数 T: 浮点类型(float/double)
    /// </summary>
    template<typename T>
    class   FELineCurve3T : public FECurveT<T>
    {
    public:
        using   Point   =   tvec3<T>;

        Point   v1  =   Point(T(0), T(0), T(0));
        Point   v2  =   Point(T(0), T(0), T(0));

        FELineCurve3T()  =   default;
        FELineCurve3T(const Point& p1, const Point& p2)
            : v1(p1), v2(p2)
        {}

        virtual Point   getPoint(T t) const override
        {
            if (t == T(1))
                return  v2;
            return  v1 + (v2 - v1) * t;
        }

        /// <summary>
        /// 直线的弧长参数与 t 一致
        /// </summary>
        virtual Point   getTangent(T /*t*/) const override
        {
            return  FE::normalize(v2 - v1);
        }
    };

    /// <summary>
    /// 单精度别名 (向后兼容)
    /// </summary>
    using   FELineCurve3   =   FELineCurve3T<float>;
    /// <summary>
    /// 双精度别名
    /// </summary>
    using   FELineCurve3D  =   FELineCurve3T<double>;
}

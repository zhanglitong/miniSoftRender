#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 二次贝塞尔曲线, 移植自 three.js r160 extras/curves/QuadraticBezierCurve3.js
    /// B(t) = (1-t)^2 v0 + 2(1-t)t v1 + t^2 v2
    /// </summary>
    class   FEQuadraticBezierCurve3 : public FECurve
    {
    public:
        float3  v0  =   float3(0, 0, 0);
        float3  v1  =   float3(0, 0, 0);
        float3  v2  =   float3(0, 0, 0);

        FEQuadraticBezierCurve3() =   default;
        FEQuadraticBezierCurve3(const float3& p0, const float3& p1, const float3& p2)
            : v0(p0), v1(p1), v2(p2)
        {}

        virtual float3  getPoint(float t) const override
        {
            float   it  =   1.0f - t;
            return  it * it * v0 + 2.0f * it * t * v1 + t * t * v2;
        }
    };
}

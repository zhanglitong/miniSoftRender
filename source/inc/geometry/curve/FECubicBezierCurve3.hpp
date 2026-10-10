#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 三次贝塞尔曲线, 移植自 three.js r160 extras/curves/CubicBezierCurve3.js
    /// B(t) = (1-t)^3 v0 + 3(1-t)^2 t v1 + 3(1-t)t^2 v2 + t^3 v3
    /// </summary>
    class   FECubicBezierCurve3 : public FECurve
    {
    public:
        float3  v0  =   float3(0, 0, 0);
        float3  v1  =   float3(0, 0, 0);
        float3  v2  =   float3(0, 0, 0);
        float3  v3  =   float3(0, 0, 0);

        FECubicBezierCurve3() =   default;
        FECubicBezierCurve3(const float3& p0, const float3& p1, const float3& p2, const float3& p3)
            : v0(p0), v1(p1), v2(p2), v3(p3)
        {}

        virtual float3  getPoint(float t) const override
        {
            float   it  =   1.0f - t;
            return  it * it * it * v0
                 + 3.0f * it * it * t * v1
                 + 3.0f * it * t * t * v2
                 + t * t * t * v3;
        }
    };
}

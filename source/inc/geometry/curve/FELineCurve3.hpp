#pragma     once

#include    "FECurve.hpp"

namespace   FE
{
    /// <summary>
    /// 3D 直线段, 移植自 three.js r160 extras/curves/LineCurve3.js
    /// </summary>
    class   FELineCurve3 : public FECurve
    {
    public:
        float3  v1  =   float3(0, 0, 0);
        float3  v2  =   float3(0, 0, 0);

        FELineCurve3()  =   default;
        FELineCurve3(const float3& p1, const float3& p2)
            : v1(p1), v2(p2)
        {}

        virtual float3  getPoint(float t) const override
        {
            if (t == 1.0f)
                return  v2;
            return  v1 + (v2 - v1) * t;
        }

        /// 直线的弧长参数与 t 一致
        virtual float3  getTangent(float /*t*/) const override
        {
            return  FE::normalize(v2 - v1);
        }
    };
}

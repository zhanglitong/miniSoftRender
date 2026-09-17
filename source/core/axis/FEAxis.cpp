
#include    "../../inc/axis/FEAxis.h"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/FECamera.hpp"
#include    <cmath>

namespace   FE
{
    FEAxis::FEAxis(AxisType , FEContext& ctx)
        :FEInputCom(ctx)
    {
        _axisType       =   AxisTypeBase;
        _transform      =   mat4r(1.0);
        _internalFlags  =   { InteralFlag_Update, InteralFlag_Visible };
    }
    FEAxis::~FEAxis()
    {
    }

    real3 FEAxis::vectorPerpendicularToAxisX() const
    {
        real3   tAxis   =   axisY();
        if (std::abs(dot(tAxis, axisX())) > FLT_EPSILON)
        {
            real3   tY  =   FEAxis::AxisY();
            real3   tZ  =   FEAxis::AxisZ();
            real    d   =   dot(tY, axisX());
            if (1.0 - std::abs(d) > FLT_EPSILON)
                tAxis = normalize(cross(axisX(), tY));
            else
                tAxis = normalize(cross(axisX(), tZ));
        }
        real    d   =   std::abs(dot(tAxis, axisX()));
        assert(d <= FLT_EPSILON);
        return tAxis;
    }
    real3 FEAxis::vectorPerpendicularToAxisY() const
    {
        real3   tAxis   =   axisX();
        if (std::abs(dot(tAxis, axisX())) > FLT_EPSILON)
        {
            real3   tZ  =   FEAxis::AxisZ();
            real3   tX  =   FEAxis::AxisX();
            real    d   =   dot(tZ, axisY());
            if (1.0 - std::abs(d) > FLT_EPSILON)
                tAxis = normalize(cross(axisY(), tZ));
            else
                tAxis = normalize(cross(axisY(), tX));
        }
        real    d   =   std::abs(dot(tAxis, axisY()));
        assert(d <= FLT_EPSILON);
        return tAxis;
    }
    real3 FEAxis::vectorPerpendicularToAxisZ() const
    {
        real3   tAxis   =   axisX();
        if (std::abs(dot(tAxis, axisZ())) > FLT_EPSILON)
        {
            real3   tX  =   FEAxis::AxisX();
            real3   tY  =   FEAxis::AxisY();
            real    d   =   dot(tX, axisZ());
            if (1.0 - std::abs(d) > FLT_EPSILON)
                tAxis = normalize(cross(axisZ(), tX));
            else
                tAxis = normalize(cross(axisZ(), tY));
        }
        real    d   =   std::abs(dot(tAxis, axisZ()));
        assert(d <= FLT_EPSILON);
        return tAxis;
    }

    bool FEAxis::calcAxisPlaneNormalize(FECamera& camera, const real3& axis, real3& outNor) const
    {
        real3   eye =   camera.getEye();
        real    d   =   dot(eye - this->position(), axis);
        if (std::abs(1 - std::abs(d)) <= FLT_EPSILON)
        {
            /// 摄像机眼睛与axis接近共线，无法计算构成的平面法线
            outNor  =   camera.getDir();
            return false;
        }
        real3   p   = position() + axis * d;
        outNor = normalize(p - eye);
        return true;
    }

    FEEditAxis::FEEditAxis(EditAxisType type, FEContext& ctx)
        :FEAxis(AxisTypeEdit, ctx)
    {
        _editAxisType = type;
    }
    FEEditAxis::~FEEditAxis() 
    {
    }
}

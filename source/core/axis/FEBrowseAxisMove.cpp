#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEBrowseAxisMove.h"

namespace   FE
{
    FEBrowseAxisMove::FEBrowseAxisMove(FEContext& context)
        : FEBrowseAxis(BrowseAxisType::BrowseAxisTypeMove, context)
    {
    }

    FEBrowseAxisMove::~FEBrowseAxisMove()
    {
    }

    void    FEBrowseAxisMove::updateAxisVerties(FEContext& context)
    {
        FECamera&   camera  =   context.activeCamera();
        real3       cenPos  =   real3(0, 0, 0);
        real        unitF   =   camera.pixelU(position());
        real        size    =   unitF * 40.0;
        ///center
        _moveAxis[0]    =   real3(0, 0, 0);
        ///X
        _moveAxis[1]    =   axisX() * size;
        ///y
        _moveAxis[2]    =   axisY() * size;
        ///z
        _moveAxis[3]    =   axisZ() * size;
        ///计算箭头
        real    arSize      =   unitF * 25.0;
        real    arRadius    =   unitF * 7.0;
        _axisAr.clear();
        real    step        =   12.0;
        ///arX
        {
            real3   tAxis   =   vectorPerpendicularToAxisX();
            _axisAr.push_back(axisX() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r   rMat    =   FE::rotate(mat4r(1), real(i) * step, axisX());
                real3   nor     =   normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
        }
        ///ary
        {
            real3   tAxis   =   vectorPerpendicularToAxisY();
            _axisAr.push_back(axisY() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r   rMat    =   FE::rotate(mat4r(1), real(i) * step, axisY());
                real3   nor     =   normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
        }
        ///arZ
        {
            real3   tAxis   =   vectorPerpendicularToAxisZ();
            _axisAr.push_back(axisZ() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r   rMat    =   FE::rotate(mat4r(1), real(i) * step, axisZ());
                real3   nor     =   normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
        }
    }
}

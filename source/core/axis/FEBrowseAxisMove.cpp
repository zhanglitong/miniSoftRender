#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEBrowseAxisMove.h"

namespace   FE
{
    class FEBrowseAxisMovePrivate
    {
    private:
        static constexpr float COLOR_ALPHA = 0.7f;
        static constexpr float COLOR_VALUE = 0.6f;
        static constexpr float AXIS_WIDTH = 6.0f;
    public:
        ///
        FEBrowseAxisMove& _d;
        ///轴
        FE::float3        _moveAxis[4];
        ///箭头
        std::vector<float3> _axisAr;
    public:
        FEBrowseAxisMovePrivate(FEBrowseAxisMove& d):_d(d) {}
    public:
        void updateAxisVerties(FEContext& context)
        {
            FECamera& camera  =   context.activeCamera();
            real3       cenPos  =   real3(0, 0, 0);
            real        unitF   =   camera.pixelU(_d.position());
            real        size    =   unitF * 40.0;
            ///center
            _moveAxis[0] = real3(0, 0, 0);
            ///X
            _moveAxis[1] = _d.axisX() * size;
            ///y
            _moveAxis[2] = _d.axisY() * size;
            ///z
            _moveAxis[3] = _d.axisZ() * size;
            ///计算箭头
            real arSize = unitF * 25.0;
            real arRadius = unitF * 7.0;
            _axisAr.clear();
            real step = 12.0;
            ///arX
            {
                real3 tAxis = _d.vectorPerpendicularToAxisX();
                _axisAr.push_back(_d.axisX() * arSize);
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisX());
                    real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                    _axisAr.push_back(nor * arRadius);
                }
            }
            ///ary
            {
                real3 tAxis = _d.vectorPerpendicularToAxisY();
                _axisAr.push_back(_d.axisY() * arSize);
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisY());
                    real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                    _axisAr.push_back(nor * arRadius);
                }
            }
            ///arZ
            {
                real3 tAxis = _d.vectorPerpendicularToAxisZ();
                _axisAr.push_back(_d.axisZ() * arSize);
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisZ());
                    real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                    _axisAr.push_back(nor * arRadius);
                }
            }
        }
        void renderAxis(FEContext& context)
        {
#if 0



            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();

            float lineWidth = context.device()->getLineWidth();

            device.bindVertexBuffer(0);
            device.bindIndexBuffer(0);

            device.disableRenderState(GL_DEPTH_TEST);
            device.enableRenderState(GL_BLEND);
            device.disableRenderState(GL_CULL_FACE);

            ///绘制箭头
            renderAr(context);
            ///绘制轴线
            prg.begin(cmdBuf);
            {
                mat4r tMat(1.0);
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(_moveAxis[0]), &_moveAxis[0]);
                {//绘制轴
                    for (int i = 0; i < 3; i++)
                    {
                        float color[4] = { 0.0f, 0.0f, 0.0f, COLOR_ALPHA };
                        color[i] = COLOR_VALUE;
                        context.device()->setLineWidth(AXIS_WIDTH);
                        FE::byte index[] =
                        {
                            FE::byte(0),          FE::byte(i + 1),
                        };
                        device.setUniform4fv(prg._color, 1, color);
                        device.drawElements(PT_LINES, IndexUint8,sizeof(index), size_t(index));
                    }
                }
            }
            prg.end(cmdBuf);

            device.enableRenderState(GL_CULL_FACE);
            device.enableRenderState(GL_DEPTH_TEST);

            device.bindVertexBuffer(0);
            device.bindIndexBuffer(0);

            context.device()->setLineWidth(lineWidth);
#endif // 0
        }
    private:
        void renderAr(FEContext &context)
        {
#if 0


            if (_axisAr.empty())
                return;

            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();

            prg.begin(cmdBuf);
            {
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(_axisAr[0]), &_axisAr[0]);
                for (int i = 0; i < 3; ++i)
                {
                    mat4r tMat0;
                    tMat0 = FE::translate(tMat0, _d.position());
                    mat4r tMat1;
                    tMat1 = FE::translate(tMat1, _moveAxis[i + 1]);
                    tMat1 = context.mvp() * tMat0 * tMat1;
                    device.setUniformMatrix4dv(prg._MVP, 1, false, tMat1.data());
                    float color[4] = { 0.0f, 0.0f, 0.0f, COLOR_ALPHA };
                    color[i] = COLOR_VALUE;
                    device.setUniform4fv(prg._color, 1, color);
                    device.drawArrays(PT_TRIANGLE_FAN, 32 * i, 32);
                }
            }
            prg.end(cmdBuf);

#endif // 0
        }
    };
    FEBrowseAxisMove::FEBrowseAxisMove(FEContext& context)
        : FEBrowseAxis(BrowseAxisType::BrowseAxisTypeMove, context)
    {
        _p = new FEBrowseAxisMovePrivate(*this);
    }

    FEBrowseAxisMove::~FEBrowseAxisMove()
    {
        delete _p;
    }

    void FEBrowseAxisMove::update(FEContext& context)
    {
        _p->updateAxisVerties(context);
    }

    void FEBrowseAxisMove::render(FEContext& context)
    {
        if (!_internalFlags.hasFlag(InteralFlag_Visible))
            return;
        _p->renderAxis(context);
    }
}

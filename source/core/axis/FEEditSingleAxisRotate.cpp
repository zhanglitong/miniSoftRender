#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditSingleAxisRotate.h"

namespace   FE
{
    class   FEEditSingleAxisRotatePrivate
    {
        static constexpr real ConeHeight = 70.0;
        static constexpr real ConeBottomRadiu = 20.0;
    public:
        ///
        FEEditSingleAxisRotate & _d;

        ///轴顶点
        FE::float3 _moveAxis[13];
        ///箭头顶点
        std::vector<float3> _axisAr;
        ///圆弧
        std::vector<float3> _vRotateAxis;
        ///切线
        float3 _tangent[8];
        ///旋转中扇形
        std::vector<float3> _fan;

        ///弧半径
        real _radiusArc;

        ///旋转轴朝向
        real3 _axis;

        ///是否选中轴
        bool _bSelectedAxis;
        ///是否高亮轴
        bool _bHoveredAxis;

        ///角度偏移
        real _offAngle;
        ///是否正在旋转
        bool _bRotatting;

        ///鼠标按钮按下
        bool _bMouseDown;
        ///鼠标按下的最后位置
        int2 _downPos;
        ///开始时的鼠标位置
        int2 _startPos;

        ///触屏按下
        bool _bTouchDown;
        ///触屏拾取轴
        bool _bTouchPickup;
        ///触屏按下的最后位置
        int2 _touchDownPos;
        ///触屏开始时按下的位置
        int2 _touchStartPos;

        ///正向最大限制距离
        real _forwardAngleMaxLimit;
        ///反向最大限制距离
        real _reverseAngleMaxLimit;

        ///回调
        FEEditSingleAxisRotate::MDelegate _delegate;
    public:
        FEEditSingleAxisRotatePrivate(FEEditSingleAxisRotate& d) :_d(d)
        {
            _radiusArc = 0.0;

            _axis = real3(0.0, 0.0, 1.0);

            _offAngle = 0.0;
            _bRotatting = false;

            _bSelectedAxis = false;
            _bHoveredAxis = false;

            _bMouseDown = false;

            _forwardAngleMaxLimit = FLT_MAX;
            _reverseAngleMaxLimit = FLT_MAX;

        }
    public:
        ///更新轴顶点
        void updateAxisVerties(FEContext& context) 
        {
            ///更新轴向箭头
            updateArrowVerties(context);
            ///更新圆弧
            updateArcVerties(context);
        }
        ///绘制轴
        void renderAxis(FEContext& context)
        {
#if 0


            FE::FEDevice& device = *(context.device());

            device.bindVertexBuffer(0);
            device.bindIndexBuffer(0);

            float lineWidth = device.getLineWidth();
            float oldPSize = device.getPointSize();

            device.disableRenderState(GL_PROGRAM_POINT_SIZE);
            device.disableRenderState(GL_DEPTH_TEST);
            device.disableRenderState(GL_CULL_FACE);

            ///如果正在旋转，绘制扇
            if (_bRotatting) 
                renderFan(context);
            ///绘制轴向箭头
            renderAxisArrow(context);
            ///绘制圆弧
            renderArc(context);
            ///如果正在旋转，绘制切线
            if (_bRotatting) 
                renderTangent(context);

            device.enableRenderState(GL_PROGRAM_POINT_SIZE);
            device.enableRenderState(GL_DEPTH_TEST);
            device.enableRenderState(GL_CULL_FACE);

            device.bindVertexBuffer(0);
            device.bindIndexBuffer(0);

            device.setLineWidth(lineWidth);
            device.setPointSize(oldPSize);

#endif // 0
        }
        ///移动计算
        void calcRotate(FEContext& context, const FE::int2& start, const FE::int2 &end, real3 &outAxis, real& outAngle) 
        {
            outAngle = calcOffAngle(context, start, end);
            outAxis = _d.axis();
            _offAngle = outAngle;
        }
        ///高亮轴
        bool hoverAxis(FEContext& context, const int2& pos)
        {
            bool oldHovered = _bHoveredAxis;
            _bHoveredAxis = false;
            static const real DIST_MAX = 6;
            FECamera& camera = context.activeCamera();

            real3 mouse(pos.x, pos.y, 0);
            for (size_t i = 0; i < _vRotateAxis.size() - 1; ++i)
            {///x
                real2 startX = camera.worldToScreen(real3(_vRotateAxis[i]) + _d.position());
                real2 endX = camera.worldToScreen(real3(_vRotateAxis[i + 1]) + _d.position());
                real dist = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(endX.x, endX.y, 0), mouse);
                if (dist < DIST_MAX)
                {
                    _bHoveredAxis = true;
                }
            }

            if (oldHovered != _bHoveredAxis)
            {
                _d.sendHoveredDelegate();
            }
            return _bHoveredAxis;
        }
    private:
        void updateArrowVerties(FEContext& context)
        {
            FECamera& camera  =   context.activeCamera();
            real3       cenPos  =   real3(0, 0, 0);
            real        unitF   =   camera.pixelU(_d.position());
            real        size    =   unitF * 40;
            ///center
            _moveAxis[0] = real3(0, 0, 0);
            ///vec
            _moveAxis[1] = _d.axis() * size;

            ///计算箭头
            real arSize = unitF * 24;
            real arRadius = unitF * 5;
            real step = 12.0;
            ///
            _axisAr.clear();
            ///arr
            real3 tAxis = vectorPerpendicularToDir();
            _axisAr.push_back(_d.axis() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r rMat(1.0);
                rMat = FE::rotate(rMat, real(i) * step, _d.axis());
                real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
        }
        void updateArcVerties(FEContext& context)
        {
            _vRotateAxis.clear();
            FECamera& camera = context.activeCamera();
            real unitF = camera.pixelU(_d.position());
            _radiusArc = unitF * 60;
            real3 dir = camera.getDir();
            real step = 12.0;
            real min = 0.3;
            real3 tAxis = vectorPerpendicularToDir();
            int index = 0;
            int insertIndex = 0;
            for (int i = 0; i <= 30; ++i)
            {
                mat4r rMat(1.0);
                rMat = FE::rotate(rMat, real(i) * step, _d.axis());
                real3 pt = normalize(real3(rMat * real4(tAxis, 0.0))) * _radiusArc;
                real3 norv = FE::normalize(pt);
                if (FE::dot(norv, dir) < min)
                {
                    if (index == 0)
                        _vRotateAxis.push_back(pt);
                    else
                    {
                        _vRotateAxis.insert(_vRotateAxis.begin() + insertIndex, pt);
                        insertIndex++;
                    }
                }
                else
                    index = i;
            }
        }

        void renderAxisArrow(FEContext& context)
        {
#if 0



            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();
            ///绘制轴线
            prg.begin(cmdBuf);
            {
                mat4r tMat(1.0);
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(_moveAxis[0]), &_moveAxis[0]);
                float color[4] = {0.6f, 0.6f, 0.6f, 1.0f};
                if (_bHoveredAxis)
                {
                    color[0] = 0.6f;
                    color[1] = 0.0f;
                    color[2] = 0.8f;
                    context.device()->setLineWidth(2);
                }
                else
                {
                    context.device()->setLineWidth(1);
                }
                device.setUniform4fv(prg._color, 1, color);
                device.drawArrays(PT_LINES, 0, 2);
            }
            prg.end(cmdBuf);

            ///绘制端点
            renderPoint(context, 8);
            ///绘制箭头
            renderAr(context);
#endif // 0
        }
        void renderPoint(FEContext& context, float pointSize)
        {
#if 0
            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;

            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();

            device.setPointSize(pointSize);

            float3 pt = float3(0);
            prg.begin(cmdBuf);
            {
                mat4r tMat(1.0);
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(FE::float3), &pt);
                float color[4] = { 0.6f, 0.0f, 0.8f, 0.6f };
                device.setUniform4fv(prg._color, 1, color);
                device.drawArrays(PT_POINTS, 0, (GLsizei)1);
            }
            prg.end(cmdBuf);
#endif // 0
        }
        void renderAr(FEContext& context)
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
                for (int i = 0; i < 1; ++i)
                {
                    mat4r tMat0;
                    tMat0 = FE::translate(tMat0, _d.position());
                    mat4r tMat1;
                    tMat1 = FE::translate(tMat1, _moveAxis[i + 1]);
                    tMat1 = context.mvp() * tMat0 * tMat1;
                    device.setUniformMatrix4dv(prg._MVP, 1, false, tMat1.data());
                    float color[4] = { 0.6f, 0.0f, 0.8f, 0.6f };
                    device.setUniform4fv(prg._color, 1, color);
                    device.drawArrays(PT_TRIANGLE_FAN, 32 * i, 32);
                }
            }
            prg.end(cmdBuf);
#endif // 0
        } 
        void renderArc(FEContext& context)
        {
#if 0


            if (_vRotateAxis.empty())
                return;
            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();

            prg.begin(cmdBuf);
            {//绘制弧线
                float color[4] = { 0.6f, 0.0f, 0.8f, 1.0f };
                context.device()->setLineWidth(1.0f);
                if (_d.isAxisHovered())
                {
                    context.device()->setLineWidth(2.0f);
                    color[0] = 1.0f;
                    color[1] = 1.0f;
                    color[2] = 0.0f;
                }
                mat4r tMat;
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(float3), &_vRotateAxis[0]);
                device.setUniform4fv(prg._color, 1, color);
                device.drawArrays(PT_LINE_STRIP, 0, GLsizei(_vRotateAxis.size()));
            }
            prg.end(cmdBuf);

#endif // 0
        }
        void renderFan(FEContext& context)
        {
#if 0

            if (_fan.empty())
                return;
            if (!_d.isAxisSelected())
                return;
            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();

            prg.begin(device.cmdBuffer());
            {//绘制扇
                mat4r tMat;
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(float3), &_fan.front());
                float color[4] = { 1.0f,1.0f,0.0f,0.4f };
                device.setUniform4fv(prg._color, 1, color);
                device.drawArrays(PT_TRIANGLE_FAN, 0, GLsizei(_fan.size()));
            }
            prg.end(device.cmdBuffer());


#endif // 0
        }
        void renderTangent(FEContext& context)
        {
#if 0

            if (!_d.isAxisSelected())
                return;
            auto&       prg     =   context._resource->_PROGRAM_P3_UC4;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();
            prg.begin(cmdBuf);
            {//绘制切线
                mat4r tMat;
                tMat = FE::translate(tMat, _d.position());
                tMat = context.mvp() * tMat;
                device.setUniformMatrix4dv(prg._MVP, 1, false, tMat.data());
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(_tangent[0]), &_tangent[0]);
                float color[2][4] = { 
                    { 0.6f, 0.6f, 0.6f,1.0f }
                    ,{ 0.6f, 0.6f, 0.6f,1.0f } 
                };
                if (_d.isAxisSelected())
                {
                    if (_offAngle > 0.0)
                    {
                        color[0][0] = 0.6f;
                        color[0][1] = 0.0f;
                        color[0][2] = 0.8f;
                    }
                    else if (_offAngle < 0.0)
                    {
                        color[1][0] = 0.6f;
                        color[1][1] = 0.0f;
                        color[1][2] = 0.8f;
                    }
                }
                for (int i = 0; i < 2; ++i)
                {
                    FE::byte firsIndex = FE::byte(4 * i);
                    FE::byte index[] =
                    {
                        FE::byte(firsIndex),      FE::byte(firsIndex + 1),
                        FE::byte(firsIndex + 1),  FE::byte(firsIndex + 2),
                        FE::byte(firsIndex + 1),  FE::byte(firsIndex + 3),
                    };
                    context.device()->setLineWidth(1.0f);
                    device.setUniform4fv(prg._color, 1, color[i]);
                    device.drawElements(PT_LINES, IndexUint8,6,  index);
                }
            }
            prg.end(cmdBuf);


#endif // 0
        }

        real calcOffAngle(FEContext& context, const FE::int2& start, const FE::int2 &end)
        {
            FE::FECamera& camera = context.activeCamera();
            real3 vtStart;
            real3 vtEnd;
            real3 rotateAxis = _d.axis();
            calcTangentAndFan(context, rotateAxis, _radiusArc, vtStart, vtEnd);
            if (vtStart == vtEnd)
                return _offAngle;
            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPtStart;
            real3 retPtEnd;
            if (FE::calcRaySurFaceInsPt(rayStart, camera.getDir(), _d.position(), retPtStart)
                && FE::calcRaySurFaceInsPt(rayEnd, camera.getDir(), _d.position(), retPtEnd))
            {
                real3 norV = FE::normalize(retPtEnd - retPtStart);
                real3 norVt = FE::normalize(vtEnd - vtStart);
                real3 vPos = FE::closePointOnVector(norVt, retPtStart, retPtEnd);
                real off = FE::distance(retPtStart, vPos);
                off = off / camera.pixelU(_d.position());
                if (dot(norV, norVt) < 0)///计算拖动方向与切线方向是否相同
                    off = -off;
                return off;
            }
            return _offAngle;
        }

        real3 vectorPerpendicularToDir()
        {
            real3 tAxis = real3(0, 1, 0);
            if (std::abs(dot(tAxis, _d.axis())) > FLT_EPSILON)
            {
                real3 tY = real3(0, 1, 0);
                real3 tZ = real3(0, 0, 1);
                real d = dot(tY, _d.axis());
                if (1.0 - std::abs(d) > FLT_EPSILON)
                {
                    tAxis = normalize(cross(_d.axis(), tY));
                }
                else
                {
                    tAxis = normalize(cross(_d.axis(), tZ));
                }
            }
            assert(std::abs(dot(tAxis, _d.axis())) <= FLT_EPSILON);
            return tAxis;
        }
        void calcTangentAndFan(FEContext& context, real3 &rotateAxis, real &radius, real3& vtStart, real3 &vtEnd)
        {
            FE::FECamera& camera = context.activeCamera();
            Ray rayStart = camera.createRayFromScreen(_startPos.x, _startPos.y);
            real3 tangentPos;   ///切点
            real3 tangentNorV;  ///切线方向向量    
            real3 retPt;
            std::pair<bool, real> ret = rayStart.intersectSphere(_d.position(), radius);
            if (FE::calcRaySurFaceInsPt(rayStart, rotateAxis, _d.position(), retPt))
            {
                tangentPos = normalize(retPt - _d.position()) *  radius;
                tangentNorV = normalize(cross(rotateAxis, tangentPos));
            }
            else if (ret.first && ret.second >= 0)
            {//当摄像机_dir与旋转轴作为法向量的面平行时，使用平面无法计算出切点，因此采用球交点计算
             ///但球交点不一定在旋转轴上，因此计算出的切点不准确（仅仅显示不准确，对计算没有影响）
                retPt = rayStart.getPoint(ret.second);
                tangentPos = normalize(retPt - _d.position()) *  radius;
                tangentNorV = normalize(cross(rotateAxis, tangentPos));
            }
            else
            {//不会进入到这里
                return;
            }
            real        unit = camera.pixelU(_d.position());
            real        tLen = unit * 40;        //切线长度
            real        tLen1 = unit * 2;         //切线间距
            real        arSize = unit * 8;      //切线箭头
            quatr       qa = FE::angleAxis(real(40), camera.getDir());
            quatr       qb = FE::angleAxis(real(40), -camera.getDir());
            {//计算切线
             ///旋转方向
                _tangent[0] = tangentPos + tangentNorV * tLen1;
                _tangent[1] = tangentPos + tangentNorV * tLen;
                _tangent[2] = tangentPos + tangentNorV * tLen - qa * (tangentNorV * arSize);
                _tangent[3] = tangentPos + tangentNorV * tLen - qb * (tangentNorV * arSize);
                ///旋转反方向   
                _tangent[4] = tangentPos - tangentNorV * tLen1;
                _tangent[5] = tangentPos - tangentNorV * tLen;
                _tangent[6] = tangentPos - tangentNorV * tLen + qa * (tangentNorV * arSize);
                _tangent[7] = tangentPos - tangentNorV * tLen + qb * (tangentNorV * arSize);
            }
            {//计算扇形
                _fan.clear();
                _fan.push_back(real3(0, 0, 0));
                _fan.push_back(tangentPos);
                real step = 1;
                for (real i = 0; i < std::abs(_offAngle); i += step)
                {
                    quatr quat(1, 0, 0, 0);
                    if (_offAngle >= 0.0)
                    {
                        quat = FE::angleAxis(-i, rotateAxis);
                    }
                    else
                    {
                        quat = FE::angleAxis(i, rotateAxis);
                    }
                    ///0.97缩短扇形半径，防止画出球外
                    _fan.push_back(quat * tangentPos * 0.97);
                }
            }
            vtStart = tangentPos;
            vtEnd = tangentPos + tangentNorV;
            ///计算完成，可以开始绘制
            _bRotatting = true;
        }
    };

    FEEditSingleAxisRotate::FEEditSingleAxisRotate(FEContext& context)
        :FEEditAxis(EditAxisTypeSingleRotate, context)
    {
        _p = new FEEditSingleAxisRotatePrivate(*this);
    }
    FEEditSingleAxisRotate::~FEEditSingleAxisRotate()
    {
        delete _p;
    }

    FEEditSingleAxisRotate::MDelegate& FEEditSingleAxisRotate::mDelegate()
    {
        return _p->_delegate;
    }

    void FEEditSingleAxisRotate::setAxis(const real3& axis)
    {
        if (_p->_axis == axis)
            return;
        _p->_axis = normalize(axis);
        _internalFlags.addFlag(InteralFlag_Update);
        _context.requireNextFrame();
    }
    const real3& FEEditSingleAxisRotate::axis()
    {
        return _p->_axis;
    }

    void FEEditSingleAxisRotate::setForwardAngleMaxLimit(real maxAngle)
    {
        _p->_forwardAngleMaxLimit = std::abs(maxAngle);
    }
    real FEEditSingleAxisRotate::forwardAngleMaxLimit() const
    {
        return _p->_forwardAngleMaxLimit;
    }
    void FEEditSingleAxisRotate::setReverseAngleMaxLimit(real maxAngle)
    {
        _p->_reverseAngleMaxLimit = std::abs(maxAngle);
    }
    real FEEditSingleAxisRotate::reverseAngleMaxLimit() const
    {
        return _p->_reverseAngleMaxLimit;
    }

    bool FEEditSingleAxisRotate::isAxisHovered() const
    {
        return _p->_bHoveredAxis;
    }
    bool FEEditSingleAxisRotate::isAxisSelected() const
    {
        return _p->_bSelectedAxis;
    }

    void FEEditSingleAxisRotate::cancelHovered()
    {
        bool oldHovered = _p->_bHoveredAxis;
        _p->_bHoveredAxis = false;
        if (oldHovered != _p->_bHoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditSingleAxisRotate::cancelSelected()
    {
        bool oldSelected = _p->_bSelectedAxis;
        _p->_bSelectedAxis = false;
        if (oldSelected != _p->_bSelectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditSingleAxisRotate::mouseButtonPress(FEContext& context, const int2& pos)
    {
        _p->_bMouseDown = true;

        bool oldSelected = _p->_bSelectedAxis;
        _p->_bSelectedAxis = _p->_bHoveredAxis;
        if (oldSelected != _p->_bSelectedAxis)
        {
            this->sendSelectedDelegate();
        }

        if (isAxisSelected())
        {
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            _p->_delegate(EditStatus::EditStart, _p->_axis, 0.0, 0.0, *this);
            return true;
        }
        return false;
    }
    bool FEEditSingleAxisRotate::mouseButtonRelease(FEContext& context, const int2& pos)
    {
        _p->_bMouseDown = false;
        if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, _p->_axis, 0.0, _p->_offAngle, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;

            bool oldSelected = _p->_bSelectedAxis;
            _p->_bSelectedAxis = false;
            if (oldSelected != _p->_bSelectedAxis)
            {
                this->sendSelectedDelegate();
            }
            return true;
        }
        return false;
    }
    bool FEEditSingleAxisRotate::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real oldAngle = _p->_offAngle;

            real3 axis = this->axis();
            real angle = 0.0;
            _p->calcRotate(context, _p->_startPos, pos, axis, angle);
            ///最大限定角度约束
            if (angle > _p->_forwardAngleMaxLimit)
            {//正向最大限定角度
                angle = _p->_forwardAngleMaxLimit;
                _p->_offAngle = angle;
            }
            else if (angle < -_p->_reverseAngleMaxLimit)
            {//反向最大限定角度
                angle = -_p->_reverseAngleMaxLimit;
                _p->_offAngle = angle;
            }
            angle = angle - oldAngle;
            _p->_delegate(EditStatus::Editting, axis, angle, _p->_offAngle, *this);
            _p->_downPos = pos;
            _context.requireNextFrame();
            return true;
        }
        else if (!_p->_bMouseDown)
        {
            if (_p->hoverAxis(context, pos))
            {
            }
            else
            {
            }
        }
        return false;
    }

    bool FEEditSingleAxisRotate::touchDown(FEContext& context, const int2& pos)
    {
        _p->_bTouchDown = true;
        if (this->isAxisSelected())
        {
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            _p->_delegate(EditStatus::EditStart, _p->_axis, 0.0, 0.0, *this);
            return true;
        }
        else
        {
            _p->_bTouchPickup = true;
            return false;
        }
        return false;

    }
    bool FEEditSingleAxisRotate::touchUp(FEContext& context, const int2& pos)
    {
        _p->_bTouchDown = false;
        if (_p->_bTouchPickup)
        {///拾取动作
            bool oldSelected = _p->_bSelectedAxis;
            _p->_bSelectedAxis = _p->hoverAxis(context, pos);
            if (oldSelected != _p->_bSelectedAxis)
            {
                this->sendSelectedDelegate();
            }

            if (this->isAxisSelected())
            {
            }
            else
            {
            }
            _p->_bTouchPickup = false;
            return false;
        }
        else if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, _p->_axis, 0.0, _p->_offAngle, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            
            bool oldSelected = _p->_bSelectedAxis;
            _p->_bSelectedAxis = false;
            if (oldSelected != _p->_bSelectedAxis)
            {
                this->sendSelectedDelegate();
            }
            
            bool oldHovered = _p->_bHoveredAxis;
            _p->_bHoveredAxis = false;
            if (oldHovered != _p->_bHoveredAxis)
            {
                this->sendHoveredDelegate();
            }

            return true;
        }
        return false;

    }
    bool FEEditSingleAxisRotate::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real oldAngle = _p->_offAngle;

            real3 axis = this->axis();
            real angle = 0.0;
            _p->calcRotate(context, _p->_startPos, pos, axis, angle);
            ///最大限定角度约束
            if (angle > _p->_forwardAngleMaxLimit)
            {//正向最大限定角度
                angle = _p->_forwardAngleMaxLimit;
                _p->_offAngle = angle;
            }
            else if (angle < -_p->_reverseAngleMaxLimit)
            {//反向最大限定角度
                angle = -_p->_reverseAngleMaxLimit;
                _p->_offAngle = angle;
            }
            angle = angle - oldAngle;
            _p->_delegate(EditStatus::Editting, axis, angle, _p->_offAngle, *this);
            _p->_downPos = pos;
            _context.requireNextFrame();
            return true;
        }
        return false;
    }

    void FEEditSingleAxisRotate::update(FEContext& context)
    {
        _p->updateAxisVerties(context);
    }
    void FEEditSingleAxisRotate::render(FEContext& context)
    {
        if (!_internalFlags.hasFlag(InteralFlag_Visible))
            return;
        _p->renderAxis(context);
    }
}

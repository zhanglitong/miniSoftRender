#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditSingleAxisMove.h"


namespace   FE
{
#define AXIS_CONE_COLOR {0.6f, 0.0f, 0.8f, 1.0f}
#define AXIS_ARROW_COLOR {0.6f, 0.0f, 0.8f, 1.0f}
    class FEEditSingleAxisMovePrivate
    {
        static constexpr real ConeHeight = 50;
        static constexpr real ConeBottomRadiu = 10;

    private:
        struct PushBlock
        {
            mat4r     _mvp;
            float4      _color;
        };
    public:
        ///
        FEEditSingleAxisMove& _d;
        ///箭头直线
        FE::float3 _moveAxis[2];
        ///圆锥
        std::vector<float3> _axisAr;

        ///轴线VBO
        VertexBufPtr    _axisVBO    =   nullptr;
        ///绘制管线
        GraphicPLPtr    _pipeLine;
        ///材质
        MaterialPtr     _material;

        ///轴风格
        FEEditSingleAxisMove::AxisStyle _axisStyle;
        ///轴朝向
        real3 _dir;

        ///是否选中轴
        bool _bSelectedAxis;
        ///是否高亮轴
        bool _bHoveredAxis;

        ///移动偏移
        real3 _offMove;

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
        ///触屏开始时的位置
        int2 _touchStartPos;

        ///正向最大限制距离
        real _forwardDistanceMaxLimit;
        ///反向最大限制距离
        real _reverseDistanceMaxLimit;

        ///回调
        FEEditSingleAxisMove::MDelegate _delegate;
    public:
        FEEditSingleAxisMovePrivate(FEEditSingleAxisMove& d) :_d(d)
        {
            _axisStyle = FEEditSingleAxisMove::AxisStyle::AxisStyleArrow;
            _dir = real3(1.0, 0.0, 0.0);
            _offMove = real3(0);

            _bSelectedAxis = false;
            _bHoveredAxis = false;

            _bMouseDown = false;
            _downPos = int2(0);
            _startPos = int2(0);

            _bTouchDown = false;
            _bTouchPickup = false;
            _touchDownPos = int2(0);
            _touchStartPos = int2(0);

            _forwardDistanceMaxLimit = FLT_MAX;
            _reverseDistanceMaxLimit = FLT_MAX;

            /// stub: FE engine does not yet provide resource/material interfaces
            _pipeLine = nullptr;
            _material = nullptr;
        }
    public:
        ///更新轴顶点
        void updateAxisVerties(FEContext& context) 
        {
            switch (_axisStyle)
            {
            case FE::FEEditSingleAxisMove::AxisStyleArrow:
                updateArrowVerties(context);
                break;
            case FE::FEEditSingleAxisMove::AxisStyleCone:
                updateConeVerties(context);
                break;
            default:
                break;
            }
        }
        ///绘制轴
        void renderAxis(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
        }
        ///移动计算
        real3 calcMove(FEContext& context, const FE::int2& start, const FE::int2 &end) 
        {
            if (!_d.isAxisSelected())
                return _offMove;
            FE::FECamera& camera = context.activeCamera();
            real3 axis = _d.dir();
            real3 faceNor = camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offMove;
            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0 = real3(0.0);
            real3 retPt1 = real3(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) &&
                calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                real3 v = FE::closePointOnVector(axis, retPt0, retPt1);
                real3 retV = axis * length(v - retPt0);
                if (dot(normalize(v - retPt0), _d.dir()) < 0)
                {
                    retV = -retV;
                }
                _offMove = retV;
            }
            return _offMove;
        }
        ///高亮轴
        bool hoverAxis(FEContext& context, const int2& pos)
        {
            bool oldHovered = _bHoveredAxis;
            _bHoveredAxis = false;
            switch (_axisStyle)
            {
            case FE::FEEditSingleAxisMove::AxisStyleArrow:
                _bHoveredAxis = hoverAxisArrow(context, pos);
                break;
            case FE::FEEditSingleAxisMove::AxisStyleCone:
                _bHoveredAxis = hoverAxisCone(context, pos);
                break;
            default:
                _bHoveredAxis = false;
                break;
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
            FECamera& camera = context.activeCamera();
            real3       cenPos = real3(0, 0, 0);
            real        unitF = camera.pixelU(_d.position());
            real        size = unitF * 80;
            ///center
            _moveAxis[0] = real3(0, 0, 0);
            ///vec
            _moveAxis[1] = _d.dir() * size;

            ///计算箭头
            real arSize = unitF * 24;
            real arRadius = unitF * 5;
            real step = 12.0;
            ///
            _axisAr.clear();
            ///arr
            real3 tAxis = vectorPerpendicularToDir();
            _axisAr.push_back(_d.dir() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r rMat(1.0);
                rMat = FE::rotate(rMat, real(i) * step, _d.dir());
                real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
            uint arBufSize      =   (uint)_axisAr.size() * sizeof(float3);
            uint lineBufSize    =   2 * sizeof(float3);
            uint bufSize        =   arBufSize + lineBufSize;
            /// stub: FEDevice does not yet provide createVertexBuffer
            if (!_axisVBO)
            {
                _axisVBO = std::make_shared<VertexBuffer>();
            }
            _axisVBO->update(0, lineBufSize, &_moveAxis[0]);
            _axisVBO->update(lineBufSize, arBufSize, _axisAr.data());
        }
        void updateConeVerties(FEContext& context)
        {
            FECamera& camera = context.activeCamera();
            _axisAr.clear();
            real    unitF = camera.pixelU(_d.position());
            ///计算箭头
            real arSize = unitF * ConeHeight;
            real arRadius = unitF * ConeBottomRadiu;
            real step = 12.0;
            ///
            real3 tAxis = vectorPerpendicularToDir();
            ///arX
            _axisAr.push_back(_d.dir() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r rMat(1.0);
                rMat = FE::rotate(rMat, real(i) * step, _d.dir());
                real3 nor = normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }

            uint bufSize      =   (uint)_axisAr.size() * sizeof(float3);
            /// stub: FEDevice does not yet provide createVertexBuffer
            if (!_axisVBO)
            {
                _axisVBO = std::make_shared<VertexBuffer>();
            }
            _axisVBO->update(0, bufSize, _axisAr.data());
        }

        void renderAxisArrow(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
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
                float color[4] = AXIS_ARROW_COLOR;
                device.setUniform4fv(prg._color, 1, color);
                device.drawArrays(PT_POINTS, 0, (GLsizei)1);
            }
            prg.end(cmdBuf);
#endif
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
                    float color[4] = AXIS_ARROW_COLOR;
                    device.setUniform4fv(prg._color, 1, color);
                    device.drawArrays(PT_TRIANGLE_FAN, 32 * i, 32);
                }
            }
            prg.end(cmdBuf);
#endif
        } 
        void renderAxisCone(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
        }

        bool hoverAxisArrow(FEContext& context, const int2& screen) 
        {
            static const real DIST_MAX = 6;
            FE::FECamera& camera = context.activeCamera();
            real3       mouse(screen.x, screen.y, 0);
            real2       center = camera.worldToScreen(real3(_moveAxis[0]) + _d.position());
            ///轴数据
            real2       vecAxis = camera.worldToScreen(real3(_moveAxis[1]) + _d.position());

            real        dist = closeDistanceOnLine(real3(center.x, center.y, 0), real3(vecAxis.x, vecAxis.y, 0), mouse);

            if (dist <= DIST_MAX)
            {
                return true;
            }
            return false;
        }
        bool hoverAxisCone(FEContext& context, const int2& screen)
        {
            static const real DIST_MAX = ConeBottomRadiu * real(0.6);
            FECamera &camera = context.activeCamera();
            real3       mouse(screen.x, screen.y, 0);
            real2       center = camera.worldToScreen(_d.position());
            ///
            real        len = camera.pixelU(_d.position()) * ConeHeight;
            ///轴数据
            real2       axis = camera.worldToScreen(_d.position() + _d.dir() * len);

            real        dist = closeDistanceOnLine(real3(center.x, center.y, 0), real3(axis.x, axis.y, 0), mouse);
            if (dist <= DIST_MAX)
            {
                return true;
            }
            return false;
        }

        real3 vectorPerpendicularToDir()
        {
            real3 tAxis = real3(0, 1, 0);
            if (std::abs(dot(tAxis, _d.dir())) > FLT_EPSILON)
            {
                real3 tY = real3(0, 1, 0);
                real3 tZ = real3(0, 0, 1);
                real d = dot(tY, _d.dir());
                if (1.0 - std::abs(d) > FLT_EPSILON)
                {
                    tAxis = normalize(cross(_d.dir(), tY));
                }
                else
                {
                    tAxis = normalize(cross(_d.dir(), tZ));
                }
            }
            assert(std::abs(dot(tAxis, _d.dir())) <= FLT_EPSILON);
            return tAxis;
        }
    };

    FEEditSingleAxisMove::FEEditSingleAxisMove(FEContext& context)
        :FEEditAxis(EditAxisTypeSingleMove, context)
    {
        _p = new FEEditSingleAxisMovePrivate(*this);
    }
    FEEditSingleAxisMove::~FEEditSingleAxisMove()
    {
        delete _p;
    }

    FEEditSingleAxisMove::MDelegate& FEEditSingleAxisMove::mDelegate()
    {
        return _p->_delegate;
    }

    void FEEditSingleAxisMove::setAxisStyle(AxisStyle style)
    {
        _p->_axisStyle = style;
    }
    FEEditSingleAxisMove::AxisStyle FEEditSingleAxisMove::axisStyle() const
    {
        return _p->_axisStyle;
    }
    void FEEditSingleAxisMove::setDir(const real3& dir)
    {
        if (_p->_dir == dir)
            return;
        _p->_dir = normalize(dir);
        _internalFlags.addFlag(InteralFlag_Update);
        _context.requireNextFrame();
    }
    const real3& FEEditSingleAxisMove::dir()
    {
        return _p->_dir;
    }

    void FEEditSingleAxisMove::setForwardDistanceMaxLimit(real maxDis)
    {
        _p->_forwardDistanceMaxLimit = std::abs(maxDis);
    }
    real FEEditSingleAxisMove::forwardDistanceMaxLimit() const
    {
        return _p->_forwardDistanceMaxLimit;
    }
    void FEEditSingleAxisMove::setReverseDistanceMaxLimit(real maxDis)
    {
        _p->_reverseDistanceMaxLimit = std::abs(maxDis);
    }
    real FEEditSingleAxisMove::reverseDistanceMaxLimit() const
    {
        return _p->_reverseDistanceMaxLimit;
    }

    bool FEEditSingleAxisMove::isAxisHovered() const
    {
        return _p->_bHoveredAxis;
    }
    bool FEEditSingleAxisMove::isAxisSelected() const
    {
        return _p->_bSelectedAxis;
    }
    
    void FEEditSingleAxisMove::cancelHovered()
    {
        bool oldHovered = _p->_bHoveredAxis;
        _p->_bHoveredAxis = false;
        if (oldHovered != _p->_bHoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditSingleAxisMove::cancelSelected()
    {
        bool oldSelected = _p->_bSelectedAxis;
        _p->_bSelectedAxis = false;
        if (oldSelected != _p->_bSelectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditSingleAxisMove::mouseButtonPress(FEContext& context, const int2& pos)
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
            _p->_offMove = real3(0);
            _p->_delegate(EditStatus::EditStart, real3(0.0), real3(0.0), *this);
            return true;
        }
        return false;
    }
    bool FEEditSingleAxisMove::mouseButtonRelease(FEContext& context, const int2& pos)
    {
        _p->_bMouseDown = false;
        if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, real3(0.0), _p->_offMove, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offMove = real3(0);

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
    bool FEEditSingleAxisMove::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_startPos, pos);
            ///最大限定距离约束
            real d = dot(offset, this->dir());
            if (d > _p->_forwardDistanceMaxLimit)
            {//正向最大限定距离
                offset = this->dir() * _p->_forwardDistanceMaxLimit;
                _p->_offMove = offset;
            }
            else if (d < -_p->_reverseDistanceMaxLimit)
            {//反向最大限定距离
                offset = this->dir() * (-_p->_reverseDistanceMaxLimit);
                _p->_offMove = offset;
            }

            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
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

    bool FEEditSingleAxisMove::touchDown(FEContext& context, const int2& pos)
    {
        _p->_bTouchDown = true;
        if (this->isAxisSelected())
        {
            _p->_bTouchPickup = false;
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offMove = real3(0);
            _p->_delegate(EditStatus::EditStart, real3(0.0), real3(0.0), *this);
            return true; 
        }
        else 
        {
            _p->_bTouchPickup = true;
            return false;
        }
        return false;
    }
    bool FEEditSingleAxisMove::touchUp(FEContext& context, const int2& pos)
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
        else if(isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, real3(0.0), _p->_offMove, *this);
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offMove = real3(0);

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

            _p->_bTouchPickup = false;
            return true;
        }
        return false;
    }
    bool FEEditSingleAxisMove::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_touchStartPos, pos);
            ///最大限定距离约束
            real d = dot(offset, this->dir());
            if (d > _p->_forwardDistanceMaxLimit)
            {//正向最大限定距离
                offset = this->dir() * _p->_forwardDistanceMaxLimit;
                _p->_offMove = offset;
            }
            else if (d < -_p->_reverseDistanceMaxLimit)
            {//反向最大限定距离
                offset = this->dir() * (-_p->_reverseDistanceMaxLimit);
                _p->_offMove = offset;
            }
            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
            _p->_touchDownPos = pos;
            _context.requireNextFrame();
            return true;
        }
        return false;
    }

    void FEEditSingleAxisMove::update(FEContext& context)
    {
        _p->updateAxisVerties(context);
    }
    void FEEditSingleAxisMove::render(FEContext& context)
    {
        if (!_internalFlags.hasFlag(InteralFlag_Visible))
            return;
        _p->renderAxis(context);
    }
}

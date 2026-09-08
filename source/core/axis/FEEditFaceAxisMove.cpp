#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditFaceAxisMove.h"

namespace   FE
{
#define AXIS_CONE_COLOR {0.6f, 0.0f, 0.8f, 0.4f}
#define AXIS_ARROW_COLOR {0.6f, 0.0f, 0.8f, 1.0f}
    class FEEditFaceAxisMovePrivate
    {
        static constexpr real ConeHeight = 50;
        static constexpr real ConeBottomRadiu = 10;
    public:
        ///
        FEEditFaceAxisMove& _d;
        ///矩形顶点
        std::array<real3, 4> _rectPoints;
        
        ///轴朝向
        real3 _dir;
        ///绘制顶点，相对于中心点 _d.position();
        std::vector<V3N3> _vertices;

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
        FEEditFaceAxisMove::MDelegate _delegate;
    public:
        FEEditFaceAxisMovePrivate(FEEditFaceAxisMove& d) :_d(d)
        {
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
        }
    public:
        ///更新轴顶点
        void updateAxisVerties(FEContext& context) 
        {
            _vertices.clear();
            const real3& pt0 = _rectPoints[0];
            const real3& pt1 = _rectPoints[1];
            const real3& pt2 = _rectPoints[2];
            const real3& pt3 = _rectPoints[3];

            ///计算中心点
            real3 center = (pt0 + pt1 + pt2 + pt3) * 0.25;
            _d.setPosition(center);

            ///计算方向
            real3 v0 = pt2 - pt0;
            real3 v1 = pt1 - pt0;
            if (length(v0) >= FLT_EPSILON && length(v1) >= FLT_EPSILON)
                _dir = normalize(cross(v0, v1));
            else
                return;

            std::array<float3, 4> tV;
            for (size_t i = 0; i < tV.size(); ++i)
            {
                tV[i] = float3(_rectPoints[i] - _d.position());
            }
            float3 tN = float3(-_dir);
            
            ///计算顶点
            _vertices = 
            {
                /// 0,1,2
                {tV[0].x, tV[0].y, tV[0].z, tN.x, tN.y, tN.z},
                
                {tV[1].x, tV[1].y, tV[1].z, tN.x, tN.y, tN.z},
                
                {tV[2].x, tV[2].y, tV[2].z, tN.x, tN.y, tN.z},
                
                /// 0,2,3
                {tV[0].x, tV[0].y, tV[0].z, tN.x, tN.y, tN.z},
                
                {tV[2].x, tV[2].y, tV[2].z, tN.x, tN.y, tN.z},
                
                {tV[3].x, tV[3].y, tV[3].z, tN.x, tN.y, tN.z},
            };
        }
        ///绘制轴 
        void renderAxis(FEContext& context)
        {
            if (_vertices.empty())
                return;

#if 0
            auto&       prg     =   context._resource->_PROGRAM_P3_N3_UC4_NO_CLIP;
            auto&       device  =   context.device();
            auto&       cmdBuf  =   device.cmdBuffer();
            
            ///float lineWidth = device.getLineWidth();

           ///  bool bBlend = device._enabledCapabilities[GL_BLEND];
           ///  bool bCullFace = device._enabledCapabilities[GL_CULL_FACE];
           ///  bool bDepthTest = device._enabledCapabilities[GL_DEPTH_TEST];
           ///  device.enableRenderState(GL_BLEND);
           ///  device.disableRenderState(GL_CULL_FACE);
           ///  device.disableRenderState(GL_DEPTH_TEST);




            prg.begin(cmdBuf);
            {
                mat4r wholeTransform =   makeTransform(_d.position(), real3(1.0), quatr(1.0, 0.0, 0.0, 0.0));
                float4  globalRot       =   float4(0.0f, 0.0f, 0.0f, 1.0f);
                
                device.setUniformMatrix4fv(prg._worldTransform, 1, false, wholeTransform.data());
                device.setUniform4fv(prg._worldQuat, 1, &globalRot[0]);
                
                float outSideColor[4]   = { 1.0f,0.0f,0.0f,0.2f };
                float inSideColor[4]    = { 1.0f,0.0f,0.0f,0.2f };
                float sideColor[4]      = { 1.0f,0.0f,0.0f,0.2f };
                
                if (_bHoveredAxis)
                {
                    outSideColor[0] =   1.0f;
                    outSideColor[1] =   1.0f;
                    outSideColor[2] =   0.0f;

                    inSideColor[0]  =   1.0f;
                    inSideColor[1]  =   1.0f;
                    inSideColor[2]  =   0.0f;

                    sideColor[0]    =   1.0f;
                    sideColor[1]    =   1.0f;
                    sideColor[2]    =   0.0f;
                }

                device.setUniform4fv(prg._outsideColor, 1, outSideColor);
                device.setUniform4fv(prg._insideColor,  1, inSideColor);
                device.setUniform4fv(prg._sideColor,    1, sideColor);
                
                ///position
                device.attributePointer(prg._position, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3)
                    , &_vertices[0].x);
                ///normal
                device.attributePointer(prg._normal, 3, GL_FLOAT, GL_FALSE, sizeof(V3N3)
                    , &_vertices[0].nx);
                ///mode
                int mode = 0x01;
                device.setUniform1i(prg._mode, mode);

                device.drawArrays(PT_TRIANGLES, 0, (GLsizei)_vertices.size());
            }
            prg.end(cmdBuf);

            /// if (bBlend)
            ///     device.enableRenderState(GL_BLEND);
            /// else
            ///     device.disableRenderState(GL_BLEND);
            /// 
            /// if (bCullFace)
            ///     device.enableRenderState(GL_CULL_FACE);
            /// else
            ///     device.disableRenderState(GL_CULL_FACE);
            /// 
            /// if (bDepthTest)
            ///     device.enableRenderState(GL_DEPTH_TEST);
            /// else
            ///     device.disableRenderState(GL_DEPTH_TEST);

            device.bindVertexBuffer(0);
            device.bindIndexBuffer(0);

            device.setLineWidth(lineWidth);
#endif // 0
        }
        ///移动计算
        real3 calcMove(FEContext& context, const FE::int2& start, const FE::int2 &end) 
        {
            if (!_d.isAxisSelected())
                return _offMove;
            FE::FECamera& camera = context.activeCamera();
            real3 axis = _dir;
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
                if (dot(normalize(v - retPt0), _dir) < 0)
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
            if (_vertices.empty())
            {
                if (oldHovered != _bHoveredAxis)
                {
                    _d.sendHoveredDelegate();
                }
                return _bHoveredAxis;
            }
            const real3& pt0 = _rectPoints[0];
            const real3& pt1 = _rectPoints[1];
            const real3& pt2 = _rectPoints[2];
            const real3& pt3 = _rectPoints[3];

            std::array<real3, 3> tri[] = { {pt0, pt1, pt2},  {pt0, pt2, pt3} };

            FECamera& camera = context.activeCamera();

            Ray ray = camera.createRayFromScreen(pos.x, pos.y);

            real t, u, v;
            for (size_t i = 0; i < tri->size(); ++i)
            {
                if (intersectTriangle(ray.getOrigin(), ray.getDirection()
                , tri[i][0], tri[i][1], tri[i][2]
                , &t, &u, &v))
                {
                    _bHoveredAxis = true;
                    break;
                }
            }

            if (oldHovered != _bHoveredAxis)
            {
                _d.sendHoveredDelegate();
            }
            return _bHoveredAxis;
        }
    private:
        real3 vectorPerpendicularToDir()
        {
            real3 tAxis = real3(0, 1, 0);
            if (std::abs(dot(tAxis, _dir)) > FLT_EPSILON)
            {
                real3 tY = real3(0, 1, 0);
                real3 tZ = real3(0, 0, 1);
                real d = dot(tY, _dir);
                if (1.0 - std::abs(d) > FLT_EPSILON)
                {
                    tAxis = normalize(cross(_dir, tY));
                }
                else
                {
                    tAxis = normalize(cross(_dir, tZ));
                }
            }
            assert(std::abs(dot(tAxis, _dir)) <= FLT_EPSILON);
            return tAxis;
        }
    };

    FEEditFaceAxisMove::FEEditFaceAxisMove(FEContext& context)
        :FEEditAxis(EditAxisTypeFaceMove, context)
    {
        _p = new FEEditFaceAxisMovePrivate(*this);
    }
    FEEditFaceAxisMove::~FEEditFaceAxisMove()
    {
        delete _p;
    }

    FEEditFaceAxisMove::MDelegate& FEEditFaceAxisMove::mDelegate()
    {
        return _p->_delegate;
    }

    void FEEditFaceAxisMove::setRectPoints(const std::array<real3, 4>& pts)
    {
        if (_p->_rectPoints == pts)
            return;
        _p->_rectPoints = pts;
        _internalFlags.addFlag(InteralFlag_Update);
        _ctx.requireNextFrame();
    }
    const std::array<real3, 4>& FEEditFaceAxisMove::rectPoints() const
    {
        return _p->_rectPoints;
    }

    bool FEEditFaceAxisMove::isAxisHovered() const
    {
        return _p->_bHoveredAxis;
    }
    bool FEEditFaceAxisMove::isAxisSelected() const
    {
        return _p->_bSelectedAxis;
    }

    void FEEditFaceAxisMove::cancelHovered()
    {
        bool oldHovered = _p->_bHoveredAxis;
        _p->_bHoveredAxis = false;
        if (oldHovered != _p->_bHoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditFaceAxisMove::cancelSelected()
    {
        bool oldSelected = _p->_bSelectedAxis;
        _p->_bSelectedAxis = false;
        if (oldSelected != _p->_bSelectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditFaceAxisMove::mouseButtonPress(FEContext& context, const int2& pos)
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
    bool FEEditFaceAxisMove::mouseButtonRelease(FEContext& context, const int2& pos)
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
    bool FEEditFaceAxisMove::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_startPos, pos);
            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
            _p->_downPos = pos;
            _ctx.requireNextFrame();
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

    bool FEEditFaceAxisMove::touchDown(FEContext& context, const int2& pos)
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
    bool FEEditFaceAxisMove::touchUp(FEContext& context, const int2& pos)
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
            _p->_bTouchPickup = false;		

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
    bool FEEditFaceAxisMove::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_touchStartPos, pos);

            ////最大限定距离约束
            ///real d = dot(offset, this->dir());
            ///if (d > _p->_forwardDistanceMaxLimit)
            ///{//正向最大限定距离
            ///    offset = this->dir() * _p->_forwardDistanceMaxLimit;
            ///    _p->_offMove = offset;
            ///}
            ///else if (d < -_p->_reverseDistanceMaxLimit)
            ///{//反向最大限定距离
            ///    offset = this->dir() * (-_p->_reverseDistanceMaxLimit);
            ///    _p->_offMove = offset;
            ///}

            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
            _p->_touchDownPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        return false;
    }
}

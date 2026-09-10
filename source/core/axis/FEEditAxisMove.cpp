#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEEditAxisMove.h"


namespace   FE
{
    ///计算射线是否与某个四边形面有交点
    bool GetRayRectaceInsPt(const Ray &ray, FE::real3 rect[4], FE::real3 &retPt)
    {
        real t, u, v;
        if (Ray::intersectTriangle(ray.getOrigin(), ray.getDirection(), rect[0], rect[1], rect[2], &t, &u, &v))
        {
            real3 tmpPt = ray.getPoint(t);
            if (FE::pointinTriangle(rect[0], rect[1], rect[2], tmpPt))
            {
                retPt = tmpPt;
                return true;
            }
        }
        real t1, u1, v1;
        if (Ray::intersectTriangle(ray.getOrigin(), ray.getDirection(), rect[2], rect[3], rect[0], &t1, &u1, &v1))
        {
            real3 tmpPt = ray.getPoint(t1);
            if (FE::pointinTriangle(rect[2], rect[3], rect[0], tmpPt))
            {
                retPt = tmpPt;
                return true;
            }
        }
        return false;
    }

    FEEditAxisMove::FEEditAxisMove(FEContext& ctx)
            : FEEditAxis(EditAxisTypeMove, ctx)
    {
        _downPosWorld       =   real3(0.0);

        _bMouseDown         =   false;
        _downPos            =   int2(0);
        _startPos           =   int2(0);

        _bTouchDown         =   false;
        _bTouchPickup       =   false;
        _touchDownPos       =   int2(0);
        _touchStartPos      =   int2(0);

        _offMove            =   real3(0.0);
        _hoveredAxis        =   FEEditAxisMove::AXIS::AXIS_NULL;
        _selectedAxis       =   FEEditAxisMove::AXIS::AXIS_NULL;
    }

    FEEditAxisMove::FEEditAxisMove(const FEEditAxisMove& other)
        : FEEditAxis(EditAxisTypeMove, other._ctx)
    {
        _downPosWorld       =   real3(0.0);

        _bMouseDown         =   false;
        _downPos            =   int2(0);
        _startPos           =   int2(0);

        _bTouchDown         =   false;
        _bTouchPickup       =   false;
        _touchDownPos       =   int2(0);
        _touchStartPos      =   int2(0);

        _offMove            =   real3(0.0);
        _hoveredAxis        =   FEEditAxisMove::AXIS::AXIS_NULL;
        _selectedAxis       =   FEEditAxisMove::AXIS::AXIS_NULL;
    }

    FEEditAxisMove::~FEEditAxisMove()
    {
    }

    FEEditAxisMove::MDelegate& FEEditAxisMove::mDelegate()
    {
        return _delegate;
    }

    FEEditAxisMove::AXIS FEEditAxisMove::hoveredAxis() const
    {
        return _hoveredAxis;
    }
    FEEditAxisMove::AXIS FEEditAxisMove::selectedAxis() const
    {
        return _selectedAxis;
    }

    /// <summary>
    /// 更新轴顶点
    /// </summary>
    /// <param name="camera"></param>
    /// <param name="axisLine"></param>
    /// <param name="indexs"></param>
    /// <param name="axisArray"></param>
    void    FEEditAxisMove::update(FECamera& camera)
    {
        uint        pixels      =   80;
        uint        planePix    =   40;

        real3       cenPos  =   real3(0, 0, 0);
        real        unitF   =   camera.pixelU(position());
        real        size    =   unitF * pixels;
        real        planeSz =   unitF * planePix;
        _moveAxis.clear();
        _moveAxis.resize(13);
        ///center
        _moveAxis[0]    =   real3(0, 0, 0);
        ///X
        _moveAxis[1]    =   axisX() * size;
        ///y
        _moveAxis[2]    =   axisY() * size;
        ///z
        _moveAxis[3]    =   axisZ() * size;
        ///xoy
        _moveAxis[4]    =   axisX() * planeSz;
        _moveAxis[6]    =   axisY() * planeSz;
        _moveAxis[5]    =   _moveAxis[4] + _moveAxis[6];
        ///float3 test = (axisX() + axisY())*planeSz;
        ///yoz
        _moveAxis[7]    =   axisY() * planeSz;
        _moveAxis[9]    =   axisZ() * planeSz;
        _moveAxis[8]    =   _moveAxis[7] + _moveAxis[9];
        ///zox
        _moveAxis[10]   =   axisZ() * planeSz;
        _moveAxis[12]   =   axisX() * planeSz;
        _moveAxis[11]   =   _moveAxis[10] + _moveAxis[12];

        ///计算箭头
        real arSize     =   unitF * 24;
        real arRadius   =   unitF * 5;
        _axisAr.clear();
        real step       =   12.0;
        ///arX
        {
            real3 tAxis = vectorPerpendicularToAxisX();
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
            real3 tAxis = vectorPerpendicularToAxisY();
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
            real3 tAxis = vectorPerpendicularToAxisZ();
            _axisAr.push_back(axisZ() * arSize);
            for (int i = 0; i <= 30; ++i)
            {
                mat4r   rMat    =   FE::rotate(mat4r(1), real(i) * step, axisZ());
                real3   nor     =   normalize(real3(rMat * real4(tAxis, 0.0)));
                _axisAr.push_back(nor * arRadius);
            }
        }

        if (_indexs.empty())
        {
            for (uint16 i = 0; i < 3; i++)
            {
                uint16 firstIndex0 = 4 + 3 * i;//4:面的第一个索引点
                uint16 firstIndex1 = 5 + (3 * ((i + 2) % 3));///5:面的第二个索引点
                //与轴线相邻面框线索引
                _indexs.push_back(firstIndex0);
                _indexs.push_back(firstIndex0 + 1);
                _indexs.push_back(firstIndex1);
                _indexs.push_back(firstIndex1 + 1);
                //轴线索引3
                _indexs.push_back(0);
                _indexs.push_back(i + 1);
                //面索引
                _indexs.push_back(0);
                _indexs.push_back(firstIndex0);
                _indexs.push_back(firstIndex0 + 1);
                _indexs.push_back(firstIndex0 + 2);
            }
        }
    }

    const   float3s&    FEEditAxisMove::moveAxis() const
    {
        return  _moveAxis;
    }
    ///箭头顶点
    const   float3s&    FEEditAxisMove::axisArray() const
    {
        return  _axisAr;
    }
    /// <summary>
    /// 索引数据
    /// </summary>
    const   uint16s&    FEEditAxisMove::indexs() const
    {
        return  _indexs;
    }

    void    FEEditAxisMove::cancelHovered()
    {
        AXIS oldHovered     =   _hoveredAxis;
        _hoveredAxis        =   FEEditAxisMove::AXIS::AXIS_NULL;
        if (oldHovered != _hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void    FEEditAxisMove::cancelSelected()
    {
        AXIS oldSelected    =   _selectedAxis;
        _selectedAxis       =   FEEditAxisMove::AXIS::AXIS_NULL;
        if (oldSelected != _selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    //高亮轴
    FEEditAxisMove::AXIS FEEditAxisMove::hoverAxis(FEContext& context, const int2& pos)
    {
        FEEditAxisMove::AXIS oldHovered = _hoveredAxis;

        _hoveredAxis = FEEditAxisMove::AXIS::AXIS_NULL;

        _hoveredAxis = pickAxis(context, pos);

        if (_hoveredAxis == FEEditAxisMove::AXIS::AXIS_NULL)
        {
            _hoveredAxis = pickFace(context, pos);
        }

        if (oldHovered != _hoveredAxis)
        {
            sendHoveredDelegate();
        }
        return _hoveredAxis;
    }

    /// <summary>
    /// 拾取高亮轴
    /// </summary>
    FEEditAxisMove::AXIS FEEditAxisMove::pickAxis(FEContext& context, const int2& screen)
    {
        if(_moveAxis.empty())
            return FEEditAxisMove::AXIS_NULL;

        static const real DIST_MAX = 6;
        auto&       camera = context.activeCamera();
        real3       mouse(screen.x, screen.y, 0);
        real2       center = camera.worldToScreen(real3(_moveAxis[0]) + position());
        //轴数据
        real2       xAxis = camera.worldToScreen((real3(_moveAxis[1]) + position()));
        real2       yAxis = camera.worldToScreen((real3(_moveAxis[2]) + position()));
        real2       zAxis = camera.worldToScreen((real3(_moveAxis[3]) + position()));

        /// 拾取坐标轴
        real        dist[3] = { 0,0,0 };
        dist[0] = closeDistanceOnLine(real3(center.x, center.y, 0), real3(xAxis.x, xAxis.y, 0), mouse);
        dist[1] = closeDistanceOnLine(real3(center.x, center.y, 0), real3(yAxis.x, yAxis.y, 0), mouse);
        dist[2] = closeDistanceOnLine(real3(center.x, center.y, 0), real3(zAxis.x, zAxis.y, 0), mouse);
        if (dist[0] == 0 && dist[1] == 0 && dist[2] == 0)
            return FEEditAxisMove::AXIS_NULL;
        int min = 0;
        if (dist[1] < dist[min])
            min = 1;
        if (dist[2] < dist[min])
            min = 2;
        if (dist[min] > DIST_MAX)
            return FEEditAxisMove::AXIS_NULL;
        if (min == 0 && enabled(FEEditAxisMove::AXIS_X))
            return FEEditAxisMove::AXIS_X;
        if (min == 1 && enabled(FEEditAxisMove::AXIS_Y))
            return FEEditAxisMove::AXIS_Y;
        if (min == 2 && enabled(FEEditAxisMove::AXIS_Z))
            return  FEEditAxisMove::AXIS_Z;
        return  FEEditAxisMove::AXIS_NULL;
    }
    FEEditAxisMove::AXIS FEEditAxisMove::pickFace(FEContext& context, const int2& screen)
    {
        if(_moveAxis.empty())
            return FEEditAxisMove::AXIS_NULL;

        auto&   camera  =   context.activeCamera();
        auto    ray     =   camera.createRayFromScreen(screen.x, screen.y);
        real3      rect[3][4] =
        {
            /// xoy
            {
                  (real3(_moveAxis[0]) + position())
                , (real3(_moveAxis[4]) + position())
                , (real3(_moveAxis[5]) + position())
                , (real3(_moveAxis[6]) + position())
            } ,
            /// yoz
            {
                  (real3(_moveAxis[0]) + position())
                , (real3(_moveAxis[7]) + position())
                , (real3(_moveAxis[8]) + position())
                , (real3(_moveAxis[9]) + position())
            },
            /// xoz
            {
                  (real3(_moveAxis[0]) + position())
                , (real3(_moveAxis[10])+ position())
                , (real3(_moveAxis[11])+ position())
                , (real3(_moveAxis[12])+ position())
            }
        };
        int index = -1;
        for (int i = 0; i < 3; ++i)
        {
            real3 retPt;
            if (GetRayRectaceInsPt(ray, rect[i], retPt))
            {
                index = i;
                break;
            }
        }
        switch (index)
        {
        case 0:
        {
            if(enabled(FEEditAxisMove::AXIS_X) || enabled(FEEditAxisMove::AXIS_Y))
                return FEEditAxisMove::AXIS_XY;
            else
                return FEEditAxisMove::AXIS_NULL;
        }
        case 1:
        {
            if(enabled(FEEditAxisMove::AXIS_Y) || enabled(FEEditAxisMove::AXIS_Z))
                return FEEditAxisMove::AXIS_YZ;
            else
                return FEEditAxisMove::AXIS_NULL;
        }
        case 2:
        {
            if(enabled(FEEditAxisMove::AXIS_X) || enabled(FEEditAxisMove::AXIS_Z))
                return FEEditAxisMove::AXIS_XZ;
            else
                return FEEditAxisMove::AXIS_NULL;
        }
        default:
            return FEEditAxisMove::AXIS_NULL;
        }
    }

    real3   FEEditAxisMove::moveX(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera  =   context.activeCamera();
        real3   axis    =   normalize(axisX());
        real3   faceNor =   camera.getDir();

        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offMove;

        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0) &&
            calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            real3 v     =   FE::closePointOnVector(axis, retPt0, retPt1);
            real3 retV  =   axis * length(v - retPt0);
            if (dot(normalize(v - retPt0), axis) < 0)
            {
                retV = -retV;
            }
            return retV;
        }
        return _offMove;
    }
    real3   FEEditAxisMove::moveY(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera  =   context.activeCamera();
        real3   axis    =   normalize(axisY());
        real3   faceNor =   camera.getDir();
        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offMove;

        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0) &&
            calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            real3   v     =     FE::closePointOnVector(axis, retPt0, retPt1);
            real3   retV  =     axis * length(v - retPt0);
            if (dot(normalize(v - retPt0), axis) < 0)
            {
                retV = -retV;
            }
            return retV;
        }
        return _offMove;
    }
    real3   FEEditAxisMove::moveZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera  =   context.activeCamera();
        real3   axis    =   normalize(axisZ());
        real3   faceNor =   camera.getDir();

        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offMove;

        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0) &&
            calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            real3 v     =   FE::closePointOnVector(axis, retPt0, retPt1);
            real3 retV  =   axis * length(v - retPt0);
            if (dot(normalize(v - retPt0), axis) < 0)
            {
                retV = -retV;
            }
            return retV;
        }
        return _offMove;
    }
    real3   FEEditAxisMove::moveXY(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera      =   context.activeCamera();
        real3   faceNor     =   normalize(cross(axisX(), axisY()));
        
        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0)
            && calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            return retPt1 - retPt0;
        }
        return _offMove;
    }
    real3   FEEditAxisMove::moveXZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera      =   context.activeCamera();
        real3   faceNor     =   normalize(cross(axisX(), axisZ()));

        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0)
            && calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            return retPt1 - retPt0;
        }
        return _offMove;
    }
    real3   FEEditAxisMove::moveYZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
    {
        auto&   camera      =   context.activeCamera();
        real3   faceNor     =   normalize(cross(axisY(), axisZ()));


        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPt0(0.0);
        real3   retPt1(0.0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0)
            && calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            return retPt1 - retPt0;
        }
        return _offMove;
    }

    /// <summary>
    /// 根据选中的轴计算移动偏移量
    /// </summary>
    real3 FEEditAxisMove::calcMove(FEContext& context, const int2& start, const int2& end)
    {
        real3 ret = real3(0.0);
        switch (_selectedAxis)
        {
        case FEEditAxisMove::AXIS_X:
            ret = moveX(context, start, end);
            break;
        case FEEditAxisMove::AXIS_Y:
            ret = moveY(context, start, end);
            break;
        case FEEditAxisMove::AXIS_Z:
            ret = moveZ(context, start, end);
            break;
        case FEEditAxisMove::AXIS_XY:
            ret = moveXY(context, start, end);
            break;
        case FEEditAxisMove::AXIS_YZ:
            ret = moveYZ(context, start, end);
            break;
        case FEEditAxisMove::AXIS_XZ:
            ret = moveXZ(context, start, end);
            break;
        default:
            break;
        }
        _offMove = ret;
        return ret;
    }

    void    FEEditAxisMove::onMessage(const FEMessage& inputMsg)
    {
        /// 说明已经被其他组件捕获了
        /// 不处理消息
        auto    capture =   captureObject();
        if (capture != nullptr && capture.get() != this)
            return;
        switch(inputMsg.msgId())
        {
        case MSG_LBUTTON_DOWN:
            mouseButtonPress(static_cast<const MsgLButtonDown&>(inputMsg)._info._mouse);
            break;
        case MSG_LBUTTON_UP:
            mouseButtonRelease(static_cast<const MsgLButtonUp&>(inputMsg)._info._mouse);
            break;
        case MSG_MOUSE_MOVE:
            mouseMove(static_cast<const MsgMouseMove&>(inputMsg)._info._mouse);
            break;
        }
    }
    bool    FEEditAxisMove::mouseButtonPress(const int2& pos)
    {
        _downPosWorld   =   this->position();
        _bMouseDown     =   true;
        AXIS oldSelected    =   _selectedAxis;
        _selectedAxis   =   _hoveredAxis;
        if (oldSelected != _selectedAxis)
        {
            this->sendSelectedDelegate();
            _ctx.requireNextFrame();
        }

        if (isAxisSelected())
        {
            _downPos    =   pos;
            _startPos   =   pos;
            _offMove    =   real3(0.0);
            _delegate(EditStatus::EditStart, real3(0.0), real3(0.0), *this);
            _ctx.requireNextFrame();
            /// 捕获消息系统,禁止继续分发
            setCapture();
            return true;
        }
        return false;
    }
    bool    FEEditAxisMove::mouseButtonRelease(const int2& pos)
    {
        _downPosWorld   =   this->position();
        _bMouseDown     =   false;
        if (isAxisSelected())
        {
            _delegate(EditStatus::EditEnd, real3(0.0), _offMove, *this);
            _downPos    =   pos;
            _startPos   =   pos;
            _offMove    =   real3(0);

            AXIS oldSelected    =   _selectedAxis;
            _selectedAxis   =   AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
                _ctx.requireNextFrame();
            }
            return true;
        }
        return false;
    }
    bool    FEEditAxisMove::mouseMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            real3   oldOff  =   _offMove;
            real3   offset  =   calcMove(_ctx, _startPos, pos);
                    offset  =   offset - oldOff;
            _delegate(EditStatus::Editting, offset, _offMove, *this);
            _downPos    =   pos;
            _ctx.requireNextFrame();
            
            return true;
        }
        else if(!_bMouseDown)
        {
            /// 捕获消息系统,禁止继续分发
            if (hoverAxis(_ctx, pos) != AXIS_NULL)
                setCapture();
            else
                releaseCapture();
        }
        return false;
    }

    bool    FEEditAxisMove::touchDown(const int2& pos)
    {
        {
            ///拾取动作
            AXIS oldSelected    =   _selectedAxis;
            _selectedAxis   =   hoverAxis(_ctx, pos);
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }
        }
        _downPosWorld = this->position();

        _bTouchDown = true;
        if (this->isAxisSelected())
        {
            _bTouchPickup   =   false;
            _touchDownPos   =   pos;
            _touchStartPos  =   pos;
            _offMove        =   real3(0);
            _delegate(EditStatus::EditStart, real3(0.0), real3(0.0), *this);
            return true;
        }
        else
        {
            _bTouchPickup   =   true;
            return false;
        }
    }
    bool    FEEditAxisMove::touchUp(const int2& pos)
    {
        _downPosWorld   =   this->position();
        _bTouchDown     =   false;
        if (_bTouchPickup)
        {
            ///拾取动作
            AXIS oldSelected    =   _selectedAxis;
            _selectedAxis   =   hoverAxis(_ctx, pos);
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            if (this->isAxisSelected())
            {

            }
            _bTouchPickup = false;
            return false;
        }
        else if (isAxisSelected())
        {
            _delegate(EditStatus::EditEnd, real3(0.0), _offMove, *this);
            _touchDownPos   =   pos;
            _touchStartPos  =   pos;
            _offMove        =   real3(0);
            _bTouchPickup   =   false;

            AXIS oldSelected    =   _selectedAxis;
            _selectedAxis   =   AXIS::AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            AXIS oldHovered     =   _hoveredAxis;
            _hoveredAxis    =   AXIS::AXIS_NULL;
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return true;
        }
        return false;

    }
    bool    FEEditAxisMove::touchMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            real3   oldOff  =   _offMove;
            real3   offset  =   calcMove(_ctx, _touchStartPos, pos);
                    offset  =   offset - oldOff;
            _delegate(EditStatus::Editting, offset, _offMove, *this);
            _touchDownPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        return false;
    }

}

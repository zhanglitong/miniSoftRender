#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEEditAxisScale.h"

namespace   FE
{
    ///计算射线是否与某个四边形面有交点
    extern bool GetRayRectaceInsPt(const Ray &ray, FE::real3 rect[4], FE::real3 &retPt);

    FEEditAxisScale::FEEditAxisScale(FEContext& context)
        :FEEditAxis(EditAxisType::EditAxisTypeScale, context)
    {
        _selectedAxis    =   FEEditAxisScale::AXIS_NULL;
        _hoveredAxis     =   FEEditAxisScale::AXIS_NULL;
        _bScaleing       =   false;
        _offScale        =   real3(1.0, 1.0, 1.0);
        _dir             =   real3(1.0, 1.0, 1.0);
        _axisSize        =   0.0;
        _bMouseDown      =   false;
        _bTouchDown      =   false;
        _bTouchPickup    =   false;
    }
    FEEditAxisScale::FEEditAxisScale(const FEEditAxisScale& other)
        :FEEditAxis(EditAxisType::EditAxisTypeScale, other._ctx)
    {
        _selectedAxis    =   other._selectedAxis;
        _hoveredAxis     =   other._hoveredAxis;
        _bScaleing       =   other._bScaleing;
        _offScale        =   other._offScale;
        _dir             =   other._dir;
        _axisSize        =   other._axisSize;
        _bMouseDown      =   other._bMouseDown;
        _bTouchDown      =   other._bTouchDown;
        _bTouchPickup    =   other._bTouchPickup;
    }
    FEEditAxisScale::~FEEditAxisScale()
    {
    }

    FEEditAxisScale::MDelegate& FEEditAxisScale::mDelegate()
    {
        return  _delegate;
    }

    FEEditAxisScale::AXIS FEEditAxisScale::hoveredAxis() const
    {
        return  _hoveredAxis;
    }
    FEEditAxisScale::AXIS FEEditAxisScale::selectedAxis() const
    {
        return  _selectedAxis;
    }

    void FEEditAxisScale::cancelHovered()
    {
        AXIS oldHovered = _hoveredAxis;
        _hoveredAxis = FEEditAxisScale::AXIS::AXIS_NULL;
        if (oldHovered != _hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditAxisScale::cancelSelected()
    {
        AXIS oldSelected = _selectedAxis;
        _selectedAxis = FEEditAxisScale::AXIS::AXIS_NULL;
        if (oldSelected != _selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    /// <summary>
    /// 更新缩放轴顶点
    /// </summary>
    void    FEEditAxisScale::update(FECamera& camera)
    {
        ///计算轴朝向
        _dir = calcAxisDir(camera);
        ///计算顶点
        real unitF = camera.pixelU(position());
        real size = (real)(unitF * 80);
        real triMinSize = size * 0.5;
        real triMaxSize = size * 3.0  * 0.25;
        _axisSize = size;

        _scaleAxis[0] = float3(0);

        real3 axisScale = _offScale;

        _scaleAxis[1] = _dir.x *  (axisX() * triMinSize) * axisScale.x;      //min x
        _scaleAxis[3] = _dir.x *  (axisX() * triMaxSize) * axisScale.x;     //max x
        _scaleAxis[5] = _dir.x *  (axisX() * size) * axisScale.x;           //x

        _scaleAxis[6] = _dir.y *  (axisY() * triMinSize) * axisScale.y;     //min y
        _scaleAxis[8] = _dir.y *  (axisY() * triMaxSize) * axisScale.y;      //max y
        _scaleAxis[10] = _dir.y * (axisY() * size) * axisScale.y;           //y

        _scaleAxis[11] = _dir.z * (axisZ() * triMinSize) * axisScale.z;     //min z
        _scaleAxis[13] = _dir.z * (axisZ() * triMaxSize) *axisScale.z;      //max z
        _scaleAxis[15] = _dir.z * (axisZ() * size) * axisScale.z;           //z

        _scaleAxis[2] = (_scaleAxis[1] + _scaleAxis[6]) * 0.5f;              //min xy
        _scaleAxis[4] = (_scaleAxis[3] + _scaleAxis[8]) * 0.5f;              //max xy

        _scaleAxis[7] = (_scaleAxis[6] + _scaleAxis[11]) * 0.5f;             //min yz
        _scaleAxis[9] = (_scaleAxis[8] + _scaleAxis[13]) * 0.5f;             //max yz

        _scaleAxis[12] = (_scaleAxis[11] + _scaleAxis[1]) * 0.5f;            //min zx
        _scaleAxis[14] = (_scaleAxis[13] + _scaleAxis[3]) * 0.5f;            //max zx

        real3 dir = camera.getDir();
        real3 right = camera.getRight();
        real3 up = camera.getUp();
        real arSize = size * 0.035;
        ///计算箭头
        ///x
        real3 arrX = _scaleAxis[5];
        _scaleAxisArr[0] = arrX;
        _scaleAxisArr[1] = arrX + (right + up) * arSize;
        _scaleAxisArr[2] = arrX + (right - up) * arSize;
        _scaleAxisArr[3] = arrX + (-right - up) * arSize;
        _scaleAxisArr[4] = arrX + (-right + up) * arSize;
        ///y
        real3 arrY = _scaleAxis[10];
        _scaleAxisArr[5] = arrY;
        _scaleAxisArr[6] = arrY + (right + up) * arSize;
        _scaleAxisArr[7] = arrY + (right - up) * arSize;
        _scaleAxisArr[8] = arrY + (-right - up) * arSize;
        _scaleAxisArr[9] = arrY + (-right + up) * arSize;
        ///z
        real3 arrZ = _scaleAxis[15];
        _scaleAxisArr[10] = arrZ;
        _scaleAxisArr[11] = arrZ + (right + up) * arSize;
        _scaleAxisArr[12] = arrZ + (right - up) * arSize;
        _scaleAxisArr[13] = arrZ + (-right - up) * arSize;
        _scaleAxisArr[14] = arrZ + (-right + up) * arSize;
    }

    const   float3 (&FEEditAxisScale::scaleAxis())[16]
    {
        return  _scaleAxis;
    }
    const   float3 (&FEEditAxisScale::scaleAxisArr())[15]
    {
        return  _scaleAxisArr;
    }

    ///缩放计算
    real3 FEEditAxisScale::calcScale(const int2& start, const int2& end)
    {
        switch (_selectedAxis)
        {
        case FEEditAxisScale::AXIS_NULL:
            break;
        case FEEditAxisScale::AXIS_X:
        {
            real retK = scaleX(start, end);
            _offScale = real3(retK, 1.0, 1.0);
        }
        break;
        case FEEditAxisScale::AXIS_Y:
        {
            real retK = scaleY(start, end);
            _offScale = real3(1.0, retK, 1.0);
        }
        break;
        case FEEditAxisScale::AXIS_Z:
        {
            real retK = scaleZ(start, end);
            _offScale = real3(1.0, 1.0, retK);
        }
        break;
        case FEEditAxisScale::AXIS_XY:
        {
            real retK = scaleXY(start, end);
            _offScale = real3(retK, retK, 1.0);
            if (!enabled(FEEditAxisScale::AXIS_X))
                _offScale.x = 1.0;
            if (!enabled(FEEditAxisScale::AXIS_Y))
                _offScale.y = 1.0;
        }
        break;
        case FEEditAxisScale::AXIS_YZ:
        {
            real retK = scaleYZ(start, end);
            _offScale = real3(1.0, retK, retK);
            if (!enabled(FEEditAxisScale::AXIS_Y))
                _offScale.y = 1.0;
            if (!enabled(FEEditAxisScale::AXIS_Z))
                _offScale.z = 1.0;
        }
        break;
        case FEEditAxisScale::AXIS_XZ:
        {
            real retK = scaleXZ(start, end);
            _offScale = real3(retK, 1.0, retK);
            if (!enabled(FEEditAxisScale::AXIS_X))
                _offScale.x = 1.0;
            if (!enabled(FEEditAxisScale::AXIS_Z))
                _offScale.z = 1.0;
        }
        break;
        case FEEditAxisScale::AXIS_XYZ:
        {
            real retK = scaleXYZ(start, end);
            _offScale = real3(retK, retK, retK);
            if (!enabled(FEEditAxisScale::AXIS_X))
                _offScale.x = 1.0;
            if (!enabled(FEEditAxisScale::AXIS_Y))
                _offScale.y = 1.0;
            if (!enabled(FEEditAxisScale::AXIS_Z))
                _offScale.z = 1.0;
        }
        break;
        default:
            break;
        }
        return _offScale;
    }
    ///高亮轴
    FEEditAxisScale::AXIS FEEditAxisScale::hoverAxis(const int2& pos)
    {
        FEEditAxisScale::AXIS oldHovered = _hoveredAxis;
        _hoveredAxis = pickAxis(pos);
        if (_hoveredAxis != FEEditAxisScale::AXIS_NULL)
        {
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return _hoveredAxis;
        }
        _hoveredAxis = pickTriMax(pos);
        if (_hoveredAxis != FEEditAxisScale::AXIS_NULL)
        {
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return _hoveredAxis;
        }

        if (oldHovered != _hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
        _hoveredAxis = pickTriMin(pos);
        return _hoveredAxis;
    }

    real3 FEEditAxisScale::calcAxisDir(FECamera& camera)
    {
        real3 dir = normalize(position() - camera.getEye());
        real3 axis[3] = { axisX(), axisY(), axisZ() };
        real3 rDir(1.0, 1.0, 1.0);
        for (size_t i = 0; i < 3; ++i)
        {
            ///计算对应轴与摄像机dir的方向
            real d = dot(axis[i], dir);
            if (d > 0)
            {///如果轴与dir同向，证明轴朝向了屏幕背面，需要将对应轴的方向置反
                rDir[i] = -1.0;
            }
            else
            {///还原轴的方向
                rDir[i] = 1.0;
            }
        }
        return rDir;
    }

    FEEditAxisScale::AXIS FEEditAxisScale::pickTriMin(const int2& screen)
    {
        FECamera& camera  =   _ctx.activeCamera();
        Ray         ray     =   camera.createRayFromScreen(screen.x, screen.y);
        real t, u, v;
        real3 tmpPos0 = real3(_scaleAxis[0]) + position();
        real3 tmpPos1 = real3(_scaleAxis[1]) + position();
        real3 tmpPos6 = real3(_scaleAxis[6]) + position();
        real3 tmpPos11 = real3(_scaleAxis[11]) + position();
        /// 1, 6, 11,   0, 1, 6,   0 , 6, 11,   0 ,11, 1
        if (intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos1, tmpPos6, tmpPos11, &t, &u, &v)
            || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos1, tmpPos6, &t, &u, &v)
            || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos6, tmpPos11, &t, &u, &v)
            || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos11, tmpPos1, &t, &u, &v)
            )
        {

            if (enabled(FEEditAxisScale::AXIS_X)
                || enabled(FEEditAxisScale::AXIS_Y)
                || enabled(FEEditAxisScale::AXIS_Z))
                return FEEditAxisScale::AXIS_XYZ;
        }
        return FEEditAxisScale::AXIS_NULL;
    }
    FEEditAxisScale::AXIS FEEditAxisScale::pickTriMax(const int2& screen)
    {
        FECamera &camera  =   _ctx.activeCamera();
        Ray         ray     =   camera.createRayFromScreen(screen.x, screen.y);

        real3 quadXY[4] =
        {
            real3(_scaleAxis[1]) + position(), real3(_scaleAxis[3]) + position(),
            real3(_scaleAxis[8]) + position(), real3(_scaleAxis[6]) + position(),
        };
        real3 quadYZ[4] =
        {
            real3(_scaleAxis[6]) + position(), real3(_scaleAxis[8]) + position(),
            real3(_scaleAxis[13]) + position(), real3(_scaleAxis[11]) + position(),
        };
        real3 quadXZ[4] =
        {
            real3(_scaleAxis[11]) + position(), real3(_scaleAxis[13]) + position(),
            real3(_scaleAxis[3]) + position(), real3(_scaleAxis[1]) + position(),
        };
        real3 retPt;
        if (GetRayRectaceInsPt(ray, quadXY, retPt))
        {
            if (enabled(FEEditAxisScale::AXIS_X)
                || enabled(FEEditAxisScale::AXIS_Y))
                return FEEditAxisScale::AXIS_XY;
        }
        if (GetRayRectaceInsPt(ray, quadYZ, retPt))
        {
            if (enabled(FEEditAxisScale::AXIS_Y)
                || enabled(FEEditAxisScale::AXIS_Z))
                return FEEditAxisScale::AXIS_YZ;
        }
        if (GetRayRectaceInsPt(ray, quadXZ, retPt))
        {
            if (enabled(FEEditAxisScale::AXIS_X)
                || enabled(FEEditAxisScale::AXIS_Z))
                return FEEditAxisScale::AXIS_XZ;
        }
        return FEEditAxisScale::AXIS_NULL;
    }
    FEEditAxisScale::AXIS FEEditAxisScale::pickAxis(const int2& screen)
    {
        static const real DIST_MAX = 6;
        FECamera &camera = _ctx.activeCamera();
        real3       mouse(screen.x, screen.y, 0);
        real2       startX = camera.worldToScreen(real3(_scaleAxis[1]) + position());
        real2       startY = camera.worldToScreen(real3(_scaleAxis[6]) + position());
        real2       startZ = camera.worldToScreen(real3(_scaleAxis[11]) + position());
        ///轴数据
        real2       xAxis = camera.worldToScreen(real3(_scaleAxis[5]) + position());
        real2       yAxis = camera.worldToScreen(real3(_scaleAxis[10]) + position());
        real2       zAxis = camera.worldToScreen(real3(_scaleAxis[15]) + position());

        /// 拾取坐标轴
        real        dist[3] = { 0,0,0 };
        dist[0] = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(xAxis.x, xAxis.y, 0), mouse);
        dist[1] = closeDistanceOnLine(real3(startY.x, startY.y, 0), real3(yAxis.x, yAxis.y, 0), mouse);
        dist[2] = closeDistanceOnLine(real3(startZ.x, startZ.y, 0), real3(zAxis.x, zAxis.y, 0), mouse);
        if (dist[0] == 0 && dist[1] == 0 && dist[2] == 0)
            return FEEditAxisScale::AXIS_NULL;
        int min = 0;
        if (dist[1] < dist[min])
            min = 1;
        if (dist[2] < dist[min])
            min = 2;
        if (dist[min] > DIST_MAX)
            return FEEditAxisScale::AXIS_NULL;
        if (min == 0)
        {
            if (enabled(FEEditAxisScale::AXIS_X))
                return FEEditAxisScale::AXIS_X;
            else
                return FEEditAxisScale::AXIS_NULL;
        }
        if (min == 1)
        {
            if (enabled(FEEditAxisScale::AXIS_Y))
                return FEEditAxisScale::AXIS_Y;
            else
                return FEEditAxisScale::AXIS_NULL;
        }
        if (min == 2)
        {
            if (enabled(FEEditAxisScale::AXIS_Z))
                return FEEditAxisScale::AXIS_Z;
            else
                return FEEditAxisScale::AXIS_NULL;
        }
        return FEEditAxisScale::AXIS_NULL;
    }

    real FEEditAxisScale::scaleX(const int2& start, const int2& end)
    {
        real3       axis    =   axisX() * _dir.x;

        FECamera& camera  =   _ctx.activeCamera();
        real3       faceNor =   camera.getDir();
        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offScale.x;

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.x;
    }
    real FEEditAxisScale::scaleY(const int2& start, const int2& end)
    {
        real3       axis    =   axisY() * _dir.y;
        FECamera& camera  =   _ctx.activeCamera();
        real3       faceNor =   camera.getDir();
        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offScale.y;

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.y;
    }
    real FEEditAxisScale::scaleZ(const int2& start, const int2& end)
    {
        real3       axis    =   axisZ() * _dir.z;

        FECamera& camera  =   _ctx.activeCamera();
        real3       faceNor =   camera.getDir();
        if (!calcAxisPlaneNormalize(camera, axis, faceNor))
            return _offScale.z;

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.z;
    }
    real FEEditAxisScale::scaleXY(const int2& start, const int2& end)
    {
        real3 axis = normalize(axisX() * _dir.x + axisY() * _dir.y);

        real3 faceNor = normalize(cross(axisX(), axisY()));

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.x;
    }
    real FEEditAxisScale::scaleYZ(const int2& start, const int2& end)
    {
        real3 axis = normalize(axisY() * _dir.y + axisZ() * _dir.z);

        real3 faceNor = normalize(cross(axisY(), axisZ()));

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.y;
    }
    real FEEditAxisScale::scaleXZ(const int2& start, const int2& end)
    {
        real3 axis = normalize(axisX() * _dir.x + axisZ() * _dir.z);

        real3 faceNor = normalize(cross(axisX(), axisZ()));

        real rets(1.0);
        if (calcScaleValue(start, end, faceNor, axis, rets, 1.0))
        {
            return rets;
        }
        return _offScale.z;
    }
    real FEEditAxisScale::scaleXYZ(const int2& start, const int2& end)
    {
        FECamera& camera  =   _ctx.activeCamera();
        real3       axis    =   camera.getUp();
        real3       faceNor =   camera.getDir();

        real        retS(1.0);
        Ray         rayStart=   camera.createRayFromScreen(start.x, start.y);
        Ray         rayEnd  =   camera.createRayFromScreen(end.x, end.y);
        ///这里将中心点向下移动，目的是降低缩放的灵敏度,防止缩放时跳动太快的访问
        real3 cenPos = position() - axis * _axisSize;
        real3 retPt0(0);
        real3 retPt1(0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, cenPos, retPt0) &&
            calcRaySurFaceInsPt(rayEnd, faceNor, cenPos, retPt1))
        {
            real3 pt0 = FE::closePointOnVector(axis, cenPos, retPt0);
            real3 pt1 = FE::closePointOnVector(axis, cenPos, retPt1);
            real3 nor = normalize(pt1 - pt0);
            ///确定当前点在鼠标按下点的那一侧
            real  d = dot(nor, axis);
            real dis0 = distance(cenPos, pt0);
            real dis1 = distance(pt0, pt1);
            real s = (dis0 + dis1) / dis0;
            if (d > FLT_EPSILON)
            {
                ///加入灵敏度计算
                s = 1 + (s - 1);
                retS = s;
            }
            else if (d < -FLT_EPSILON)
            {
                s = 1.0 / s;
                ///加入灵敏度计算
                s = 1 + (s - 1);
                retS = s;
            }
            else
            {
                retS = 1.0;
            }
            return retS;
        }
        return _offScale.x;
    }

    bool FEEditAxisScale::calcScaleValue(const int2& start, const int2& end
        , const real3& faceNor, const real3& axis
        , real& retS, real limit)
    {
        retS = 1.0;
        FECamera& camera      =   _ctx.activeCamera();
        Ray         rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray         rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3       retPt0(0);
        real3       retPt1(0);
        if (calcRaySurFaceInsPt(rayStart, faceNor, position(), retPt0) &&
            calcRaySurFaceInsPt(rayEnd, faceNor, position(), retPt1))
        {
            real3 pt0 = FE::closePointOnVector(axis, position(), retPt0);
            real3 pt1 = FE::closePointOnVector(axis, position(), retPt1);
            real3 nor = normalize(pt1 - pt0);
            ///确定当前点在鼠标按下点的那一侧
            real  d = dot(nor, axis);
            real dis0 = distance(position(), pt0);
            real dis1 = distance(pt0, pt1);
            real s = (dis0 + dis1) / dis0;
            if (d > FLT_EPSILON)
            {
                ///加入灵敏度计算
                s = 1 + (s - 1) * limit;
                retS = s;
            }
            else if (d < -FLT_EPSILON)
            {
                s = 1.0 / s;
                ///加入灵敏度计算
                s = 1 + (s - 1) * limit;
                retS = s;
            }
            else
            {
                retS = 1.0;
            }
            return true;
        }
        return false;
    }

    void    FEEditAxisScale::onMessage(const FEMessage& inputMsg)
    {
        ///说明已经被其他组件捕获了
        ///不处理消息
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

    bool FEEditAxisScale::mouseButtonPress(const int2& pos)
    {
        _bMouseDown = true;

        AXIS oldSelected = _selectedAxis;
        _selectedAxis = _hoveredAxis;
        if (oldSelected != _selectedAxis)
        {
            this->sendSelectedDelegate();
            _ctx.requireNextFrame();
        }

        if (isAxisSelected())
        {
            _downPos = pos;
            _startPos = pos;
            _offScale = real3(1.0, 1.0, 1.0);
            _bScaleing = true;
            _delegate(EditStatus::EditStart, real3(1.0), real3(1.0), *this);
            _ctx.requireNextFrame();
            ///捕获消息系统,禁止继续分发
            setCapture();
            return true;
        }
        return false;
    }
    bool FEEditAxisScale::mouseButtonRelease(const int2& pos)
    {
        _bMouseDown = false;
        if (isAxisSelected())
        {
            _delegate(EditStatus::EditEnd, real3(1.0), _offScale, *this);
            _downPos = pos;
            _startPos = pos;
            _offScale = real3(1.0, 1.0, 1.0);

            AXIS oldSelected = _selectedAxis;
            _selectedAxis = AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
                _ctx.requireNextFrame();
            }

            _bScaleing = false;
            releaseCapture();
            return true;
        }
        return false;
    }
    bool FEEditAxisScale::mouseMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 prevScale = _offScale;
            real3 scale = calcScale(_startPos, pos);
            scale = scale / prevScale;
            _delegate(EditStatus::Editting, scale, _offScale, *this);
            _downPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        else if(!_bMouseDown)
        {
            if (hoverAxis(pos) != AXIS_NULL)
                setCapture();
            else
                releaseCapture();
        }
        return false;
    }

    bool FEEditAxisScale::touchDown(const int2& pos)
    {
        _bTouchDown = true;
        if (this->isAxisSelected())
        {
            _bTouchPickup = false;
            _touchDownPos = pos;
            _touchStartPos = pos;
            _offScale = real3(1.0, 1.0, 1.0);
            _bScaleing = true;
            _delegate(EditStatus::EditStart, real3(1.0), real3(1.0), *this);
            return true;
        }
        else
        {
            _bTouchPickup = true;
            return false;
        }
    }
    bool FEEditAxisScale::touchUp(const int2& pos)
    {
        _bTouchDown = false;
        if (_bTouchPickup)
        {///拾取动作
            AXIS oldSelected = _selectedAxis;
            _selectedAxis = hoverAxis(pos);
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }
            _bTouchPickup = false;
            return false;
        }
        else if (isAxisSelected())
        {
            _delegate(EditStatus::EditEnd, real3(1.0), _offScale, *this);
            _downPos = pos;
            _startPos = pos;

            AXIS oldSelected = _selectedAxis;
            _selectedAxis = AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            AXIS oldHovered = _hoveredAxis;
            _hoveredAxis = AXIS_NULL;
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }

            _offScale = real3(1.0, 1.0, 1.0);
            _bScaleing = false;
            return true;
        }
        return false;
    }
    bool FEEditAxisScale::touchMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 prevScale = _offScale;
            real3 scale = calcScale(_startPos, pos);
            scale = scale / prevScale;
            _delegate(EditStatus::Editting, scale, _offScale, *this);
            _downPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        return false;
    }
}

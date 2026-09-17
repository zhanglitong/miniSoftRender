#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEEditAxisRotate.h"
#include    <stack>

namespace   FE
{
    FEEditAxisRotate::FEEditAxisRotate(FEContext& context)
        :FEEditAxis(EditAxisTypeRotate, context)
    {
        _radiusBall              =   0.0;
        _radiusCircle            =   0.0;
        _selectedAxis            =   FEEditAxisRotate::AXIS::AXIS_NULL;
        _hoveredAxis             =   FEEditAxisRotate::AXIS::AXIS_NULL;
        _bRotatting              =   false;
        _offAngle                =   0.0;
        _bMouseDown              =   false;

        _rotationAxis            =   AxisX();
        _rotationAngleWeight     =   0.0;

        _adsorptionEnabled       =   false;
        _adsorptionFlags         =   FEEditAxisRotate::AdsorptionFlag::AF_Angle;
        _adsorptionAngle         =   5.0;
    }
    FEEditAxisRotate::FEEditAxisRotate(const FEEditAxisRotate& other)
        :FEEditAxis(EditAxisTypeRotate, other._ctx)
    {
        _radiusBall              =   other._radiusBall;
        _radiusCircle            =   other._radiusCircle;
        _selectedAxis            =   other._selectedAxis;
        _hoveredAxis             =   other._hoveredAxis;
        _bRotatting              =   other._bRotatting;
        _offAngle                =   other._offAngle;
        _bMouseDown              =   other._bMouseDown;
        _rotationAxis            =   other._rotationAxis;
        _rotationAngleWeight     =   other._rotationAngleWeight;
        _adsorptionEnabled       =   other._adsorptionEnabled;
        _adsorptionFlags         =   other._adsorptionFlags;
        _adsorptionAngle         =   other._adsorptionAngle;
    }
    FEEditAxisRotate::~FEEditAxisRotate()
    {
    }

    FEEditAxisRotate::MDelegate& FEEditAxisRotate::mDelegate()
    {
        return  _delegate;
    }

    FEEditAxisRotate::AXIS FEEditAxisRotate::hoveredAxis() const
    {
        return  _hoveredAxis;
    }
    FEEditAxisRotate::AXIS FEEditAxisRotate::selectedAxis() const
    {
        return  _selectedAxis;
    }

    void FEEditAxisRotate::setAdsorptionEnabled(bool enabled)
    {
        _adsorptionEnabled  =   enabled;
    }
    bool FEEditAxisRotate::adsorptionEnabled() const
    {
        return  _adsorptionEnabled;
    }
    FEEditAxisRotate::AdsorptionFlags FEEditAxisRotate::adsorptionFlags() const
    {
        return  _adsorptionFlags;
    }
    void FEEditAxisRotate::setAdsorptionFlags(AdsorptionFlags flags)
    {
        _adsorptionFlags    =   flags;
    }
    real FEEditAxisRotate::adsorptionAngle() const
    {
        return  _adsorptionAngle;
    }
    void FEEditAxisRotate::setAdsorptionAngle(real angle)
    {
        _adsorptionAngle    =   angle;
    }

    void FEEditAxisRotate::cancelHovered()
    {
        AXIS    oldHovered  =   _hoveredAxis;
        _hoveredAxis    =   FEEditAxisRotate::AXIS::AXIS_NULL;
        if (oldHovered != _hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditAxisRotate::cancelSelected()
    {
        AXIS    oldSelected  =   _selectedAxis;
        _selectedAxis    =   FEEditAxisRotate::AXIS::AXIS_NULL;
        if (oldSelected != _selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    /// <summary>
    /// 更新旋转轴顶点
    /// </summary>
    void    FEEditAxisRotate::update(FECamera& camera)
    {
        real3       cenPos  =   real3(0, 0, 0);
        real        unitF   =   camera.pixelU(position());
        _radiusCircle   =   unitF * 80;
        _radiusBall     =   unitF * 60;
        real size       =   unitF * 30;
        ///计算坐标轴
        ///X
        _rotateAxis[0] = cenPos;
        _rotateAxis[1] = axisX() * size;
        ///y
        _rotateAxis[2] = cenPos;
        _rotateAxis[3] = axisY() * size;
        ///z
        _rotateAxis[4] = cenPos;
        _rotateAxis[5] = axisZ() * size;
        real step = real(DEG2RAD(12.0));
        real3 dir = camera.getDir();
        real3 right = FE::normalize(FE::cross(camera.getUp(), dir));

        for (int i = 0; i <= 30; ++i)
        {
            mat4r rMat(1.0);
            rMat = FE::rotate(rMat, real(i) * step, dir);
            real3 nor = real3(rMat * real4(right, 0.0));
            ///三个坐标轴构成的球
            _rotateScreenCricle[i] = nor * _radiusBall;
            ///大圆
            _rotateScreenCricle[i + 31] = nor * _radiusCircle;
        }
        ///三个轴的旋转弧线
        _vRotateAxis[0].clear();
        _vRotateAxis[1].clear();
        _vRotateAxis[2].clear();
        /// 使用摄像机前向方向做背面剔除(与原始代码一致)
        real    min     =   0.3;

        auto    buildArc    =   [&](int axisIdx, const real3& rotAxis)
        {
            real3   tAxis;
            switch(axisIdx)
            {
                case 0: tAxis = vectorPerpendicularToAxisX(); break;
                case 1: tAxis = vectorPerpendicularToAxisY(); break;
                default:tAxis = vectorPerpendicularToAxisZ(); break;
            }
            /// 原始代码的 insert 逻辑: 处理弧线环绕 0/30 边界
            int index       =   0;
            int insertIndex =   0;
            for (int i = 0; i <= 30; ++i)
            {
                mat4r rMat(1.0);
                rMat = FE::rotate(rMat, real(i) * step, rotAxis);
                real3 pt    =   normalize(real3(rMat * real4(tAxis, 0.0))) * _radiusBall;
                real3 norv  =   FE::normalize(pt - cenPos);
                if (FE::dot(norv, dir) < min)
                {
                    /// 保留正面朝向摄像机的可见弧线点
                    if (index == 0)
                        _vRotateAxis[axisIdx].push_back(float3(pt));
                    else
                    {
                        _vRotateAxis[axisIdx].insert(_vRotateAxis[axisIdx].begin() + insertIndex, float3(pt));
                        insertIndex++;
                    }
                }
                else
                    index = i;
            }
        };
        buildArc(0, axisX());
        buildArc(1, axisY());
        buildArc(2, axisZ());
    }

    const   float3 (&FEEditAxisRotate::rotateAxis())[6]
    {
        return  _rotateAxis;
    }
    const   float3 (&FEEditAxisRotate::rotateScreenCircle())[62]
    {
        return  _rotateScreenCricle;
    }
    const   float3s    (&FEEditAxisRotate::rotateArcs())[3]
    {
        return  _vRotateAxis;
    }
    const   float3 (&FEEditAxisRotate::tangent())[8]
    {
        return  _tangent;
    }
    const   float3s&    FEEditAxisRotate::fan() const
    {
        return  _fan;
    }

    ///高亮轴
    FEEditAxisRotate::AXIS FEEditAxisRotate::hoverAxis(const int2& pos)
    {
        FEEditAxisRotate::AXIS oldHovered = _hoveredAxis;
        _hoveredAxis = FEEditAxisRotate::AXIS_NULL;
        ///首先计算是否与X,Y,Z轴对应的圆弧相交
        _hoveredAxis = pickAxis(pos);
        if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
        {
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return _hoveredAxis;
        }
        _hoveredAxis = pickBall(pos);
        if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
        {
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return _hoveredAxis;
        }
        _hoveredAxis = pickCricle(pos);
        if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
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
        return _hoveredAxis;
    }
    ///更新此次的旋转轴方向以及该旋转方向的权重
    void FEEditAxisRotate::updateRotationAxisAndWeight()
    {
        _rotationAxis = axisX();
        _rotationAngleWeight = 0.0;
        switch (_selectedAxis)
        {
        case FE::FEEditAxisRotate::AXIS_X:
        {
            _rotationAxis = axisX();
            _rotationAngleWeight = 1.0;
        }break;
        case FE::FEEditAxisRotate::AXIS_Y:
        {
            _rotationAxis = axisY();
            _rotationAngleWeight = 1.0;
        }break;
        case FE::FEEditAxisRotate::AXIS_Z:
        {
            _rotationAxis = axisZ();
            _rotationAngleWeight = 1.0;
        }break;
        case FE::FEEditAxisRotate::AXIS_SC:
        {
            _rotationAxis           =   normalize(_ctx.activeCamera().getDir());
            _rotationAngleWeight    =   1.0;
        }break;
        default:
        {
            return ;
        }break;
        }
        ///加入轴禁用计算
        if (!enabled(FEEditAxisRotate::AXIS_X))
        {
            _rotationAxis = ptProjToPlane(_rotationAxis, axisX(), real3(0.0));
            _rotationAngleWeight = length(_rotationAxis);
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                _rotationAngleWeight = 0.0;
            else
                _rotationAxis = normalize(_rotationAxis);
        }
        if (!enabled(FEEditAxisRotate::AXIS_Y))
        {
            _rotationAxis = ptProjToPlane(_rotationAxis, axisY(), real3(0.0));
            _rotationAngleWeight = length(_rotationAxis);
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                _rotationAngleWeight = 0.0;
            else
                _rotationAxis = normalize(_rotationAxis);
        }
        if (!enabled(FEEditAxisRotate::AXIS_Z))
        {
            _rotationAxis = ptProjToPlane(_rotationAxis, axisZ(), real3(0.0));
            _rotationAngleWeight = length(_rotationAxis);
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                _rotationAngleWeight = 0.0;
            else
                _rotationAxis = normalize(_rotationAxis);
        }
    }
    ///旋转计算
    void FEEditAxisRotate::calcRotate(const int2& start, const int2& end, real& rAngle)
    {
        rAngle = 0.0;
        if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
            return;
        switch (_selectedAxis)
        {
        case FEEditAxisRotate::AXIS_X:
            rAngle = this->rotateX(start, end, _rotationAxis) * _rotationAngleWeight;
            break;
        case FEEditAxisRotate::AXIS_Y:
            rAngle = this->rotateY(start, end, _rotationAxis) * _rotationAngleWeight;
            break;
        case FEEditAxisRotate::AXIS_Z:
            rAngle = this->rotateZ(start, end, _rotationAxis) * _rotationAngleWeight;
            break;
        case FEEditAxisRotate::AXIS_SC:
            rAngle = this->rotateSC(start, end, _rotationAxis) * _rotationAngleWeight;
            break;
        default:
            rAngle = 0.0;
            break;
        }
        ///角度吸附
        if (_adsorptionEnabled && _adsorptionFlags.hasFlag(FEEditAxisRotate::AdsorptionFlag::AF_Angle))
        {
            int ratio = static_cast<int>(rAngle / _adsorptionAngle);
            rAngle = _adsorptionAngle * static_cast<real>(ratio);
        }
        ///
        _offAngle = rAngle;
    }

    FEEditAxisRotate::AXIS FEEditAxisRotate::pickAxis(const int2& screen)
    {
        real3 mouse(screen.x, screen.y, 0);
        static const real DIST_MAX = 4;
        FECamera&   camera  =   _ctx.activeCamera();
        real        dist[3] =   { FLT_MAX, FLT_MAX, FLT_MAX };
        if (_vRotateAxis[0].size() == 0)
            return FEEditAxisRotate::AXIS_NULL;
        for (size_t index = 0; index < 3; ++index)
        {
            const float3s& vs = _vRotateAxis[index];
            real& rDist = dist[index];
            int count = (int)(vs.size());
            int cenIndex = count / 2;
            bool bPickup = false;
            ///先拾取右侧(中心点到右侧所有点)
            for (int i = cenIndex; i < count - 1; ++i)
            {
                real2 startX = camera.worldToScreen(real3(vs[i]) + position());
                real2 endX = camera.worldToScreen(real3(vs[i + 1]) + position());
                real dis = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(endX.x, endX.y, 0), mouse);
                if (dis <= DIST_MAX)
                {
                    bPickup = true;
                    rDist = dis;
                    break;
                }
            }
            ///已拾取到，跳过左侧拾取
            if (bPickup)
                continue;
            ///再拾取左侧
            for (int i = cenIndex; i > 0; --i)
            {
                real2 startX = camera.worldToScreen(real3(vs[i]) + position());
                real2 endX = camera.worldToScreen(real3(vs[i - 1]) + position());
                real dis = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(endX.x, endX.y, 0), mouse);
                if (dis <= DIST_MAX)
                {
                    rDist = dis;
                    break;
                }
            }
        }
        ///计算距离最小
        int min = 0;
        if (dist[1] < dist[min])
            min = 1;
        if (dist[2] < dist[min])
            min = 2;
        if (dist[min] > DIST_MAX)
            return  FEEditAxisRotate::AXIS_NULL;
        if (min == 0)
        {
            if (enabled(FEEditAxisRotate::AXIS_X))
                return  FEEditAxisRotate::AXIS_X;
            else
                return  FEEditAxisRotate::AXIS_NULL;
        }
        if (min == 1)
        {
            if (enabled(FEEditAxisRotate::AXIS_Y))
                return  FEEditAxisRotate::AXIS_Y;
            else
                return  FEEditAxisRotate::AXIS_NULL;
        }
        if (min == 2)
        {
            if (enabled(FEEditAxisRotate::AXIS_Z))
                return  FEEditAxisRotate::AXIS_Z;
            else
                return  FEEditAxisRotate::AXIS_NULL;
        }
        return  FEEditAxisRotate::AXIS_NULL;
    }
    FEEditAxisRotate::AXIS FEEditAxisRotate::pickBall(const int2& )
    {
        return FEEditAxisRotate::AXIS_NULL;
    }
    FEEditAxisRotate::AXIS FEEditAxisRotate::pickCricle(const int2& screen)
    {
        /// 大圆比轴更粗更靠外, 用更大的拾取范围
        static const real DIST_MAX = 10;
        FECamera &camera = _ctx.activeCamera();
        real3       mouse(screen.x, screen.y, 0);
        real        minDis = FLT_MAX;
        for (int i = 0; i < 30; ++i)
        {
            real2 startX = camera.worldToScreen(real3(_rotateScreenCricle[31 + i]) + position());
            real2 endX = camera.worldToScreen(real3(_rotateScreenCricle[31 + i + 1]) + position());
            real d = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(endX.x, endX.y, 0), mouse);
            if (d < minDis) minDis = d;
        }
        if (minDis < DIST_MAX && (enabled(FEEditAxisRotate::AXIS_X)
            || enabled(FEEditAxisRotate::AXIS_Y)
            || enabled(FEEditAxisRotate::AXIS_Z)))
        {
            return FEEditAxisRotate::AXIS_SC;
        }
        return FEEditAxisRotate::AXIS_NULL;
    }

    real FEEditAxisRotate::rotateX(const int2& start, const int2& end, const real3& axis)
    {
        FECamera& camera = _ctx.activeCamera();
        real3       vtStart;
        real3       vtEnd;
        calcTangentAndFan(axis, _radiusBall, vtStart, vtEnd);
        if (vtStart == vtEnd)
            return _offAngle;
        Ray rayStart = camera.createRayFromScreen(start.x, start.y);
        Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
        real3 retPtStart;
        real3 retPtEnd;
        if (FE::calcRaySurFaceInsPt(rayStart, camera.getDir(), position(), retPtStart)
            && FE::calcRaySurFaceInsPt(rayEnd, camera.getDir(), position(), retPtEnd))
        {
            real3 norV = FE::normalize(retPtEnd - retPtStart);
            real3 norVt = FE::normalize(vtEnd - vtStart);
            real3 vPos = FE::closePointOnVector(norVt, retPtStart, retPtEnd);
            real off = FE::distance(retPtStart, vPos);
            off = off / camera.pixelU(position());
            if (dot(norV, norVt) < 0)///计算拖动方向与切线方向是否相同
                off = -off;
            return off;
        }
        return _offAngle;
    }
    real FEEditAxisRotate::rotateY(const int2& start, const int2& end, const real3& axis)
    {
        FECamera& camera = _ctx.activeCamera();
        real3       vtStart;
        real3       vtEnd;
        calcTangentAndFan(axis, _radiusBall, vtStart, vtEnd);
        if (vtStart == vtEnd)
            return _offAngle;
        Ray rayStart = camera.createRayFromScreen(start.x, start.y);
        Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
        real3 retPtStart;
        real3 retPtEnd;
        if (FE::calcRaySurFaceInsPt(rayStart, camera.getDir(), position(), retPtStart)
            && FE::calcRaySurFaceInsPt(rayEnd, camera.getDir(), position(), retPtEnd))
        {
            real3 norV = FE::normalize(retPtEnd - retPtStart);
            real3 norVt = FE::normalize(vtEnd - vtStart);
            real3 vPos = FE::closePointOnVector(norVt, retPtStart, retPtEnd);
            real off = FE::distance(retPtStart, vPos);
            off = off / camera.pixelU(position());
            if (dot(norV, norVt) < 0)
                off = -off;
            return off;
        }
        return _offAngle;
    }
    real FEEditAxisRotate::rotateZ(const int2& start, const int2& end, const real3& axis)
    {
        FECamera& camera  =   _ctx.activeCamera();
        real3       vtStart;
        real3       vtEnd;
        calcTangentAndFan(axis, _radiusBall, vtStart, vtEnd);
        if (vtStart == vtEnd)
            return _offAngle;
        Ray rayStart = camera.createRayFromScreen(start.x, start.y);
        Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
        real3 retPtStart;
        real3 retPtEnd;
        if (FE::calcRaySurFaceInsPt(rayStart, camera.getDir(), position(), retPtStart)
            && FE::calcRaySurFaceInsPt(rayEnd, camera.getDir(), position(), retPtEnd))
        {
            real3 norV = FE::normalize(retPtEnd - retPtStart);
            real3 norVt = FE::normalize(vtEnd - vtStart);
            real3 vPos = FE::closePointOnVector(norVt, retPtStart, retPtEnd);
            real off = FE::distance(retPtStart, vPos);
            off = off / camera.pixelU(position());
            if (dot(norV, norVt) < 0)
                off = -off;
            return off;
        }
        return _offAngle;
    }
    real FEEditAxisRotate::rotateSC(const int2& start, const int2& end, const real3& axis)
    {
        FECamera& camera  =   _ctx.activeCamera();
        real3       vtStart;
        real3       vtEnd;
        calcTangentAndFan(axis, _radiusCircle, vtStart, vtEnd);
        if (vtStart == vtEnd)
            return _offAngle;
        Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
        Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
        real3   retPtStart;
        real3   retPtEnd;
        if (FE::calcRaySurFaceInsPt(rayStart, camera.getDir(), position(), retPtStart)
            && FE::calcRaySurFaceInsPt(rayEnd, camera.getDir(), position(), retPtEnd))
        {
            real3 norV = FE::normalize(retPtEnd - retPtStart);
            real3 norVt = FE::normalize(vtEnd - vtStart);
            real3 vPos = FE::closePointOnVector(norVt, retPtStart, retPtEnd);
            real off = FE::distance(retPtStart, vPos);
            off = off / camera.pixelU(position());
            if (dot(norV, norVt) < 0)
                off = -off;
            return off;
        }
        return _offAngle;
    }

    void FEEditAxisRotate::calcTangentAndFan(const real3& rotateAxis, real radius, real3& vtStart, real3& vtEnd)
    {
        FE::FECamera&   camera      =   _ctx.activeCamera();
        Ray                 rayStart    =   camera.createRayFromScreen(_startPos.x, _startPos.y);
        real3               tangentPos;   ///切点
        real3               tangentNorV;  ///切线方向向量
        real3               retPt;
        std::pair<bool, real> ret = rayStart.intersectSphere(position(), radius);
        if (FE::calcRaySurFaceInsPt(rayStart, rotateAxis, position(), retPt))
        {
            tangentPos = normalize(retPt - position()) *  radius;
            tangentNorV = normalize(cross(rotateAxis, tangentPos));
        }
        else if (ret.first && ret.second >= 0)
        {//当摄像机_dir与旋转轴作为法向量的面平行时，使用平面无法计算出切点，因此采用球交点计算
         ///但球交点不一定在旋转轴上，因此计算出的切点不准确（仅仅显示不准确，对计算没有影响）
            retPt = rayStart.getPoint(ret.second);
            tangentPos = normalize(retPt - position()) *  radius;
            tangentNorV = normalize(cross(rotateAxis, tangentPos));
        }
        else
        {//不会进入到这里
            return;
        }
        real        unit = camera.pixelU(position());
        real        tLen = unit * 40;        //切线长度
        real        tLen1 = unit * 2;         //切线间距
        real        arSize = unit * 8;      //切线箭头
        quatr       qa = FE::angleAxis(real(DEG2RAD(40.0)), camera.getDir());
        quatr       qb = FE::angleAxis(real(DEG2RAD(40.0)), -camera.getDir());
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
            /// CELL 中 step=1 度, angleAxis 接收度数; FE 的 angleAxis 接收弧度, 统一转弧度
            real step = real(DEG2RAD(1.0));
            real total = real(DEG2RAD(std::abs(_offAngle)));
            for (real i = 0; i < total; i += step)
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

    void    FEEditAxisRotate::onMessage(const FEMessage& inputMsg)
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

    bool FEEditAxisRotate::mouseButtonPress(const int2& pos)
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
            _offAngle = 0.0;
            _bRotatting = false;
            ///更新计算此次的旋转轴以及其角度权重
            updateRotationAxisAndWeight();
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _delegate(EditStatus::EditStart, _rotationAxis, 0.0, 0.0, *this);
            _ctx.requireNextFrame();
            ///捕获消息系统,禁止继续分发
            setCapture();
            return true;
        }
        return false;
    }
    bool FEEditAxisRotate::mouseButtonRelease(const int2& pos)
    {
        _bMouseDown = false;
        if (isAxisSelected())
        {
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;

            _delegate(EditStatus::EditEnd, _rotationAxis, 0.0, _offAngle, *this);
            _downPos = pos;
            _startPos = pos;
            _offAngle = 0.0;
            _bRotatting = false;

            AXIS oldSelected = _selectedAxis;
            _selectedAxis = AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
                _ctx.requireNextFrame();
            }
            releaseCapture();
            return true;
        }
        return false;
    }
    bool FEEditAxisRotate::mouseMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            real angle = 0.0;
            real oldAngle = _offAngle;
            calcRotate(_startPos, pos, angle);
            angle = angle - oldAngle;
            _delegate(EditStatus::Editting, _rotationAxis, angle, _offAngle, *this);
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

    bool FEEditAxisRotate::touchDown(const int2& pos)
    {
        _bTouchDown = true;
        if (this->isAxisSelected())
        {
            _bTouchPickup = false;
            _touchDownPos = pos;
            _touchStartPos = pos;
            _offAngle = 0.0;
            _bRotatting = false;
            ///更新计算此次的旋转轴以及其角度权重
            updateRotationAxisAndWeight();
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _delegate(EditStatus::EditStart, _rotationAxis, 0.0, 0.0, *this);
            return true;
        }
        else
        {
            _bTouchPickup = true;
            return false;
        }
    }
    bool FEEditAxisRotate::touchUp(const int2& pos)
    {
        _bTouchDown = false;
        if (_bTouchPickup)
        {
            ///拾取动作
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
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _delegate(EditStatus::EditEnd, _rotationAxis, 0.0, _offAngle, *this);
            _touchDownPos = pos;
            _touchStartPos = pos;
            _offAngle = 0.0;
            _bRotatting = false;
            _bTouchPickup = false;

            AXIS oldSelected = _selectedAxis;
            _selectedAxis = AXIS::AXIS_NULL;
            if (oldSelected != _selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            AXIS oldHovered = _hoveredAxis;
            _hoveredAxis = AXIS::AXIS_NULL;
            if (oldHovered != _hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return true;
        }
        return false;
    }
    bool FEEditAxisRotate::touchMove(const int2& pos)
    {
        if (isAxisSelected())
        {
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            real angle = 0.0;
            real oldAngle = _offAngle;
            calcRotate(_startPos, pos, angle);
            angle = angle - oldAngle;
            _delegate(EditStatus::Editting, _rotationAxis, angle, _offAngle, *this);
            _downPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        return false;
    }
}

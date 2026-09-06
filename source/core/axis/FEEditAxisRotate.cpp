#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditAxisRotate.h"
#include    <stack>

namespace   FE
{
    class FEEditAxisRotatePrivate
    {
    private:
        struct PushBlock
        {
            mat4r     _mvp;
            float4      _color;
        };
    public:
        ///
        FEEditAxisRotate & _d;
        ///旋转轴
        float3 _rotateAxis[6];
        
        ///始终与屏幕平行的圆
        float3 _rotateScreenCricle[62];
        ///x, y, z 摄像机方向的圆弧
        std::vector<float3> _vRotateAxis[3];

        VertexBufPtr    _axisVBO    =   nullptr;

        ///切线
        float3 _tangent[8];
        ///旋转中扇形
        std::vector<float3> _fan;
        VertexBufPtr    _fanTangentVBO    =   nullptr;

        ///绘制管线
        GraphicPLPtr    _pipeLine;
        ///材质
        MaterialPtr     _material;
        ///切线IBO
        IndexBufPtr     _tangentIBO    =   nullptr;
        mat4r _tMat;

        ///球半径
        real _radiusBall;
        ///大圆半径
        real _radiusCircle;

        ///选中的轴
        FEEditAxisRotate::AXIS _selectedAxis;
        ///高亮的轴
        FEEditAxisRotate::AXIS _hoveredAxis;

        ///是否正在旋转
        bool _bRotatting;
        ///此次的旋转轴
        real3 _rotationAxis;
        ///此次的旋转角权重
        real _rotationAngleWeight;
        ///旋转过的角度
        real _offAngle;

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

        ///旋转吸附是否启用
        bool _adsorptionEnabled;
        ///旋转吸附标志
        FEEditAxisRotate::AdsorptionFlags _adsorptionFlags;
        ///旋转吸附角度
        real _adsorptionAngle;

        ///回调
        FEEditAxisRotate::MDelegate _delegate;
    public:
        FEEditAxisRotatePrivate(FEEditAxisRotate& d) :_d(d)
        {
            _radiusBall = 0.0;
            _radiusCircle = 0.0;
            _selectedAxis = FEEditAxisRotate::AXIS::AXIS_NULL;
            _hoveredAxis = FEEditAxisRotate::AXIS::AXIS_NULL;
            _bRotatting = false;
            _offAngle = 0.0;
            _bMouseDown = false;
            
            _rotationAxis = FEEditAxisRotate::AxisX();
            _rotationAngleWeight = 0.0;

            _adsorptionEnabled = false;
            _adsorptionFlags = FEEditAxisRotate::AdsorptionFlag::AF_Angle;
            _adsorptionAngle = 5.0;

            /// stub: 渲染管线与材质暂未接入 FE 图形系统
            _pipeLine = nullptr;
            _material = nullptr;
            _tangentIBO = nullptr;
        }
    public:
        ///更新轴顶点
        void updateAxisVerties(FEContext &context)
        {
            FECamera& camera  = context.activeCamera();
            real3       cenPos  = real3(0, 0, 0);
            real        unitF   = camera.pixelU(_d.position());
            _radiusCircle   =   unitF * 80;
            _radiusBall     =   unitF * 60;
            real size       =   unitF * 30;
            ///计算坐标轴
            ///X
            _rotateAxis[0] = cenPos;
            _rotateAxis[1] = _d.axisX() * size;
            ///y
            _rotateAxis[2] = cenPos;
            _rotateAxis[3] = _d.axisY() * size;
            ///z
            _rotateAxis[4] = cenPos;
            _rotateAxis[5] = _d.axisZ() * size;
            real step = 12.0;
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
            real min = 0.3;
            {///X
                real3 tAxis = _d.vectorPerpendicularToAxisX();
                int index = 0;
                int insertIndex = 0;
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisX());
                    real3 pt = normalize(real3(rMat * real4(tAxis, 0.0))) * _radiusBall;
                    real3 norv = FE::normalize(pt - cenPos);
                    if (FE::dot(norv, dir) < min)
                    {
                        if (index == 0)
                            _vRotateAxis[0].push_back(pt);
                        else
                        {
                            _vRotateAxis[0].insert(_vRotateAxis[0].begin() + insertIndex, pt);
                            insertIndex++;
                        }
                    }
                    else
                        index = i;
                }
            }
            {///y
                real3 tAxis = _d.vectorPerpendicularToAxisY();
                int index = 0;
                int insertIndex = 0;
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisY());
                    real3 pt = normalize(real3(rMat * real4(tAxis, 0.0))) * _radiusBall;
                    real3 norv = FE::normalize(pt - cenPos);
                    if (FE::dot(norv, dir) < min)
                    {
                        if (index == 0)
                            _vRotateAxis[1].push_back(pt);
                        else
                        {
                            _vRotateAxis[1].insert(_vRotateAxis[1].begin() + insertIndex, pt);
                            insertIndex++;
                        }
                    }
                    else
                        index = i;
                }
            }
            {///z
                real3 tAxis = _d.vectorPerpendicularToAxisZ();

                int index = 0;
                int insertIndex = 0;
                for (int i = 0; i <= 30; ++i)
                {
                    mat4r rMat(1.0);
                    rMat = FE::rotate(rMat, real(i) * step, _d.axisZ());
                    real3 pt = normalize(real3(rMat * real4(tAxis, 0.0))) * _radiusBall;
                    real3 norv = FE::normalize(pt - cenPos);
                    if (FE::dot(norv, dir) < min)
                    {
                        if (index == 0)
                            _vRotateAxis[2].push_back(pt);
                        else
                        {
                            _vRotateAxis[2].insert(_vRotateAxis[2].begin() + insertIndex, pt);
                            insertIndex++;
                        }
                    }
                    else
                        index = i;
                }
            }

            uint axisBufSize    =   6 * sizeof(float3);
            uint cricleBufSize  =   62 * sizeof(float3);
            uint arcBufSize0    =   (uint)_vRotateAxis[0].size() * sizeof(float3);
            uint arcBufSize1    =   (uint)_vRotateAxis[1].size() * sizeof(float3);
            uint arcBufSize2    =   (uint)_vRotateAxis[2].size() * sizeof(float3);
            uint bufSize        =   axisBufSize + cricleBufSize + arcBufSize0 + arcBufSize1 + arcBufSize2;

            /// stub: FEDevice 暂未提供 createVertexBuffer 接口，使用占位对象
            if (!_axisVBO)
            {
                _axisVBO = std::make_shared<VertexBuffer>();
            }
            uint offset = 0;
            _axisVBO->update(offset, axisBufSize, &_rotateAxis[0]);
            offset += axisBufSize;
            _axisVBO->update(offset, cricleBufSize, &_rotateScreenCricle[0]);
            offset += cricleBufSize;
            _axisVBO->update(offset, arcBufSize0, _vRotateAxis[0].data());
            offset += arcBufSize0;
            _axisVBO->update(offset, arcBufSize1, _vRotateAxis[1].data());
            offset += arcBufSize1;
            _axisVBO->update(offset, arcBufSize2, _vRotateAxis[2].data());

        }
        ///绘制轴
        void renderAxis(FEContext& context)
        {
            /// stub: 渲染部分暂未接入 FE 图形系统
            (void)context;
        }
        ///高亮轴
        FEEditAxisRotate::AXIS hoverAxis(FEContext& context, const int2& pos)
        {
            FEEditAxisRotate::AXIS oldHovered = _hoveredAxis;
            _hoveredAxis = FEEditAxisRotate::AXIS_NULL;
            ///首先计算是否与X,Y,Z轴对应的圆弧相交
            _hoveredAxis = pickAxis(context, pos);
            if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
            {
                if (oldHovered != _hoveredAxis)
                {
                    _d.sendHoveredDelegate();
                }
                return _hoveredAxis;
            }
            _hoveredAxis = pickBall(context, pos);
            if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
            {
                if (oldHovered != _hoveredAxis)
                {
                    _d.sendHoveredDelegate();
                }
                return _hoveredAxis;
            }
            _hoveredAxis = pickCricle(context, pos);
            if (_hoveredAxis != FEEditAxisRotate::AXIS_NULL)
            {
                if (oldHovered != _hoveredAxis)
                {
                    _d.sendHoveredDelegate();
                }
                return _hoveredAxis;
            }

            if (oldHovered != _hoveredAxis)
            {
                _d.sendHoveredDelegate();
            }
            return _hoveredAxis;
        }
        ///更新此次的旋转轴方向以及该旋转方向的权重
        void updateRotationAxisAndWeight(FEContext& context)
        {
            _rotationAxis = _d.axisX();
            _rotationAngleWeight = 0.0;
            switch (_selectedAxis)
            {
            case FE::FEEditAxisRotate::AXIS_X:
            {
                _rotationAxis = _d.axisX();
                _rotationAngleWeight = 1.0;
            }break;
            case FE::FEEditAxisRotate::AXIS_Y:
            {
                _rotationAxis = _d.axisY();
                _rotationAngleWeight = 1.0;
            }break;
            case FE::FEEditAxisRotate::AXIS_Z:
            {
                _rotationAxis = _d.axisZ();
                _rotationAngleWeight = 1.0;
            }break;
            case FE::FEEditAxisRotate::AXIS_SC:
            {
                _rotationAxis           =   normalize(context.activeCamera().getDir());
                _rotationAngleWeight    =   1.0;
            }break;
            default:
            {
                return ;
            }break;
            }
            ///加入轴禁用计算
            if (!_d.enabled(FEEditAxisRotate::AXIS_X))
            {
                _rotationAxis = ptProjToPlane(_rotationAxis, _d.axisX(), real3(0.0));
                _rotationAngleWeight = length(_rotationAxis);
                if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                    _rotationAngleWeight = 0.0;
                else
                    _rotationAxis = normalize(_rotationAxis);
            }
            if (!_d.enabled(FEEditAxisRotate::AXIS_Y))
            {
                _rotationAxis = ptProjToPlane(_rotationAxis, _d.axisY(), real3(0.0));
                _rotationAngleWeight = length(_rotationAxis);
                if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                    _rotationAngleWeight = 0.0;
                else
                    _rotationAxis = normalize(_rotationAxis);
            }
            if (!_d.enabled(FEEditAxisRotate::AXIS_Z))
            {
                _rotationAxis = ptProjToPlane(_rotationAxis, _d.axisZ(), real3(0.0));
                _rotationAngleWeight = length(_rotationAxis);
                if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                    _rotationAngleWeight = 0.0;
                else
                    _rotationAxis = normalize(_rotationAxis);
            }
        }
        ///旋转计算
        ///start,end 屏幕坐标开始，结束点
        ///axisAndWeight xyz表示此次旋转轴, w表示计算出的角度在该轴上的权重
        ///rAngle 计算的结果角度
        void calcRotate(FEContext& context, const int2& start, const int2& end, real& rAngle)
        {
            rAngle = 0.0;
            if (_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return;
            switch (_selectedAxis)
            {
            case FEEditAxisRotate::AXIS_X:
            {
                rAngle = this->rotateX(context, start, end, _rotationAxis) * _rotationAngleWeight;
            }
            break;
            case FEEditAxisRotate::AXIS_Y:
            {
                rAngle = this->rotateY(context, start, end, _rotationAxis) * _rotationAngleWeight;
            }
            break;
            case FEEditAxisRotate::AXIS_Z:
            {
                rAngle = this->rotateZ(context, start, end, _rotationAxis) * _rotationAngleWeight;
            }
            break;
            case FEEditAxisRotate::AXIS_SC:
            {
                rAngle = this->rotateSC(context, start, end, _rotationAxis) * _rotationAngleWeight;
            }
            break;
            default:
            {
                rAngle = 0.0;
            }break;
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
    private:
        void renderFan(FEContext& context)
        {
            /// stub: 渲染部分暂未接入 FE 图形系统
            (void)context;
        }
        void renderBallAndCircle(FEContext& context)
        {
            /// stub: 渲染部分暂未接入 FE 图形系统
            (void)context;
        }
        void renderAxisLine(FEContext& context)
        {
            /// stub: 渲染部分暂未接入 FE 图形系统
            (void)context;
        }
        void renderTangent(FEContext& context)
        {
            /// stub: 渲染部分暂未接入 FE 图形系统
            (void)context;
        }

        FEEditAxisRotate::AXIS pickAxis(FE::FEContext& context, const FE::int2& screen)
        {
            real3 mouse(screen.x, screen.y, 0);
            static const real DIST_MAX = 4;
            FECamera &camera  =   context.activeCamera();
            real        dist[3] =   { FLT_MAX, FLT_MAX, FLT_MAX };
            if (_vRotateAxis[0].size() == 0)
                return FEEditAxisRotate::AXIS_NULL;
            for (size_t index = 0; index < 3; ++index)
            {
                const ArrayFloat3& vs = _vRotateAxis[index];
                real& rDist = dist[index];
                int count = (int)(vs.size());
                int cenIndex = count / 2;
                bool bPickup = false;
                ///先拾取右侧(中心点到右侧所有点)
                for (int i = cenIndex; i < count - 1; ++i)
                {
                    real2 startX = camera.worldToScreen(real3(vs[i]) + _d.position());
                    real2 endX = camera.worldToScreen(real3(vs[i + 1]) + _d.position());
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
                    real2 startX = camera.worldToScreen(real3(vs[i]) + _d.position());
                    real2 endX = camera.worldToScreen(real3(vs[i - 1]) + _d.position());
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
                if (_d.enabled(FEEditAxisRotate::AXIS_X))
                    return  FEEditAxisRotate::AXIS_X;
                else
                    return  FEEditAxisRotate::AXIS_NULL;
            }
            if (min == 1)
            {
                if (_d.enabled(FEEditAxisRotate::AXIS_Y))
                    return  FEEditAxisRotate::AXIS_Y;
                else
                    return  FEEditAxisRotate::AXIS_NULL;
            }
            if (min == 2)
            {
                if (_d.enabled(FEEditAxisRotate::AXIS_Z))
                    return  FEEditAxisRotate::AXIS_Z;
                else
                    return  FEEditAxisRotate::AXIS_NULL;
            }
            return  FEEditAxisRotate::AXIS_NULL;
        }
        FEEditAxisRotate::AXIS pickBall(FE::FEContext& context, const FE::int2& screen)
        {
            return FEEditAxisRotate::AXIS_NULL;
        }
        FEEditAxisRotate::AXIS pickCricle(FE::FEContext& context, const FE::int2& screen)
        {
            static const real DIST_MAX = 4;
            FECamera &camera = context.activeCamera();
            real3       mouse(screen.x, screen.y, 0);
            real        dis = FLT_MAX;
            real        dist[3] = { 0,0,0 };
            for (int i = 0; i < 30; ++i)
            {
                real2 startX = camera.worldToScreen(real3(_rotateScreenCricle[31 + i]) + _d.position());
                real2 endX = camera.worldToScreen(real3(_rotateScreenCricle[31 + i + 1]) + _d.position());
                dist[0] = closeDistanceOnLine(real3(startX.x, startX.y, 0), real3(endX.x, endX.y, 0), mouse);
                dist[0] = std::min(dist[0], dis);
                if (dist[0] < DIST_MAX && (_d.enabled(FEEditAxisRotate::AXIS_X)
                    || _d.enabled(FEEditAxisRotate::AXIS_Y)
                    || _d.enabled(FEEditAxisRotate::AXIS_Z)))
                {
                    return FEEditAxisRotate::AXIS_SC;
                }
            }
            return FEEditAxisRotate::AXIS_NULL;
        }

        real rotateX(FEContext& context, const FE::int2& start, const FE::int2& end, const real3& axis)
        {
            FECamera& camera = context.activeCamera();
            real3       vtStart;
            real3       vtEnd;
            calcTangentAndFan(context, axis, _radiusBall, vtStart, vtEnd);
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
        real rotateY(FEContext& context, const FE::int2& start, const FE::int2& end, const real3& axis)
        {
            FECamera& camera = context.activeCamera();
            real3       vtStart;
            real3       vtEnd;
            calcTangentAndFan(context, axis, _radiusBall, vtStart, vtEnd);
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
        real rotateZ(FEContext& context, const FE::int2& start, const FE::int2& end, const real3& axis)
        {
            FECamera& camera  = context.activeCamera();
            real3       vtStart;
            real3       vtEnd;
            calcTangentAndFan(context, axis, _radiusBall, vtStart, vtEnd);
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
                ///real lenV   = length(retPtEnd - retPtStart);
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
        real rotateSC(FEContext& context, const FE::int2& start, const FE::int2& end, const real3& axis)
        {
            FECamera& camera  =   context.activeCamera();
            real3       vtStart;
            real3       vtEnd;
            calcTangentAndFan(context, axis, _radiusCircle, vtStart, vtEnd);
            if (vtStart == vtEnd)
                return _offAngle;
            Ray     rayStart    =   camera.createRayFromScreen(start.x, start.y);
            Ray     rayEnd      =   camera.createRayFromScreen(end.x, end.y);
            real3   retPtStart;
            real3   retPtEnd;
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

        void calcTangentAndFan(FEContext& context, const real3 &rotateAxis, real &radius, real3& vtStart, real3 &vtEnd)
        {
            FE::FECamera&   camera      =   context.activeCamera();
            Ray                 rayStart    =   camera.createRayFromScreen(_startPos.x, _startPos.y);
            real3               tangentPos;   ///切点
            real3               tangentNorV;  ///切线方向向量    
            real3               retPt;
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

            uint tangentBufSize =   8 * sizeof(float3);
            uint fanBufSize     =   (uint)_fan.size() * sizeof(float3);
            uint bufSize = tangentBufSize + fanBufSize;

            /// stub: FEDevice 暂未提供 createVertexBuffer 接口，使用占位对象
            if (!_fanTangentVBO)
            {
                _fanTangentVBO = std::make_shared<VertexBuffer>();
            }

            _fanTangentVBO->update(0, tangentBufSize, &_tangent[0]);
            _fanTangentVBO->update(tangentBufSize, fanBufSize, _fan.data());

            ///计算完成，可以开始绘制
            _bRotatting = true;
        }
        
    };

    FEEditAxisRotate::FEEditAxisRotate(FEContext& context) :FEEditAxis(EditAxisTypeRotate, context)
    {
        _p = new FEEditAxisRotatePrivate(*this);
    }

    FEEditAxisRotate::~FEEditAxisRotate()
    {
        delete _p;
    }

    FEEditAxisRotate::MDelegate& FEEditAxisRotate::mDelegate()
    {
        return _p->_delegate;
    }

    FEEditAxisRotate::AXIS FEEditAxisRotate::hoveredAxis() const
    {
        return _p->_hoveredAxis;
    }
    FEEditAxisRotate::AXIS FEEditAxisRotate::selectedAxis() const
    {
        return _p->_selectedAxis;
    }
    
    void FEEditAxisRotate::setAdsorptionEnabled(bool enabled)
    {
        _p->_adsorptionEnabled = enabled;
    }
    bool FEEditAxisRotate::adsorptionEnabled() const
    {
        return _p->_adsorptionEnabled;
    }
    FEEditAxisRotate::AdsorptionFlags FEEditAxisRotate::adsorptionFlags() const
    {
        return _p->_adsorptionFlags;
    }
    void FEEditAxisRotate::setAdsorptionFlags(AdsorptionFlags flags)
    {
        _p->_adsorptionFlags = flags;
    }
    real FEEditAxisRotate::adsorptionAngle() const
    {
        return _p->_adsorptionAngle;
    }
    void FEEditAxisRotate::setAdsorptionAngle(real angle)
    {
        _p->_adsorptionAngle = angle;
    }

    void FEEditAxisRotate::cancelHovered()
    {
        AXIS oldHovered = _p->_hoveredAxis;
        _p->_hoveredAxis = FEEditAxisRotate::AXIS::AXIS_NULL;
        if (oldHovered != _p->_hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditAxisRotate::cancelSelected()
    {
        AXIS oldSelected = _p->_selectedAxis;
        _p->_selectedAxis = FEEditAxisRotate::AXIS::AXIS_NULL;
        if (oldSelected != _p->_selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditAxisRotate::mouseButtonPress(FEContext& context, const int2& pos)
    {
        _p->_bMouseDown = true;

        AXIS oldSelected = _p->_selectedAxis;
        _p->_selectedAxis = _p->_hoveredAxis;
        if (oldSelected != _p->_selectedAxis)
        {
            this->sendSelectedDelegate();
        }
        if (isAxisSelected())
        {
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            ///更新计算此次的旋转轴以及其角度权重
            _p->updateRotationAxisAndWeight(context);
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _p->_delegate(EditStatus::EditStart, _p->_rotationAxis, 0.0, 0.0, *this);
            return true;
        }
        return false;
    }
    bool FEEditAxisRotate::mouseButtonRelease(FEContext& context, const int2& pos)
    {
        _p->_bMouseDown = false;
        if (isAxisSelected())
        {
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;

            _p->_delegate(EditStatus::EditEnd, _p->_rotationAxis, 0.0, _p->_offAngle, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;

            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = AXIS_NULL;
            if (oldSelected != _p->_selectedAxis)
            {
                this->sendSelectedDelegate();
            }
            return true;
        }
        return false;
    }
    bool FEEditAxisRotate::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            real angle = 0.0;
            real oldAngle = _p->_offAngle;
            _p->calcRotate(context, _p->_startPos, pos, angle);
            angle = angle - oldAngle;
            _p->_delegate(EditStatus::Editting, _p->_rotationAxis, angle, _p->_offAngle, *this);
            _p->_downPos = pos;
            _context.requireNextFrame();
            return true;
        }
        else if (!_p->_bMouseDown)
        {
            if (_p->hoverAxis(context, pos) != AXIS_NULL)
            {
            }
            else
            {
            }
        }
        return false;
    }

    bool FEEditAxisRotate::touchDown(FEContext& context, const int2& pos)
    {
        _p->_bTouchDown = true;
        if (this->isAxisSelected())
        {
            _p->_bTouchPickup = false;
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            ///更新计算此次的旋转轴以及其角度权重
            _p->updateRotationAxisAndWeight(context);
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _p->_delegate(EditStatus::EditStart, _p->_rotationAxis, 0.0, 0.0, *this);
            return true;
        }
        else
        {
            _p->_bTouchPickup = true;
            return false;
        }
        return false;

    }
    bool FEEditAxisRotate::touchUp(FEContext& context, const int2& pos)
    {
        _p->_bTouchDown = false;
        if (_p->_bTouchPickup)
        {///拾取动作
            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = _p->hoverAxis(context, pos);
            if (oldSelected != _p->_selectedAxis)
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
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            _p->_delegate(EditStatus::EditEnd, _p->_rotationAxis, 0.0, _p->_offAngle, *this);
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offAngle = 0.0;
            _p->_bRotatting = false;
            _p->_bTouchPickup = false;

            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = AXIS::AXIS_NULL;
            if (oldSelected != _p->_selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            AXIS oldHovered = _p->_hoveredAxis;
            _p->_hoveredAxis = AXIS::AXIS_NULL;
            if (oldHovered != _p->_hoveredAxis)
            {
                this->sendHoveredDelegate();
            }
            return true;
        }
        return false;

    }
    bool FEEditAxisRotate::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            if (_p->_rotationAngleWeight <= std::numeric_limits<float>::epsilon())
                return true;
            real3 axis = AxisX();
            real angle = 0.0;
            real oldAngle = _p->_offAngle;
            _p->calcRotate(context, _p->_startPos, pos, angle);
            angle = angle - oldAngle;
            _p->_delegate(EditStatus::Editting, _p->_rotationAxis, angle, _p->_offAngle, *this);
            _p->_downPos = pos;
            _context.requireNextFrame();
            return true;
        }
        return false;
    }

    void FEEditAxisRotate::update(FEContext& context)
    {
        _p->updateAxisVerties(context);
    }
    void FEEditAxisRotate::render(FEContext& context)
    {
        if (!_internalFlags.hasFlag(InteralFlag_Visible))
            return;
        _p->renderAxis(context);
    }
}

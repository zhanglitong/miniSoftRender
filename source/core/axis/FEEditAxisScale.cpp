#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditAxisScale.h"

namespace   FE
{
    ///计算射线是否与某个四边形面有交点
    extern bool GetRayRectaceInsPt(const Ray &ray, FE::real3 rect[4], FE::real3 &retPt);
    class FEEditAxisScalePrivate
    {
    private:
        struct PushBlock
        {
            mat4r     _mvp;
            float4      _color;
        };
    public:
        ///
        FEEditAxisScale& _d;
        ///缩放轴
        FE::real3 _scaleAxis[16];
        ///箭头
        FE::real3 _scaleAxisArr[15];
        ///轴线VBO
        VertexBufPtr    _axisVBO    =   nullptr;
        ///箭头VBO
        VertexBufPtr    _axisArrVBO =   nullptr;
        ///绘制管线
        GraphicPLPtr    _pipeLine;
        ///材质
        MaterialPtr     _material;
        ///轴线IBO
        IndexBufPtr     _axisIBO    =   nullptr;
        ///箭头IBO
        IndexBufPtr     _axisArrIBO    =   nullptr;
        ///当前选中轴
        FEEditAxisScale::AXIS _selectedAxis;
        ///当前高亮的轴
        FEEditAxisScale::AXIS _hoveredAxis;

        ///是否正在进行缩放
        bool _bScaleing;
        ///缩放偏移
        real3 _offScale;

        ///保持把坐标轴的屁股不朝向屏幕
        real3 _dir;

        ///轴长度
        real _axisSize;

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

        ///回调
        FEEditAxisScale::MDelegate _delegate;
    public:
        FEEditAxisScalePrivate(FEEditAxisScale& d) :_d(d)
        {
            _selectedAxis = FEEditAxisScale::AXIS_NULL;
            _hoveredAxis = FEEditAxisScale::AXIS_NULL;
            _bScaleing = false;
            _offScale = real3(1.0, 1.0, 1.0);
            _dir = real3(1.0, 1.0, 1.0);
            _axisSize = 0.0;
            _bMouseDown = false;

            /// stub: rendering not yet integrated with FE graphics system
            _pipeLine = nullptr;
            _material = nullptr;
            _axisIBO = nullptr;
            _axisArrIBO = nullptr;
        }
    public: 
        ///更新轴顶点
        void updateAxisVerties(FEContext& context)
        {
            FECamera& camera = context.activeCamera();
            ///计算轴朝向
            _dir = calcAxisDir(camera);
            ///计算顶点
            real unitF = camera.pixelU(_d.position());
            real size = (real)(unitF * 80);
            real triMinSize = size * 0.5;
            real triMaxSize = size * 3.0  * 0.25;
            _axisSize = size;

            _scaleAxis[0] = real3(0);

            real3 axisScale = _offScale;

            _scaleAxis[1] = _dir.x *  (_d.axisX() * triMinSize) * axisScale.x;		//min x
            _scaleAxis[3] = _dir.x *  (_d.axisX() * triMaxSize) * axisScale.x;		//max x
            _scaleAxis[5] = _dir.x *  (_d.axisX() * size) * axisScale.x;			//x

            _scaleAxis[6] = _dir.y *  (_d.axisY() * triMinSize) * axisScale.y;		//min y
            _scaleAxis[8] = _dir.y *  (_d.axisY() * triMaxSize) * axisScale.y;		//max y
            _scaleAxis[10] = _dir.y * (_d.axisY() * size) * axisScale.y;			//y

            _scaleAxis[11] = _dir.z * (_d.axisZ() * triMinSize) * axisScale.z;		//min z
            _scaleAxis[13] = _dir.z * (_d.axisZ() * triMaxSize) *axisScale.z;		//max z
            _scaleAxis[15] = _dir.z * (_d.axisZ() * size) * axisScale.z;			//z

            _scaleAxis[2] = (_scaleAxis[1] + _scaleAxis[6]) * 0.5;				//min xy
            _scaleAxis[4] = (_scaleAxis[3] + _scaleAxis[8]) * 0.5;				//max xy

            _scaleAxis[7] = (_scaleAxis[6] + _scaleAxis[11]) * 0.5;				//min yz
            _scaleAxis[9] = (_scaleAxis[8] + _scaleAxis[13]) * 0.5;				//max yz

            _scaleAxis[12] = (_scaleAxis[11] + _scaleAxis[1]) * 0.5;			//min zx
            _scaleAxis[14] = (_scaleAxis[13] + _scaleAxis[3]) * 0.5;			//max zx

            float3 tmpScaleAxis[16];
            for (int i = 0; i < 16; ++i)
            {
                tmpScaleAxis[i] = _scaleAxis[i];
            }

            uint bufSize    =   16 * sizeof(float3);
            /// stub: rendering not yet integrated with FE graphics system
            if (!_axisVBO)
            {
                _axisVBO = std::make_shared<VertexBuffer>();
            }
            _axisVBO->update(0, bufSize, &tmpScaleAxis[0]);

            real3 dir = camera.getDir();
            real3 right = camera.getRight();
            real3 up = camera.getUp();
            real arSize = size * 0.035;
            ///计算箭头
            ///x
            _scaleAxisArr[0] = _scaleAxis[5];
            _scaleAxisArr[1] = _scaleAxisArr[0] + (right + up) * arSize;
            _scaleAxisArr[2] = _scaleAxisArr[0] + (right - up) * arSize;
            _scaleAxisArr[3] = _scaleAxisArr[0] + (-right - up) * arSize;
            _scaleAxisArr[4] = _scaleAxisArr[0] + (-right + up) * arSize;
            ///y
            _scaleAxisArr[5] = _scaleAxis[10];
            _scaleAxisArr[6] = _scaleAxisArr[5] + (right + up) * arSize;
            _scaleAxisArr[7] = _scaleAxisArr[5] + (right - up) * arSize;
            _scaleAxisArr[8] = _scaleAxisArr[5] + (-right - up) * arSize;
            _scaleAxisArr[9] = _scaleAxisArr[5] + (-right + up) * arSize;
            ///z
            _scaleAxisArr[10] = _scaleAxis[15];
            _scaleAxisArr[11] = _scaleAxisArr[10] + (right + up) * arSize;
            _scaleAxisArr[12] = _scaleAxisArr[10] + (right - up) * arSize;
            _scaleAxisArr[13] = _scaleAxisArr[10] + (-right - up) * arSize;
            _scaleAxisArr[14] = _scaleAxisArr[10] + (-right + up) * arSize;

            float3 tmpAr[15];
            for (int i = 0; i < 15; ++i)
            {
                tmpAr[i] = _scaleAxisArr[i];
            }
            uint arrBufSize    =   15 * sizeof(float3);
            /// stub: rendering not yet integrated with FE graphics system
            if (!_axisArrVBO)
            {
                _axisArrVBO = std::make_shared<VertexBuffer>();
            }
            _axisArrVBO->update(0, arrBufSize, &tmpAr[0]);
        }
        ///绘制轴
        void renderAxis(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
        }
        ///缩放计算
        real3 calcScale(FEContext& context, const int2& start, const int2& end)
        {
            switch (_selectedAxis)
            {
            case FEEditAxisScale::AXIS_NULL:
                break;
            case FEEditAxisScale::AXIS_X:
            {
                real retK = scaleX(context, start, end);
                _offScale = real3(retK, 1.0, 1.0);
            }
            break;
            case FEEditAxisScale::AXIS_Y:
            {
                real retK = scaleY(context, start, end);
                _offScale = real3(1.0, retK, 1.0);
            }
            break;
            case FEEditAxisScale::AXIS_Z:
            {
                real retK = scaleZ(context, start, end);
                _offScale = real3(1.0, 1.0, retK);
            }
            break;
            case FEEditAxisScale::AXIS_XY:
            {
                real retK = scaleXY(context, start, end);
                _offScale = real3(retK, retK, 1.0);
                if (!_d.enabled(FEEditAxisScale::AXIS_X))
                    _offScale.x = 1.0;
                if (!_d.enabled(FEEditAxisScale::AXIS_Y))
                    _offScale.y = 1.0;
            }
            break;
            case FEEditAxisScale::AXIS_YZ:
            {
                real retK = scaleYZ(context, start, end);
                _offScale = real3(1.0, retK, retK);
                if (!_d.enabled(FEEditAxisScale::AXIS_Y))
                    _offScale.y = 1.0;
                if (!_d.enabled(FEEditAxisScale::AXIS_Z))
                    _offScale.z = 1.0;
            }
            break;
            case FEEditAxisScale::AXIS_XZ:
            {
                real retK = scaleXZ(context, start, end);
                _offScale = real3(retK, 1.0, retK);
                if (!_d.enabled(FEEditAxisScale::AXIS_X))
                    _offScale.x = 1.0;
                if (!_d.enabled(FEEditAxisScale::AXIS_Z))
                    _offScale.z = 1.0;
            }
            break;
            case FEEditAxisScale::AXIS_XYZ:
            {
                real retK = scaleXYZ(context, start, end);
                _offScale = real3(retK, retK, retK);
                if (!_d.enabled(FEEditAxisScale::AXIS_X))
                    _offScale.x = 1.0;
                if (!_d.enabled(FEEditAxisScale::AXIS_Y))
                    _offScale.y = 1.0;
                if (!_d.enabled(FEEditAxisScale::AXIS_Z))
                    _offScale.z = 1.0;
            }
            break;
            default:
                break;
            }
            return _offScale;
        }
        ///高亮轴
        FEEditAxisScale::AXIS hoverAxis(FEContext& context, const int2& pos)
        {
            FEEditAxisScale::AXIS oldHovered = _hoveredAxis;
            _hoveredAxis = pickAxis(context, pos);
            if (_hoveredAxis != FEEditAxisScale::AXIS_NULL)
            {
                if (oldHovered != _hoveredAxis)
                {
                    _d.sendHoveredDelegate();
                }
                return _hoveredAxis;
            }
            _hoveredAxis = pickTriMax(context, pos);
            if (_hoveredAxis != FEEditAxisScale::AXIS_NULL)
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
            _hoveredAxis = pickTriMin(context, pos);
            return _hoveredAxis;
        }
    private:
        real3 calcAxisDir(FECamera& camera)
        {
            real3 dir = normalize(_d.position() - camera.getEye());
            real3 axis[3] = { _d.axisX(), _d.axisY(), _d.axisZ() };
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
        void renderAxisP(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
        }
        void renderAr(FEContext& context)
        {
            /// stub: rendering not yet integrated with FE graphics system
            (void)context;
        }

        FEEditAxisScale::AXIS pickTriMin(FEContext& context, const FE::int2& screen)
        {
            FECamera& camera  =   context.activeCamera();
            Ray         ray     =   camera.createRayFromScreen(screen.x, screen.y);
            real t, u, v;
            real3 tmpPos0 = _scaleAxis[0] + _d.position();
            real3 tmpPos1 = _scaleAxis[1] + _d.position();
            real3 tmpPos6 = _scaleAxis[6] + _d.position();
            real3 tmpPos11 = _scaleAxis[11] + _d.position();
            /// 1, 6, 11,   0, 1, 6,   0 , 6, 11,   0 ,11, 1
            if (intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos1, tmpPos6, tmpPos11, &t, &u, &v)
                || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos1, tmpPos6, &t, &u, &v)
                || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos6, tmpPos11, &t, &u, &v)
                || intersectTriangle(ray.getOrigin(), ray.getDirection(), tmpPos0, tmpPos11, tmpPos1, &t, &u, &v)
                )
            {
                
                if (_d.enabled(FEEditAxisScale::AXIS_X)
                    || _d.enabled(FEEditAxisScale::AXIS_Y)
                    || _d.enabled(FEEditAxisScale::AXIS_Z))
                    return FEEditAxisScale::AXIS_XYZ;
            }
            return FEEditAxisScale::AXIS_NULL;
        }
        FEEditAxisScale::AXIS pickTriMax(FEContext& context, const FE::int2& screen)
        {
            FECamera &camera  =   context.activeCamera();
            Ray         ray     =   camera.createRayFromScreen(screen.x, screen.y);

            real3 quadXY[4] =
            {
                _scaleAxis[1] + _d.position(), _scaleAxis[3] + _d.position(),
                _scaleAxis[8] + _d.position(), _scaleAxis[6] + _d.position(),
            };
            real3 quadYZ[4] =
            {
                _scaleAxis[6] + _d.position(), _scaleAxis[8] + _d.position(),
                _scaleAxis[13] + _d.position(), _scaleAxis[11] + _d.position(),
            };
            real3 quadXZ[4] =
            {
                _scaleAxis[11] + _d.position(), _scaleAxis[13] + _d.position(),
                _scaleAxis[3] + _d.position(), _scaleAxis[1] + _d.position(),
            };
            real3 retPt;
            if (GetRayRectaceInsPt(ray, quadXY, retPt))
            {
                if (_d.enabled(FEEditAxisScale::AXIS_X)
                    || _d.enabled(FEEditAxisScale::AXIS_Y))
                    return FEEditAxisScale::AXIS_XY;
            }
            if (GetRayRectaceInsPt(ray, quadYZ, retPt))
            {
                if (_d.enabled(FEEditAxisScale::AXIS_Y)
                    || _d.enabled(FEEditAxisScale::AXIS_Z))
                    return FEEditAxisScale::AXIS_YZ;
            }
            if (GetRayRectaceInsPt(ray, quadXZ, retPt))
            {
                if (_d.enabled(FEEditAxisScale::AXIS_X)
                    || _d.enabled(FEEditAxisScale::AXIS_Z))
                    return FEEditAxisScale::AXIS_XZ;
            }
            return FEEditAxisScale::AXIS_NULL;
        }
        FEEditAxisScale::AXIS pickAxis(FEContext& context, const FE::int2& screen)
        {
            static const real DIST_MAX = 6;
            FECamera &camera = context.activeCamera();
            real3       mouse(screen.x, screen.y, 0);
            real2       startX = camera.worldToScreen(_scaleAxis[1] + _d.position());
            real2       startY = camera.worldToScreen(_scaleAxis[6] + _d.position());
            real2       startZ = camera.worldToScreen(_scaleAxis[11] + _d.position());
            ///轴数据
            real2       xAxis = camera.worldToScreen(_scaleAxis[5] + _d.position());
            real2       yAxis = camera.worldToScreen(_scaleAxis[10] + _d.position());
            real2       zAxis = camera.worldToScreen(_scaleAxis[15] + _d.position());

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
                if (_d.enabled(FEEditAxisScale::AXIS_X))
                    return FEEditAxisScale::AXIS_X;
                else
                    return FEEditAxisScale::AXIS_NULL;
            }
            if (min == 1)
            {
                if (_d.enabled(FEEditAxisScale::AXIS_Y))
                    return FEEditAxisScale::AXIS_Y;
                else
                    return FEEditAxisScale::AXIS_NULL;
            }
            if (min == 2)
            {
                if (_d.enabled(FEEditAxisScale::AXIS_Z))
                    return FEEditAxisScale::AXIS_Z;
                else
                    return FEEditAxisScale::AXIS_NULL;
            }
            return FEEditAxisScale::AXIS_NULL;
        }

        real scaleX(FEContext& context, const int2& start, const int2& end)
        {
            real3       axis    =   _d.axisX() * _dir.x;

            FECamera& camera  =   context.activeCamera();
            real3       faceNor =   camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offScale.x;

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.x;
        }
        real scaleY(FEContext& context, const int2& start, const int2& end)
        {
            real3       axis    =   _d.axisY() * _dir.y;
            FECamera& camera  =   context.activeCamera();
            real3       faceNor =   camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offScale.y;

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.y;
        }
        real scaleZ(FEContext& context, const int2& start, const int2& end)
        {
            real3       axis    =   _d.axisZ() * _dir.z;

            FECamera& camera  =   context.activeCamera();
            real3       faceNor =   camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offScale.z;

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.z;
        }
        real scaleXY(FEContext& context, const int2& start, const int2& end)
        {
            real3 axis = normalize(_d.axisX() * _dir.x + _d.axisY() * _dir.y);

            real3 faceNor = normalize(cross(_d.axisX(), _d.axisY()));

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.x;
        }
        real scaleYZ(FEContext& context, const int2 & start, const int2 &end)
        {
            real3 axis = normalize(_d.axisY() * _dir.y + _d.axisZ() * _dir.z);

            real3 faceNor = normalize(cross(_d.axisY(), _d.axisZ()));

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.y;
        }
        real scaleXZ(FEContext& context, const int2 & start, const int2 &end)
        {
            real3 axis = normalize(_d.axisX() * _dir.x + _d.axisZ() * _dir.z);

            real3 faceNor = normalize(cross(_d.axisX(), _d.axisZ()));

            real rets(1.0);
            if (calcScaleValue(context, start, end, faceNor, axis, rets, 1.0))
            {
                return rets;
            }
            return _offScale.z;
        }
        real scaleXYZ(FEContext& context, const int2 & start, const int2 &end)
        {
            FECamera& camera  =   context.activeCamera();
            real3       axis    =   camera.getUp();
            real3       faceNor =   camera.getDir();

            real        retS(1.0);
            Ray         rayStart=   camera.createRayFromScreen(start.x, start.y);
            Ray         rayEnd  =   camera.createRayFromScreen(end.x, end.y);
            ///这里将中心点向下移动，目的是降低缩放的灵敏度,防止缩放时跳动太快的访问
            real3 cenPos = _d.position() - axis * _axisSize;
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

        bool calcScaleValue(FEContext& context
            , const int2& start, const int2& end
            , const real3& faceNor, const real3& axis
            , real& retS, real limit = 1.0)
        {
            retS = 1.0;
            FECamera& camera      =   context.activeCamera();
            Ray         rayStart    =   camera.createRayFromScreen(start.x, start.y);
            Ray         rayEnd      =   camera.createRayFromScreen(end.x, end.y);
            real3       retPt0(0);
            real3       retPt1(0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) &&
                calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                real3 pt0 = FE::closePointOnVector(axis, _d.position(), retPt0);
                real3 pt1 = FE::closePointOnVector(axis, _d.position(), retPt1);
                real3 nor = normalize(pt1 - pt0);
                ///确定当前点在鼠标按下点的那一侧
                real  d = dot(nor, axis);
                real dis0 = distance(_d.position(), pt0);
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
    };

    FEEditAxisScale::FEEditAxisScale(FEContext& context)
        :FEEditAxis(EditAxisType::EditAxisTypeScale, context)
    {
        _p = new FEEditAxisScalePrivate(*this);
    }
    FEEditAxisScale::~FEEditAxisScale()
    {
        delete _p;
    }

    FEEditAxisScale::MDelegate& FEEditAxisScale::mDelegate()
    {
        return _p->_delegate;
    }

    FEEditAxisScale::AXIS FEEditAxisScale::hoveredAxis() const
    {
        return _p->_hoveredAxis;
    }
    FEEditAxisScale::AXIS FEEditAxisScale::selectedAxis() const
    {
        return _p->_selectedAxis;
    }
    
    void FEEditAxisScale::cancelHovered()
    {
        AXIS oldHovered = _p->_hoveredAxis;
        _p->_hoveredAxis = FEEditAxisScale::AXIS::AXIS_NULL;
        if (oldHovered != _p->_hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditAxisScale::cancelSelected()
    {
        AXIS oldSelected = _p->_selectedAxis;
        _p->_selectedAxis = FEEditAxisScale::AXIS::AXIS_NULL;
        if (oldSelected != _p->_selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditAxisScale::mouseButtonPress(FEContext& , const int2& pos)
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
            _p->_offScale = real3(1.0, 1.0, 1.0);
            _p->_bScaleing = true;
            _p->_delegate(EditStatus::EditStart, real3(1.0), real3(1.0), *this);
            return true;
        }
        return false;
    }
    bool FEEditAxisScale::mouseButtonRelease(FEContext& , const int2& pos)
    {
        _p->_bMouseDown = false;
        if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, real3(1.0), _p->_offScale, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offScale = real3(1.0, 1.0, 1.0);

            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = AXIS_NULL;
            if (oldSelected != _p->_selectedAxis)
            {
                this->sendSelectedDelegate();
            }

            _p->_bScaleing = false;
            return true;
        }
        return false;
    }
    bool FEEditAxisScale::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 prevScale = _p->_offScale;
            real3 scale = _p->calcScale(context, _p->_startPos, pos);
            scale = scale / prevScale;
            _p->_delegate(EditStatus::Editting, scale,_p->_offScale, *this);
            _p->_downPos = pos;
            _ctx.requireNextFrame();
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

    bool FEEditAxisScale::touchDown(FEContext& , const int2& pos)
    {
        _p->_bTouchDown = true;
        if (this->isAxisSelected())
        {
            _p->_bTouchPickup = false;
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offScale = real3(1.0, 1.0, 1.0);
            _p->_bScaleing = true;
            _p->_delegate(EditStatus::EditStart, real3(1.0), real3(1.0), *this);
            return true;
        }
        else
        {
            _p->_bTouchPickup = true;
            return false;
        }
        return false;

    }
    bool FEEditAxisScale::touchUp(FEContext& context, const int2& pos)
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
            _p->_delegate(EditStatus::EditEnd, real3(1.0), _p->_offScale, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;

            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = AXIS_NULL;
            if (oldSelected != _p->_selectedAxis)
            {
                this->sendSelectedDelegate();
            }
            
            AXIS oldHovered = _p->_hoveredAxis;
            _p->_hoveredAxis = AXIS_NULL;
            if (oldHovered != _p->_hoveredAxis)
            {
                this->sendHoveredDelegate();
            }


            _p->_offScale = real3(1.0, 1.0, 1.0);
            _p->_bScaleing = false;
            return true;
        }
        return false;

    }
    bool FEEditAxisScale::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 prevScale = _p->_offScale;
            real3 scale = _p->calcScale(context, _p->_startPos, pos);
            scale = scale / prevScale;
            _p->_delegate(EditStatus::Editting, scale, _p->_offScale, *this);
            _p->_downPos = pos;
            _ctx.requireNextFrame();
            return true;
        }
        return false;
    }
}

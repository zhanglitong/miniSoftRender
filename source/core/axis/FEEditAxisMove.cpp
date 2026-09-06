#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditAxisMove.h"


namespace   FE
{
    ///计算射线是否与某个四边形面有交点
    bool GetRayRectaceInsPt(const Ray &ray, FE::real3 rect[4], FE::real3 &retPt)
    {
        FE::real t, u, v;
        if (FE::intersectTriangle<FE::real>(ray.getOrigin(), ray.getDirection(), rect[0], rect[1], rect[2], &t, &u, &v))
        {
            FE::real3 tmpPt = ray.getPoint(t);
            if (FE::pointinTriangle(rect[0], rect[1], rect[2], tmpPt))
            {
                retPt = tmpPt;
                return true;
            }
        }
        FE::real t1, u1, v1;
        if (FE::intersectTriangle<FE::real>(ray.getOrigin(), ray.getDirection(), rect[2], rect[3], rect[0], &t1, &u1, &v1))
        {
            FE::real3 tmpPt = ray.getPoint(t1);
            if (FE::pointinTriangle(rect[2], rect[3], rect[0], tmpPt))
            {
                retPt = tmpPt;
                return true;
            }
        }
        return false;
    }
    
    struct AdsorptionResult
    {
        mat4r mat;
        real3s pts;
        int ptIndex;
        AdsorptionResult() 
        {
            mat = mat4r(1.0);
            ptIndex = -1;
        }
        void reset()
        {
            mat = mat4r(1.0);
            pts.clear();
            ptIndex = -1;
        }
        bool isVaild() const
        {
            return (!pts.empty()) && (ptIndex >= 0);
        }
        real3 ptWorld() const 
        {
            if (!isVaild())
                return real3(0.0);
            return real3(mat * real4(pts[ptIndex], 1.0));
        }
    };

    using AdsorptionResults = std::vector<AdsorptionResult>;

    ///计算几何体顶点吸附
    bool CalcMoveAdsorptionPoint(FEContext* app, const int2& screenPos, real pixel, AdsorptionResult& outRet)
    {
        (void)app; (void)screenPos; (void)pixel;
        outRet.reset();
        return false;
    }
    ///计算节点位置吸附
    bool CalcMoveAdsorptionPosition(FEContext* app, const int2& screenPos, real pixel, AdsorptionResult& outRet)
    {
        (void)app; (void)screenPos; (void)pixel;
        outRet.reset();
        return false;
    }
    ///计算节点包围盒吸附
    bool CalcMoveAdsorptionAabb(FEContext* app, const int2& screenPos, real pixel, AdsorptionResult& outRet)
    {
        (void)app; (void)screenPos; (void)pixel;
        outRet.reset();
        return false;
    }
    
    class FEEditAxisMovePrivate
    {
    private:
        struct PushBlock
        {
            mat4r     _mvp;
            float4      _color;
        };
    public:
        ///
        FEEditAxisMove& _d;
        ///轴顶点
        std::vector<float3> _moveAxis;
        ///箭头顶点
        std::vector<float3> _axisAr;

        VertexBufPtr    _axisLineVBO    =   nullptr;
        VertexBufPtr    _axisArrowVBO   =   nullptr;
        IndexBufPtr     _axisLineIBO    =   nullptr;
        std::vector<uint>   _lineindex;

        ///高亮的轴
        FEEditAxisMove::AXIS _hoveredAxis;
        ///选择的轴
        FEEditAxisMove::AXIS _selectedAxis;

        ///
        real3 _offMove;

        ///开始移动时轴的位置
        real3 _downPosWorld;

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
        
        ///移动吸附启用标志
        bool _adsorptionEnabled;
        ///移动吸附标志
        FEEditAxisMove::AdsorptionFlags _adsorptionFlags;
        ///移动点吸附像素范围
        real _adsorptionPixel;
        ///移动吸附计算结果
        AdsorptionResult _adsorptionRet;

        ///移动通知
        FEEditAxisMove::MDelegate _delegate;
        ///绘制管线
        GraphicPLPtr    _pipeLine;
        ///材质
        MaterialPtr     _material;

        bool          _bThreeClipEdit;
        int           _threeClipSize;
    public:
        FEEditAxisMovePrivate(FEEditAxisMove& d) :_d(d)
        {
            _downPosWorld = real3(0.0);

            _bMouseDown = false;
            _downPos = int2(0);
            _startPos = int2(0);

            _bTouchDown = false;
            _bTouchPickup = false;
            _touchDownPos = int2(0);
            _touchStartPos = int2(0);

            _offMove = real3(0.0);
            _hoveredAxis = FEEditAxisMove::AXIS::AXIS_NULL;
            _selectedAxis = FEEditAxisMove::AXIS::AXIS_NULL;

            _adsorptionEnabled = false;
            _adsorptionFlags = FEEditAxisMove::AdsorptionFlag::AF_Points;
            _adsorptionPixel = 10.0;

            _bThreeClipEdit = false;
            ///设置配置项并绑定响应事件

            _threeClipSize = 240;
            _pipeLine = nullptr;
            _material = nullptr;
        }
    public:
        ///更新轴顶点
        void updateAxisVerties(FEContext& context)
        {
            uint        pixels  =   80;
            uint        planePix=   40;
            if(_bThreeClipEdit)
            {
                pixels      =   _threeClipSize;
                planePix    =   _threeClipSize;
            }
            FECamera& camera  =   context.activeCamera();;
            real3       cenPos  =   real3(0, 0, 0);
            real        unitF   =   camera.pixelU(_d.position());
            real        size    =   unitF * pixels;
            real        planeSz =   unitF * planePix;
            _moveAxis.clear();
            _moveAxis.resize(13);
            ///center
            _moveAxis[0] = real3(0, 0, 0);
            ///X
            _moveAxis[1] = _d.axisX() * size;
            ///y
            _moveAxis[2] = _d.axisY() * size;
            ///z
            _moveAxis[3] = _d.axisZ() * size;
            ///xoy
            _moveAxis[4] = _d.axisX() * planeSz;
            _moveAxis[6] = _d.axisY() * planeSz;
            _moveAxis[5] = _moveAxis[4] + _moveAxis[6];
            ///float3 test = (_d.axisX() + _d.axisY())*planeSz;
            ///yoz
            _moveAxis[7] = _d.axisY() * planeSz;
            _moveAxis[9] = _d.axisZ() * planeSz;
            _moveAxis[8] = _moveAxis[7] + _moveAxis[9];
            ///zox
            _moveAxis[10] = _d.axisZ() * planeSz;
            _moveAxis[12] = _d.axisX() * planeSz;
            _moveAxis[11] = _moveAxis[10] + _moveAxis[12];

            uint            bufAxisSize =   (uint)_moveAxis.size() * sizeof(float3);
            (void)bufAxisSize;
            /// stub: FEDevice 暂未提供 createVertexBuffer 接口，渲染部分已占位
            if (!_axisLineVBO)
            {
                _axisLineVBO = std::make_shared<VertexBuffer>();
            }
            if(!_bThreeClipEdit)
            {
                ///计算箭头
                real arSize = unitF * 24;
                real arRadius = unitF * 5;
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

                uint            bufArSize =   (uint)_axisAr.size() * sizeof(float3);
                (void)bufArSize;
                /// stub: FEDevice 暂未提供 createVertexBuffer 接口，渲染部分已占位
                if (!_axisArrowVBO)
                {
                    _axisArrowVBO = std::make_shared<VertexBuffer>();
                }
            }
        }
        ///绘制轴
        void renderAxis(FEContext& context)
        {
            (void)context;
        }
    private:
        void renderAr(FEContext& context)
        {
            (void)context;
        }
    public:
        /// <summary>
        /// 拾取高亮轴
        /// </summary>
        FEEditAxisMove::AXIS hoverAxis(FEContext& context, const int2& pos)
        {
            FECamera& camera = context.activeCamera();
            Ray ray = camera.createRayFromScreen(pos.x, pos.y);
            real3 cenPos = _d.position();

            real pixelDis = 10.0;
            real unitF = camera.pixelU(cenPos);
            real worldDis = unitF * pixelDis;

            /// 先检测轴线索引 X, Y, Z
            real3 axisPts[3] = { _d.axisX(), _d.axisY(), _d.axisZ() };
            for (int i = 0; i < 3; ++i)
            {
                real3 axisEnd = cenPos + axisPts[i] * (unitF * 80);
                real dis = closeDistanceOnLine(cenPos, axisEnd, ray.getOrigin() + ray.getDirection() * dot(axisEnd - ray.getOrigin(), ray.getDirection()));
                if (dis < worldDis)
                {
                    FEEditAxisMove::AXIS axisEnum = (FEEditAxisMove::AXIS)(FEEditAxisMove::AXIS_X + i);
                    if (_d.enabled(axisEnum))
                    {
                        _hoveredAxis = axisEnum;
                        _d.sendHoveredDelegate();
                        return _hoveredAxis;
                    }
                }
            }

            /// 检测面索引 XY, YZ, XZ
            real planeSz = unitF * 40;
            real3 rect[3][4];
            /// XY 面
            rect[0][0] = cenPos;
            rect[0][1] = cenPos + _d.axisX() * planeSz;
            rect[0][2] = cenPos + _d.axisX() * planeSz + _d.axisY() * planeSz;
            rect[0][3] = cenPos + _d.axisY() * planeSz;
            /// YZ 面
            rect[1][0] = cenPos;
            rect[1][1] = cenPos + _d.axisY() * planeSz;
            rect[1][2] = cenPos + _d.axisY() * planeSz + _d.axisZ() * planeSz;
            rect[1][3] = cenPos + _d.axisZ() * planeSz;
            /// XZ 面
            rect[2][0] = cenPos;
            rect[2][1] = cenPos + _d.axisZ() * planeSz;
            rect[2][2] = cenPos + _d.axisZ() * planeSz + _d.axisX() * planeSz;
            rect[2][3] = cenPos + _d.axisX() * planeSz;

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
                if(_d.enabled(FEEditAxisMove::AXIS_X) || _d.enabled(FEEditAxisMove::AXIS_Y))
                    _hoveredAxis = FEEditAxisMove::AXIS_XY;
                else
                    _hoveredAxis = FEEditAxisMove::AXIS_NULL;
                break;
            }
            case 1:
            {
                if(_d.enabled(FEEditAxisMove::AXIS_Y) || _d.enabled(FEEditAxisMove::AXIS_Z))
                    _hoveredAxis = FEEditAxisMove::AXIS_YZ;
                else
                    _hoveredAxis = FEEditAxisMove::AXIS_NULL;
                break;
            }
            case 2:
            {
                if(_d.enabled(FEEditAxisMove::AXIS_X) || _d.enabled(FEEditAxisMove::AXIS_Z))
                    _hoveredAxis = FEEditAxisMove::AXIS_XZ;
                else
                    _hoveredAxis = FEEditAxisMove::AXIS_NULL;
                break;
            }
            default:    
                _hoveredAxis = FEEditAxisMove::AXIS_NULL;
                break;
            }
            _d.sendHoveredDelegate();
            return _hoveredAxis;
        }

        real3 moveX(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            FE::FECamera& camera = context.activeCamera();
            real3 axis = normalize(_d.axisX());

            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                real3 v = FE::closePointOnVector(axis, _downPosWorld, rPt);
                real3 retV = axis * length(v - _downPosWorld);
                if (dot(normalize(v - _downPosWorld), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }

            real3 faceNor = camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offMove;
            
            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0(0.0);
            real3 retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) &&
                calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                real3 v = FE::closePointOnVector(axis, retPt0, retPt1);
                real3 retV = axis * length(v - retPt0);
                if (dot(normalize(v - retPt0), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }
            return _offMove;
        }
        real3 moveY(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            FE::FECamera& camera = context.activeCamera();
            real3 axis = normalize(_d.axisY());

            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                real3 v = FE::closePointOnVector(axis, _downPosWorld, rPt);
                real3 retV = axis * length(v - _downPosWorld);
                if (dot(normalize(v - _downPosWorld), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }

            real3 faceNor = camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offMove;

            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0(0.0);
            real3 retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) &&
                calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                real3 v = FE::closePointOnVector(axis, retPt0, retPt1);
                real3 retV = axis * length(v - retPt0);
                if (dot(normalize(v - retPt0), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }
            return _offMove;
        }
        real3 moveZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            FE::FECamera& camera = context.activeCamera();
            real3 axis = normalize(_d.axisZ());
            
            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                real3 v = FE::closePointOnVector(axis, _downPosWorld, rPt);
                real3 retV = axis * length(v - _downPosWorld);
                if (dot(normalize(v - _downPosWorld), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }

            real3 faceNor = camera.getDir();
            if (!_d.calcAxisPlaneNormalize(camera, axis, faceNor))
                return _offMove;

            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0(0.0);
            real3 retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) &&
                calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                real3 v = FE::closePointOnVector(axis, retPt0, retPt1);
                real3 retV = axis * length(v - retPt0);
                if (dot(normalize(v - retPt0), axis) < 0)
                {
                    retV = -retV;
                }
                return retV;
            }
            return _offMove;
        }
        real3 moveXY(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            if(_bThreeClipEdit)
            {
                return moveZ(context, start, end);
            }
            FE::FECamera& camera = context.activeCamera();

            real3 faceNor = normalize(cross(_d.axisX(), _d.axisY()));
            
            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                ///将结果点投影到XOY平面上
                real3 rPrjPt = ptProjToPlane(rPt, faceNor, _downPosWorld);
                ///计算偏移量
                return rPrjPt - _downPosWorld;
            }

            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0(0.0);
            real3 retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0)
                && calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                return retPt1 - retPt0;
            }
            return _offMove;
        }
        real3 moveXZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            if(_bThreeClipEdit)
            {
                return moveY(context, start, end);
            }
            FE::FECamera& camera = context.activeCamera();

            real3 faceNor = normalize(cross(_d.axisX(), _d.axisZ()));
            
            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                ///将结果点投影到XOY平面上
                real3 rPrjPt = ptProjToPlane(rPt, faceNor, _downPosWorld);
                ///计算偏移量
                return rPrjPt - _downPosWorld;
            }

            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3 retPt0(0.0);
            real3 retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0)
                && calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                return retPt1 - retPt0;
            }
            return _offMove;
        }
        real3 moveYZ(FEContext& context, const FE::int2 &start, const FE::int2& end)
        {
            if(_bThreeClipEdit)
            {
                return moveX(context, start, end);
            }
            FECamera& camera  =   context.activeCamera();
            real3       faceNor =   normalize(cross(_d.axisY(), _d.axisZ()));
            
            ///吸附计算
            _adsorptionRet = this->calcAdsorption(&context, end);
            if (_adsorptionRet.isVaild())
            {
                real3 rPt = _adsorptionRet.ptWorld();
                ///将结果点投影到XOY平面上
                real3 rPrjPt = ptProjToPlane(rPt, faceNor, _downPosWorld);
                ///计算偏移量
                return rPrjPt - _downPosWorld;
            }

            Ray rayStart = camera.createRayFromScreen(start.x, start.y);
            Ray rayEnd = camera.createRayFromScreen(end.x, end.y);
            real3   retPt0(0.0);
            real3   retPt1(0.0);
            if (calcRaySurFaceInsPt(rayStart, faceNor, _d.position(), retPt0) 
                && calcRaySurFaceInsPt(rayEnd, faceNor, _d.position(), retPt1))
            {
                return retPt1 - retPt0;
            }
            return _offMove;
        }

        ///计算吸附
        AdsorptionResult calcAdsorption(FEContext* app, const int2& screenPos)
        {
            if (!_adsorptionEnabled)
                return AdsorptionResult();

            FECamera& camera = (*app).activeCamera();
            AdsorptionResults rets;
            AdsorptionResult ret;
            ///几何体顶点吸附
            if (_adsorptionFlags.hasFlag(FEEditAxisMove::AdsorptionFlag::AF_Points)
                && CalcMoveAdsorptionPoint(app, screenPos, _adsorptionPixel, ret))
            {
                rets.push_back(ret);
            }
            ///位置点吸附
            if (_adsorptionFlags.hasFlag(FEEditAxisMove::AdsorptionFlag::AF_Position)
                && CalcMoveAdsorptionPosition(app, screenPos, _adsorptionPixel, ret))
            {
                rets.push_back(ret);
            }
            ///包围盒吸附
            if (_adsorptionFlags.hasFlag(FEEditAxisMove::AdsorptionFlag::AF_Aabb)
                && CalcMoveAdsorptionAabb(app, screenPos, _adsorptionPixel, ret))
            {
                rets.push_back(ret);
            }

            real minDis = 9999999.0;
            for (size_t i = 0; i < rets.size(); ++i)
            {
                if (!rets[i].isVaild())
                    continue;
                real3 tmpPt = rets[i].ptWorld();
                real dis = distance(camera.getEye(), tmpPt);
                if (dis > minDis)
                    continue;
                minDis = dis;
                ret = rets[i];
            }
            return ret;
        }

        
        void createIBO(FEContext& context)
        {
            _lineindex.clear();
            for (uint i = 0; i < 3; i++)
            {
                uint firstIndex0 = 4 + 3 * i;//4:面的第一个索引点
                uint firstIndex1 = 5 + (3 * ((i + 2) % 3));///5:面的第二个索引点
                ///与轴线相邻面框线索引
                _lineindex.push_back(firstIndex0);
                _lineindex.push_back(firstIndex0 + 1);
                _lineindex.push_back(firstIndex1);
                _lineindex.push_back(firstIndex1 + 1);
                ///轴线索引3
                _lineindex.push_back(0);
                _lineindex.push_back(i + 1);
                ///面索引
                _lineindex.push_back(0);
                _lineindex.push_back(firstIndex0);
                _lineindex.push_back(firstIndex0 + 1);
                _lineindex.push_back(firstIndex0 + 2);
            }
            /// stub: FEDevice 暂未提供 createIndexBuffer 接口，渲染部分已占位
            if (!_axisLineIBO)
            {
                _axisLineIBO = std::make_shared<IndexBuffer>();
            }
            (void)context;
        }
        /// <summary>
        /// 根据选中的轴计算移动偏移量
        /// </summary>
        real3 calcMove(FEContext& context, const int2& start, const int2& end)
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
        /// <summary>
        /// 绘制吸附点(占位)
        /// </summary>
        void renderAdsorptionPoint(FEContext& context)
        {
            (void)context;
        }
        /// <summary>
        /// 绘制吸附线(占位)
        /// </summary>
        void renderAdsorptionLine(FEContext& context)
        {
            (void)context;
        }
    };

    FEEditAxisMove::FEEditAxisMove(FEContext& context): FEEditAxis(EditAxisTypeMove, context)
    {
        _p = new FEEditAxisMovePrivate(*this);
    }
    FEEditAxisMove::~FEEditAxisMove()
    {
        delete _p;
    }

    FEEditAxisMove::MDelegate& FEEditAxisMove::mDelegate()
    {
        return _p->_delegate;
    }

    FEEditAxisMove::AXIS FEEditAxisMove::hoveredAxis() const
    {
        return _p->_hoveredAxis;
    }
    FEEditAxisMove::AXIS FEEditAxisMove::selectedAxis() const
    {
        return _p->_selectedAxis;
    }

    
    void FEEditAxisMove::setAdsorptionEnabled(bool enabled)
    {
        _p->_adsorptionEnabled = enabled;
    }
    bool FEEditAxisMove::adsorptionEnabled() const
    {
        return _p->_adsorptionEnabled;
    }
    FEEditAxisMove::AdsorptionFlags FEEditAxisMove::adsorptionFlags() const
    {
        return _p->_adsorptionFlags;
    }
    void FEEditAxisMove::setAdsorptionFlags(AdsorptionFlags flags)
    {
        _p->_adsorptionFlags = flags;
    }
    real FEEditAxisMove::adsorptionPixel() const
    {
        return _p->_adsorptionPixel;
    }
    void FEEditAxisMove::setAdsorptionPixel(real pixel)
    {
        _p->_adsorptionPixel = pixel;
    }

    bool FEEditAxisMove::isClipEdit() const
    {
        return _p->_bThreeClipEdit;
    }

    void FEEditAxisMove::setClipEdit(bool b)
    {
        _p->_bThreeClipEdit = b;
    }

    void FEEditAxisMove::cancelHovered()
    {
        AXIS oldHovered = _p->_hoveredAxis;
        _p->_hoveredAxis = FEEditAxisMove::AXIS::AXIS_NULL;
        if (oldHovered != _p->_hoveredAxis)
        {
            this->sendHoveredDelegate();
        }
    }
    void FEEditAxisMove::cancelSelected()
    {
        AXIS oldSelected = _p->_selectedAxis;
        _p->_selectedAxis = FEEditAxisMove::AXIS::AXIS_NULL;
        if (oldSelected != _p->_selectedAxis)
        {
            this->sendSelectedDelegate();
        }
    }

    bool FEEditAxisMove::mouseButtonPress(FEContext& context, const int2& pos)
    {
        _p->_adsorptionRet.reset();

        _p->_downPosWorld = this->position();

        _p->_bMouseDown = true;

        AXIS oldSelected = _p->_selectedAxis;
        _p->_selectedAxis = _p->_hoveredAxis;
        if (oldSelected != _p->_selectedAxis)
        {
            this->sendSelectedDelegate();
            context.requireNextFrame();
        }

        if (isAxisSelected())
        {
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offMove = real3(0.0);
            _p->_delegate(EditStatus::EditStart, real3(0.0), real3(0.0), *this);
            context.requireNextFrame();
            return true;
        }
        return false;
    }
    bool FEEditAxisMove::mouseButtonRelease(FEContext& context, const int2& pos)
    {
        _p->_adsorptionRet.reset();

        _p->_downPosWorld = this->position();

        _p->_bMouseDown = false;
        if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, real3(0.0), _p->_offMove, *this);
            _p->_downPos = pos;
            _p->_startPos = pos;
            _p->_offMove = real3(0);

            AXIS oldSelected = _p->_selectedAxis;
            _p->_selectedAxis = AXIS_NULL;
            if (oldSelected != _p->_selectedAxis)
            {
                this->sendSelectedDelegate();
                context.requireNextFrame();
            }
            return true;
        }
        return false;
    }
    bool FEEditAxisMove::mouseMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_startPos, pos);
            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
            _p->_downPos = pos;
            _context.requireNextFrame();
            return true;
        }
        else if(!_p->_bMouseDown)
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

    bool FEEditAxisMove::touchDown(FEContext& context, const int2& pos)
    {
        {
            ///拾取动作
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
        }
        _p->_downPosWorld = this->position();

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
    bool FEEditAxisMove::touchUp(FEContext& context, const int2& pos)
    {
        
        _p->_downPosWorld = this->position();

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
            _p->_bTouchPickup = false;
            return false;
        }
        else if (isAxisSelected())
        {
            _p->_delegate(EditStatus::EditEnd, real3(0.0), _p->_offMove, *this);
            _p->_touchDownPos = pos;
            _p->_touchStartPos = pos;
            _p->_offMove = real3(0);
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
    bool FEEditAxisMove::touchMove(FEContext& context, const int2& pos)
    {
        if (isAxisSelected())
        {
            real3 oldOff = _p->_offMove;
            real3 offset = _p->calcMove(context, _p->_touchStartPos, pos);
            offset = offset - oldOff;
            _p->_delegate(EditStatus::Editting, offset, _p->_offMove, *this);
            _p->_touchDownPos = pos;
            _context.requireNextFrame();
            return true;
        }
        return false;
    }

    void FEEditAxisMove::update(FEContext& context)
    {
        _p->updateAxisVerties(context);
    }
    void FEEditAxisMove::render(FEContext& context)
    {
        if (!_internalFlags.hasFlag(InteralFlag_Visible))
            return;
        _p->renderAxis(context);

        _p->renderAdsorptionPoint(context);
        _p->renderAdsorptionLine(context);
    }
}

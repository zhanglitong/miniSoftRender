#pragma     once

#include    "FEConstraint.hpp"
#include    "../node/FENode.hpp"
#include    <vector>

namespace FE
{
    DEFINE_CLASS_UUID(FEConstraintPath, "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}");

    /// <summary>
    /// 路径约束：使被约束节点沿给定的路径点序列移动
    /// 路径由一组有序的路径点(waypoints)构成,相邻点之间用直线段连接;
    /// 节点以恒定速度沿路径行进,到终点后可选择循环回到起点
    ///
    /// 典型用法:
    ///   ConstraintPath path = new FEConstraintPath(ctx);
    ///   path->addPoint(real3(-1,0,0));
    ///   path->addPoint(real3( 1,0,0));
    ///   path->setSpeed(2.0f);     /// 2 单位/秒
    ///   path->setLoop(true);
    ///   node->addComponent(path.get());
    ///
    /// 注意: 本约束直接设置世界平移,与 FEConstraintTrans 类似;
    /// 若同一节点同时挂 LookAt 约束,可用 priority 控制求解顺序
    /// (路径先定位置,LookAt 再算朝向)
    /// </summary>
    class   FE_API  FEConstraintPath : public FEConstraint
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintPath)
    public:
        FEConstraintPath(FEContext& ctx)
            :FEConstraint(ctx)
        {}
        FEConstraintPath(const FEConstraintPath& other)    =   default;
        ~FEConstraintPath()                                =   default;
    public:
        /// <summary>
        /// 整体设置路径点(替换已有路径)
        /// </summary>
        void    setPath(const std::vector<real3>& points)
        {
            _points        =   points;
            _dirty         =   true;
        }
        /// <summary>
        /// 追加一个路径点
        /// </summary>
        void    addPoint(const real3& point)
        {
            _points.push_back(point);
            _dirty  =   true;
        }
        /// <summary>
        /// 清空路径点
        /// </summary>
        void    clearPoints()
        {
            _points.clear();
            _dirty  =   true;
        }
        /// <summary>
        /// 当前路径点数量
        /// </summary>
        size_t  pointCount() const { return _points.size(); }
        /// <summary>
        /// 只读访问路径点
        /// </summary>
        const std::vector<real3>&  points() const { return _points; }
        /// <summary>
        /// 移动速度(单位/秒)
        /// </summary>
        real    speed() const { return _speed; }
        void    setSpeed(real s) { _speed = s; }
        /// <summary>
        /// 是否循环(到终点后回到起点),默认 true
        /// </summary>
        bool    loop() const { return _loop; }
        void    setLoop(bool b) { _loop = b; }
        /// <summary>
        /// 起点偏移: 沿路径从起点偏移的距离(单位),用于控制初始位置
        /// </summary>
        real    startOffset() const { return _startOffset; }
        void    setStartOffset(real off) { _startOffset = off; }
    public:
        /// <summary>
        /// 求解: 根据约束系统累计时间和速度,计算节点在路径上的世界位置
        /// 仅修改平移,保留当前旋转与缩放
        /// </summary>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const override
        {
            FETransform   desired   =   currentWorld;
            if (_points.size() < 2)
                return  desired;

            ensureCache();

            /// 沿路径行进的距离 = 速度 * 时间 + 起点偏移
            real    dist    =   _speed * time + _startOffset;
            if (_loop && _totalLength > 0)
                dist    =   std::fmod(dist, _totalLength);
            else
                dist    =   std::clamp(dist, real(0), _totalLength);

            desired.setPosition(sampleAtDistance(dist));
            return  desired;
        }
    protected:
        /// <summary>
        /// 根据沿路径距离取位置点
        /// </summary>
        real3   sampleAtDistance(real dist) const
        {
            if (_points.empty())
                return  real3(0);
            if (_points.size() == 1)
                return  _points.front();

            /// 找到 dist 所在的线段
            real    acc =   0;
            for (size_t i = 1; i < _points.size(); ++i)
            {
                real    segLen  =   _segLengths[i - 1];
                if (dist <= acc + segLen)
                {
                    real    t   =   (segLen > 0) ? ((dist - acc) / segLen) : real(0);
                    return  _points[i - 1] + t * (_points[i] - _points[i - 1]);
                }
                acc +=  segLen;
            }
            /// 超出末尾(非循环且 clamp 后也不会到这里,保险起见返回终点)
            return  _points.back();
        }
        /// <summary>
        /// 缓存各段长度与总长度;路径变更时标记 _dirty 后重建
        /// </summary>
        void    ensureCache() const
        {
            if (!_dirty)
                return;
            _segLengths.resize(_points.size() > 0 ? _points.size() - 1 : 0);
            _totalLength    =   0;
            for (size_t i = 1; i < _points.size(); ++i)
            {
                real    len =   glm::length(_points[i] - _points[i - 1]);
                _segLengths[i - 1]  =   len;
                _totalLength        +=  len;
            }
            _dirty  =   false;
        }
    protected:
        /// 路径点序列(世界坐标)
        std::vector<real3>     _points;
        /// 移动速度(单位/秒)
        real                    _speed          =   1.0f;
        /// 是否循环
        bool                    _loop           =   true;
        /// 起点偏移(沿路径距离)
        real                    _startOffset    =   0;
        /// 缓存: 各段长度
        mutable std::vector<real>   _segLengths;
        /// 缓存: 路径总长度
        mutable real                _totalLength    =   0;
        /// 缓存是否失效
        mutable bool                _dirty          =   true;
    };

    using   ConstraintPath  =   SharedPtr<FEConstraintPath>;
}

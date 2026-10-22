#pragma     once

#include    "../../FEMath.hpp"
#include    <vector>
#include    <cmath>
#include    <limits>

namespace   FE
{
    /// <summary>
    /// 曲线类型
    /// </summary>
    enum class  FECurveType : uint
    {
        /// 直线
        Line                =   0,
        /// 二次贝塞尔
        QuadraticBezier     =   1,
        /// 三次贝塞尔
        CubicBezier         =   2,
        /// Catmull-Rom 样条
        CatmullRom          =   3,
    };

    /// <summary>
    /// Catmull-Rom 参数化类型
    /// </summary>
    enum class  FECatmullRomType : uint
    {
        /// 向心(默认, 避免尖点和自交)
        Centripetal =   0,
        /// 弦长
        Chordal     =   1,
        /// 均匀
        Uniform     =   2,
    };

    /// <summary>
    /// Frenet 标架 (切线/法线/副法线), 模板参数 T 为浮点类型(float/double)
    /// </summary>
    template<typename T>
    struct  FEFrenetFramesT
    {
        std::vector<tvec3<T>>   tangents;
        std::vector<tvec3<T>>   normals;
        std::vector<tvec3<T>>   binormals;
    };

    /// <summary>
    /// 3D 曲线基类模板, 移植自 three.js r160 extras/core/Curve.js
    /// 子类只需实现 getPoint(t), t∈[0,1]
    /// 模板参数 T: 浮点类型(float/double)
    /// </summary>
    template<typename T>
    class   FECurveT
    {
    public:
        using   Scalar      =   T;
        using   Point       =   tvec3<T>;
        using   Points      =   std::vector<Point>;
        using   Quaternion  =   tquat<T>;

        virtual ~FECurveT() {}

        /// <summary>弧长分段数(用于弧长重参数化)</summary>
        uint    arcLengthDivisions =   200;

        /// <summary>
        /// 子类实现: 参数 t∈[0,1] 对应的曲线点
        /// </summary>
        virtual Point   getPoint(T t) const   =   0;

        /// <summary>
        /// 按弧长比例 u∈[0,1] 取点(等距采样)
        /// </summary>
        Point   getPointAt(T u) const
        {
            return  getPoint(getUtoTmapping(u));
        }

        /// <summary>
        /// 按参数 t 均匀取 divisions+1 个点
        /// </summary>
        Points  getPoints(uint divisions = 5) const
        {
            Points points;
            points.reserve(divisions + 1);
            for (uint d = 0; d <= divisions; ++d)
                points.push_back(getPoint(static_cast<T>(d) / static_cast<T>(divisions)));
            return  points;
        }

        /// <summary>
        /// 按弧长等距取 divisions+1 个点
        /// </summary>
        Points  getSpacedPoints(uint divisions = 5) const
        {
            Points points;
            points.reserve(divisions + 1);
            for (uint d = 0; d <= divisions; ++d)
                points.push_back(getPointAt(static_cast<T>(d) / static_cast<T>(divisions)));
            return  points;
        }

        /// <summary>
        /// 曲线总弧长
        /// </summary>
        T   getLength() const
        {
            const auto& lengths   =   getLengths();
            return  lengths.back();
        }

        /// <summary>
        /// 累积段长度表
        /// </summary>
        const std::vector<T>&   getLengths(uint divisions = 0) const
        {
            if (divisions == 0)
                divisions   =   arcLengthDivisions;
            if (!_cacheArcLengths.empty()
             && (_cacheArcLengths.size() == (size_t)divisions + 1)
             && !_needsUpdate)
            {
                return  _cacheArcLengths;
            }
            _needsUpdate    =   false;

            auto&   cache   =   _cacheArcLengths;
            cache.clear();
            cache.reserve(divisions + 1);

            Point   last    =   getPoint(T(0));
            T       sum     =   T(0);
            cache.push_back(T(0));
            for (uint p = 1; p <= divisions; ++p)
            {
                Point   current =   getPoint(static_cast<T>(p) / static_cast<T>(divisions));
                sum +=   FE::distance(current, last);
                cache.push_back(sum);
                last    =   current;
            }
            return  cache;
        }

        /// <summary>
        /// 强制刷新弧长缓存(曲线参数变化后调用)
        /// </summary>
        void    updateArcLengths()
        {
            _needsUpdate    =   true;
            getLengths();
        }

        /// <summary>
        /// 弧长比例 u -> 曲线参数 t (二分查找)
        /// </summary>
        T   getUtoTmapping(T u, T distance = T(-1)) const
        {
            const auto& arcLengths =   getLengths();
            uint    il  =   (uint)arcLengths.size();

            T   targetArcLength;
            if (distance >= T(0))
                targetArcLength =   distance;
            else
                targetArcLength =   u * arcLengths[il - 1];

            /// 二分查找累积长度最接近目标的段
            uint    low =   0, high = il - 1, i = 0;
            while (low <= high)
            {
                i   =   low + (high - low) / 2;
                T   comparison  =   arcLengths[i] - targetArcLength;
                if (comparison < T(0))
                    low     =   i + 1;
                else if (comparison > T(0))
                    high    =   i - 1;
                else
                {
                    high    =   i;
                    break;
                }
            }
            i   =   high;

            if (arcLengths[i] == targetArcLength)
                return  static_cast<T>(i) / static_cast<T>(il - 1);

            T   lengthBefore    =   arcLengths[i];
            T   lengthAfter     =   arcLengths[i + 1];
            T   segmentLength   =   lengthAfter - lengthBefore;
            T   segmentFraction =   (targetArcLength - lengthBefore) / segmentLength;
            T   t               =   (static_cast<T>(i) + segmentFraction) / static_cast<T>(il - 1);
            return  t;
        }

        /// <summary>
        /// 参数 t 处的单位切线(中心差分, 子类可覆写为解析形式)
        /// </summary>
        virtual Point   getTangent(T t) const
        {
            const T delta =   T(0.0001);
            T   t1  =   t - delta;
            T   t2  =   t + delta;
            if (t1 < T(0)) t1 =   T(0);
            if (t2 > T(1)) t2 =   T(1);

            Point   pt1 =   getPoint(t1);
            Point   pt2 =   getPoint(t2);
            return  FE::normalize(pt2 - pt1);
        }

        /// <summary>
        /// 按弧长比例 u 取单位切线
        /// </summary>
        Point   getTangentAt(T u) const
        {
            T   t   =   getUtoTmapping(u);
            return  getTangent(t);
        }

        /// <summary>
        /// 计算 Frenet 标架, 移植自 three.js Curve.computeFrenetFrames
        /// 参见 http://www.cs.indiana.edu/pub/techreports/TR425.pdf
        /// </summary>
        FEFrenetFramesT<T>  computeFrenetFrames(uint segments, bool closed) const
        {
            FEFrenetFramesT<T>  frames;
            frames.tangents.resize(segments + 1);
            frames.normals.resize(segments + 1);
            frames.binormals.resize(segments + 1);

            /// 每段切线
            for (uint i = 0; i <= segments; ++i)
            {
                T   u   =   static_cast<T>(i) / static_cast<T>(segments);
                frames.tangents[i]  =   getTangentAt(u);
            }

            /// 选择与首条切线垂直、沿其最小分量方向的初始法线
            Point   normal(T(0), T(0), T(0));
            T       minV    =   std::numeric_limits<T>::max();
            T       tx  =   std::abs(frames.tangents[0].x);
            T       ty  =   std::abs(frames.tangents[0].y);
            T       tz  =   std::abs(frames.tangents[0].z);
            if (tx <= minV) { minV = tx; normal = Point(T(1), T(0), T(0)); }
            if (ty <= minV) { minV = ty; normal = Point(T(0), T(1), T(0)); }
            if (tz <= minV) {             normal = Point(T(0), T(0), T(1)); }

            Point   vec =   FE::normalize(FE::cross(frames.tangents[0], normal));
            frames.normals[0]     =   FE::cross(frames.tangents[0], vec);
            frames.binormals[0]   =   FE::cross(frames.tangents[0], frames.normals[0]);

            /// 逐段传播法线/副法线
            for (uint i = 1; i <= segments; ++i)
            {
                frames.normals[i]     =   frames.normals[i - 1];
                frames.binormals[i]   =   frames.binormals[i - 1];

                vec =   FE::cross(frames.tangents[i - 1], frames.tangents[i]);
                if (FE::length(vec) > std::numeric_limits<T>::epsilon())
                {
                    vec =   FE::normalize(vec);
                    T   theta   =   std::acos(FE::clamp(FE::dot(frames.tangents[i - 1], frames.tangents[i]), T(-1), T(1)));
                    Quaternion  rot =   FE::angleAxis(theta, vec);
                    frames.normals[i]   =   rot * frames.normals[i];
                }
                frames.binormals[i]   =   FE::cross(frames.tangents[i], frames.normals[i]);
            }

            /// 闭合曲线: 扭转使首尾法线一致
            if (closed)
            {
                T   theta   =   std::acos(FE::clamp(FE::dot(frames.normals[0], frames.normals[segments]), T(-1), T(1)));
                theta   /=  static_cast<T>(segments);
                if (FE::dot(frames.tangents[0], FE::cross(frames.normals[0], frames.normals[segments])) > T(0))
                    theta   =   -theta;

                for (uint i = 1; i <= segments; ++i)
                {
                    Quaternion  rot =   FE::angleAxis(theta * static_cast<T>(i), frames.tangents[i]);
                    frames.normals[i]     =   rot * frames.normals[i];
                    frames.binormals[i]   =   FE::cross(frames.tangents[i], frames.normals[i]);
                }
            }
            return  frames;
        }

    protected:
        mutable std::vector<T>      _cacheArcLengths;
        mutable bool                _needsUpdate    =   false;
    };

    /// <summary>
    /// 单精度别名 (向后兼容)
    /// </summary>
    using   FEFrenetFrames  =   FEFrenetFramesT<float>;
    using   FECurve         =   FECurveT<float>;
    /// <summary>
    /// 双精度别名
    /// </summary>
    using   FEFrenetFramesD =   FEFrenetFramesT<double>;
    using   FECurveD        =   FECurveT<double>;
}

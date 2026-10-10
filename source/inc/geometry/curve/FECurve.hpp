#pragma     once

#include    "../../FEMath.hpp"
#include    <vector>
#include    <cmath>
#include    <cfloat>

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
    /// Frenet 标架 (切线/法线/副法线)
    /// </summary>
    struct  FEFrenetFrames
    {
        float3s     tangents;
        float3s     normals;
        float3s     binormals;
    };

    /// <summary>
    /// 3D 曲线基类, 移植自 three.js r160 extras/core/Curve.js
    /// 子类只需实现 getPoint(t), t∈[0,1]
    /// </summary>
    class   FECurve
    {
    public:
        virtual ~FECurve() {}

        /// <summary>弧长分段数(用于弧长重参数化)</summary>
        uint    arcLengthDivisions =   200;

        /// <summary>
        /// 子类实现: 参数 t∈[0,1] 对应的曲线点
        /// </summary>
        virtual float3  getPoint(float t) const   =   0;

        /// <summary>
        /// 按弧长比例 u∈[0,1] 取点(等距采样)
        /// </summary>
        float3  getPointAt(float u) const
        {
            return  getPoint(getUtoTmapping(u));
        }

        /// <summary>
        /// 按参数 t 均匀取 divisions+1 个点
        /// </summary>
        float3s getPoints(uint divisions = 5) const
        {
            float3s points;
            points.reserve(divisions + 1);
            for (uint d = 0; d <= divisions; ++d)
                points.push_back(getPoint((float)d / (float)divisions));
            return  points;
        }

        /// <summary>
        /// 按弧长等距取 divisions+1 个点
        /// </summary>
        float3s getSpacedPoints(uint divisions = 5) const
        {
            float3s points;
            points.reserve(divisions + 1);
            for (uint d = 0; d <= divisions; ++d)
                points.push_back(getPointAt((float)d / (float)divisions));
            return  points;
        }

        /// <summary>
        /// 曲线总弧长
        /// </summary>
        float   getLength() const
        {
            const auto& lengths   =   getLengths();
            return  lengths.back();
        }

        /// <summary>
        /// 累积段长度表
        /// </summary>
        const std::vector<float>&   getLengths(uint divisions = 0) const
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

            float3  last    =   getPoint(0.0f);
            float   sum     =   0.0f;
            cache.push_back(0.0f);
            for (uint p = 1; p <= divisions; ++p)
            {
                float3  current =   getPoint((float)p / (float)divisions);
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
        float   getUtoTmapping(float u, float distance = -1.0f) const
        {
            const auto& arcLengths =   getLengths();
            uint    il  =   (uint)arcLengths.size();

            float   targetArcLength;
            if (distance >= 0.0f)
                targetArcLength =   distance;
            else
                targetArcLength =   u * arcLengths[il - 1];

            /// 二分查找累积长度最接近目标的段
            uint    low =   0, high = il - 1, i = 0;
            while (low <= high)
            {
                i   =   low + (high - low) / 2;
                float   comparison  =   arcLengths[i] - targetArcLength;
                if (comparison < 0.0f)
                    low     =   i + 1;
                else if (comparison > 0.0f)
                    high    =   i - 1;
                else
                {
                    high    =   i;
                    break;
                }
            }
            i   =   high;

            if (arcLengths[i] == targetArcLength)
                return  (float)i / (float)(il - 1);

            float   lengthBefore    =   arcLengths[i];
            float   lengthAfter     =   arcLengths[i + 1];
            float   segmentLength   =   lengthAfter - lengthBefore;
            float   segmentFraction =   (targetArcLength - lengthBefore) / segmentLength;
            float   t               =   ((float)i + segmentFraction) / (float)(il - 1);
            return  t;
        }

        /// <summary>
        /// 参数 t 处的单位切线(中心差分, 子类可覆写为解析形式)
        /// </summary>
        virtual float3  getTangent(float t) const
        {
            const float delta =   0.0001f;
            float   t1  =   t - delta;
            float   t2  =   t + delta;
            if (t1 < 0.0f) t1 =   0.0f;
            if (t2 > 1.0f) t2 =   1.0f;

            float3  pt1 =   getPoint(t1);
            float3  pt2 =   getPoint(t2);
            return  FE::normalize(pt2 - pt1);
        }

        /// <summary>
        /// 按弧长比例 u 取单位切线
        /// </summary>
        float3  getTangentAt(float u) const
        {
            float   t   =   getUtoTmapping(u);
            return  getTangent(t);
        }

        /// <summary>
        /// 计算 Frenet 标架, 移植自 three.js Curve.computeFrenetFrames
        /// 参见 http://www.cs.indiana.edu/pub/techreports/TR425.pdf
        /// </summary>
        FEFrenetFrames  computeFrenetFrames(uint segments, bool closed) const
        {
            FEFrenetFrames  frames;
            frames.tangents.resize(segments + 1);
            frames.normals.resize(segments + 1);
            frames.binormals.resize(segments + 1);

            /// 每段切线
            for (uint i = 0; i <= segments; ++i)
            {
                float   u   =   (float)i / (float)segments;
                frames.tangents[i]  =   getTangentAt(u);
            }

            /// 选择与首条切线垂直、沿其最小分量方向的初始法线
            float3  normal(0, 0, 0);
            float   minV    =   FLT_MAX;
            float   tx  =   std::abs(frames.tangents[0].x);
            float   ty  =   std::abs(frames.tangents[0].y);
            float   tz  =   std::abs(frames.tangents[0].z);
            if (tx <= minV) { minV = tx; normal = float3(1, 0, 0); }
            if (ty <= minV) { minV = ty; normal = float3(0, 1, 0); }
            if (tz <= minV) {             normal = float3(0, 0, 1); }

            float3  vec =   FE::normalize(FE::cross(frames.tangents[0], normal));
            frames.normals[0]     =   FE::cross(frames.tangents[0], vec);
            frames.binormals[0]   =   FE::cross(frames.tangents[0], frames.normals[0]);

            /// 逐段传播法线/副法线
            for (uint i = 1; i <= segments; ++i)
            {
                frames.normals[i]     =   frames.normals[i - 1];
                frames.binormals[i]   =   frames.binormals[i - 1];

                vec =   FE::cross(frames.tangents[i - 1], frames.tangents[i]);
                if (FE::length(vec) > FLT_EPSILON)
                {
                    vec =   FE::normalize(vec);
                    float   theta   =   std::acos(FE::clamp(FE::dot(frames.tangents[i - 1], frames.tangents[i]), -1.0f, 1.0f));
                    quatf   rot     =   FE::angleAxis(theta, vec);
                    frames.normals[i]   =   rot * frames.normals[i];
                }
                frames.binormals[i]   =   FE::cross(frames.tangents[i], frames.normals[i]);
            }

            /// 闭合曲线: 扭转使首尾法线一致
            if (closed)
            {
                float   theta   =   std::acos(FE::clamp(FE::dot(frames.normals[0], frames.normals[segments]), -1.0f, 1.0f));
                theta   /=  (float)segments;
                if (FE::dot(frames.tangents[0], FE::cross(frames.normals[0], frames.normals[segments])) > 0.0f)
                    theta   =   -theta;

                for (uint i = 1; i <= segments; ++i)
                {
                    quatf   rot =   FE::angleAxis(theta * (float)i, frames.tangents[i]);
                    frames.normals[i]     =   rot * frames.normals[i];
                    frames.binormals[i]   =   FE::cross(frames.tangents[i], frames.normals[i]);
                }
            }
            return  frames;
        }

    protected:
        mutable std::vector<float>  _cacheArcLengths;
        mutable bool                _needsUpdate    =   false;
    };
}

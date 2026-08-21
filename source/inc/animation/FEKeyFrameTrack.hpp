#pragma     once

#include    "../FEPropertyIndex.hpp"
#include    "FETrackResult.hpp"
#include    "TValueArray.hpp"

namespace FE
{
    /// 
    /// tension值	效果	            应用场景
    /// 0.0	        完全线性插值	    匀速运动、机械运动、默认值
    /// 0.3 - 0.5   缓入缓出（标准）	最常用，自然运动、UI动画
    /// 0.5 - 0.8   强缓入缓出	        强调效果、弹性感、UI弹窗
    /// -0.3 - -0.5	反向缓入缓出	    先快后慢再快（特殊效果）
    /// 1.0	        极端加速	        快速启动、冲击效果
    /// -1.0        极端减速	        缓慢启动、阻尼效果
    /// 
    
    /// Material Design 标准缓动
    const   real    MD_EASE     =   0.4;  
    /// iOS 标准缓动
    const   real    IOS_EASE    =   0.42;
    /// 弹性效果
    const   real    BOUNCE_EASE =   0.6;

    using   PropIndex       =   int;
    DEFINE_CLASS_UUID(FEKeyFrameTrack, "{474DC049-28C0-4F79-AB66-CDCA4707DD8B}");

    class   FEKeyFrameTrack : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEKeyFrameTrack)
    public:
        enum    InterpolateType : uint8_t
        {
            /// <summary>
            /// 贝塞尔曲线
            /// </summary>
            IT_Bezier,
            /// <summary>
            /// 线性
            /// </summary>
            IT_Linear,
            /// <summary>
            /// 常量（离散）
            /// </summary>
            IT_Constant,
        };
    public:
        FEKeyFrameTrack(FEContext& ctx,PropIndex index = -1)  
            :FEObject(ctx)
        {
            _propIndex  =   index;
        }
        FEKeyFrameTrack(const FEKeyFrameTrack& other)
            :FEObject(other)
        {
            _propIndex  =   other._propIndex;
            _times      =   other._times;
            _values     =   other._values;
        }
        ~FEKeyFrameTrack() 
        {}
    public:
        /// <summary>
        /// 范围
        /// </summary>
        /// <returns></returns>
        real2       range() const
        {
            if (_times == nullptr || _times->values().empty())
                return  real2(-1,-1);
            real    nMin  =   _times->values().front();
            real    nMax  =   _times->values().back();
            return  real2(nMin,nMax);
        }
        /// <summary>
        /// 是否有效 
        /// </summary>
        /// <returns></returns>
        bool        isValid() const
        {
            return (_times && !_times->values().empty());
        }
        /// <summary>
        /// 设置属性索引
        /// </summary>
        /// <param name="index"></param>
        void        setPropertyIndex(const PropIndex& index)
        {
            _propIndex  =   index;
        }
        void        setTimeObject(RealsObject timeObject)
        {
            _times  =   timeObject;
        }
        void        setValueObject(RealsObject valueObject)
        {
            _values  =   valueObject;
        }
        /// <summary>
        /// 获取时间信息
        /// </summary>
        /// <returns></returns>
        auto        times() const
        {
            return _times;
        }
        /// <summary>
        /// 获取数据信息
        /// </summary>
        /// <returns></returns>
        auto&       values()
        {
            return _values;
        }
        /// <summary>
        /// 第一帧信息
        /// </summary>
        /// <returns></returns>
        real2       front() const
        {
            if (   _values == nullptr 
                || _times == nullptr 
                || _values->values().empty() 
                || _times->values().empty())
                return  real2(0,0);
            else
                return  real2(_times->values().front(),_values->values().front());
        }
        /// <summary>
        /// 最后一帧数据
        /// </summary>
        /// <returns></returns>
        real2       back() const
        {
            if (   _values == nullptr 
                || _times == nullptr 
                || _values->values().empty() 
                || _times->values().empty())
                return  real2(0,0);
            else
                return  real2(_times->values().back(),_values->values().back());
        }
        /// <summary>
        /// 添加关键帧
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns></returns>
        bool        addKeyFrame(const real& tm,const real& value)
        {
            /// 查找插入位置
            auto    itr =   std::lower_bound(_times->values().begin(), _times->values().end(), tm, [](const real& l, const real& r)
            {
                return l < r;
            });
            /// 没有找到
            if (itr == _times->values().end())
            {
                _times->values().push_back(tm);
                _values->values().push_back(value);
            } 
            /// 找到插入位置
            else if((*itr)  != tm)
            {
                const auto  dist    =   std::distance(_times->values().begin(),itr);
                _times->values().insert(itr,tm);
                auto vItr   =   _values->values().begin() + dist;
                _values->values().insert(vItr,value);
            }
            else
                return  false;
            return  true;
        }
        /// <summary>
        /// 追加关键帧
        /// </summary>
        /// <param name="frames"></param>
        void        appendKeyFrames(const real2s& frames)
        {
            _times->values().reserve(_times->values().size() + frames.size());
            _values->values().reserve(_values->values().size() + frames.size());
            for (auto& frame : frames)
            {
                _times->values().push_back(frame.x);
                _values->values().push_back(frame.y);
            }
        }
        /// <summary>
        /// 根据索引获取关键帧数据
        /// </summary>
        /// <param name="index"></param>
        /// <returns>real2(-1,-1) 无效/returns>
        real2       keyFrame(size_t index) const
        {
            if (index < _times->values().size())
                return  real2(_times->values().at(index),_values->values().at(index));
            else
                return  real2(-1,-1);
        }
        /// <summary>
        /// 需要手动调用
        /// </summary>
        void        sortKeyFames()
        {
            auto&   times   =   _times->values();
            auto&   values  =   _values->values();
            assert(times.size() == values.size());
            if (times.size() != values.size())
            {
                return;
            }
            real2s  frames(values.size());
            for (size_t i = 0; i < values.size(); i++)
            {
                frames[i].x =   times[i];
                frames[i].y =   values[i];
            }
            std::sort(frames.begin(), frames.end(), [&](const real2& l, const real2& r) 
            {
                return l.x < r.x;
            });
            for (size_t i = 0; i < values.size(); i++)
            {
                times[i]    =   frames[i].x;
                values[i]   =   frames[i].y;
            }
        }
        /// <summary>
        /// 每一帧更新
        /// </summary>
        /// <param name="frame"></param>
        bool        update(const real& tm,FETrackResult& result)
        {
            if (!isValid())
                return  false;
            auto    rng =   range();
            if (tm < rng.x)
                result._value   =   _values->values().front();
            else if(tm > rng.y)
                result._value   =   _values->values().back();
            else
            {
                auto    itr =   std::lower_bound(_times->values().begin(), _times->values().end(), tm, [](const real& l, const real& tm)
                {
                    return l < tm;
                });
                if (itr == _times->values().end())
                {
                    result._value   =   _values->values().back();
                } 
                else if (itr == _times->values().begin())
                {
                    result._value   =   _values->values().front();
                }
                else
                {
                    auto    dist        =   std::distance(_times->values().begin(),itr);
                    auto    itrEnd      =   _values->values().begin() + dist;
                    real2   startFrame  =   real2(*(itr-1), *(itrEnd-1));
                    real2   endFrame    =   real2(*itr,     *itrEnd);
                    result._value       =   interpolate(_type,tm,startFrame,endFrame,_tension);
                }
            } 
            return  true;
        }
    protected:
        /// <summary>
        /// 计算差值
        /// </summary>
        /// <param name="iFrame">帧</param>
        /// <param name="eFrame">另一个关键帧</param>
        /// <param name="tension">贝塞尔插值有效</param>
        /// <returns></returns>
        static  real    interpolate(InterpolateType type,const real& dTime, const real2& startFrame,const real2& endFrame,real tension = MD_EASE) 
        {
            /// 如果时间超过了结束帧范围，使用结束帧值
            if (dTime >= endFrame.x)
                return  endFrame.y;
            /// 如果开始帧与结束帧一样，返回开始帧值
            if (startFrame.x == endFrame.x) 
                return  startFrame.y;
            switch (type)
            {
            case IT_Bezier:
                {
                    real    t   =   real(dTime - startFrame.x) / real(endFrame.x - startFrame.x);
                            t   =   std::clamp(t,0.0,1.0);
                    /// 使用三次贝塞尔实现缓入缓出
                    /// 控制点根据tension参数调整曲线形状
                    real    p0  =   startFrame.y;
                    real    p3  =   endFrame.y;
                    real    p1  =   startFrame.y + (endFrame.y - startFrame.y) * (0.5 - tension * 0.3);
                    real    p2  =   startFrame.y + (endFrame.y - startFrame.y) * (0.5 + tension * 0.3);
                    return  cubicBezierInterpolate(p0,p1,p2,p3,t);
                }
            case IT_Linear:
                {
                    real    t   =   real(dTime - startFrame.x) / real(endFrame.x - startFrame.x);
                    return  (1.0 - t) * startFrame.y + t * endFrame.y;
                }
            case IT_Constant:
                {
                    /// if (dTime >= endFrame.x)
                    ///     return  endFrame.y;
                    return startFrame.y;
                }
            default:
                return startFrame.y;
            }
        }
        /// <summary>
        /// 在两个关键帧之间进行三次贝塞尔插值（最常用）
        /// </summary>
        /// <param name="p0">起始关键帧值</param>
        /// <param name="p1">起始控制点（影响曲线起始方向）</param>
        /// <param name="p2">结束控制点（影响曲线结束方向）</param>
        /// <param name="p3">结束关键帧值</param>
        /// <param name="t">插值参数 [0, 1]</param>
        /// <returns></returns>
        template<class TValue>
        static  TValue  cubicBezierInterpolate(TValue p0, TValue p1, TValue p2, TValue p3, real t) 
        {
            // 三次贝塞尔公式: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
            real    u   =   1.0 - t;
            return  u * u * u * p0 + 
                    3.0 * u * u * t * p1 + 
                    3.0 * u * t * t * p2 + 
                    t * t * t * p3;
        }
    public:
        RealsObject     _times;
        RealsObject     _values;
        real            _tension    =   MD_EASE;
        PropIndex       _propIndex  =   -1;
        InterpolateType _type;
    };

    using   KeyFrameTrack       =   SharedPtr<FEKeyFrameTrack>;
    using   KeyFrameTracks      =   std::vector<KeyFrameTrack>;
}

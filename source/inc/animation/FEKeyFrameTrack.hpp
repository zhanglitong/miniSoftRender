#pragma     once

#include    "../FEPropertyIndex.hpp"
#include    "FETrackResult.hpp"

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
            _type       =   other._type;
        }
        ~FEKeyFrameTrack()  =   default;
    public:
        /// <summary>
        /// 范围
        /// </summary>
        /// <returns></returns>
        real2   range() const
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
        bool    isValid() const
        {
            return (_times && !_times->values().empty());
        }
        /// <summary>
        /// 设置属性索引
        /// </summary>
        /// <param name="index"></param>
        void    setPropertyIndex(const PropIndex& index)
        {
            _propIndex  =   index;
        }
        auto    propertyIndex() const
        {
            return  _propIndex;
        }
        void    setTimeObject(RealsObject timeObject)
        {
            _times  =   timeObject;
        }
        void    setValueObject(ValueObject valueObject)
        {
            _values  =   valueObject;
        }
        /// <summary>
        /// 获取时间信息
        /// </summary>
        /// <returns></returns>
        auto    times() const
        {
            return _times;
        }
        /// <summary>
        /// 获取数据信息
        /// </summary>
        /// <returns></returns>
        auto&   values()
        {
            return _values;
        }
        /// <summary>
        /// 需要手动调用
        /// </summary>
        void    sortKeyFames()
        {
            switch(_values.index())
            {
            case 1:     return  sortImpl(std::get<RealsObject>(_values));
            case 2:     return  sortImpl(std::get<Real2sObject>(_values));
            case 3:     return  sortImpl(std::get<Real3sObject>(_values));
            case 4:     return  sortImpl(std::get<Real4sObject>(_values));
            case 5:     return  sortImpl(std::get<QuatrsObject>(_values));
            case 6:     return  sortImpl(std::get<FloatsObject>(_values));
            case 7:     return  sortImpl(std::get<Float2sObject>(_values));
            case 8:     return  sortImpl(std::get<Float3sObject>(_values));
            case 9:     return  sortImpl(std::get<Float4sObject>(_values));
            case 10:    return  sortImpl(std::get<QuatfsObject>(_values));
            case 11:    return  sortImpl(std::get<BoolsObject>(_values));
            }
        }
        /// <summary>
        /// 每一帧更新
        /// </summary>
        /// <param name="frame"></param>
        bool    update(const real& tm,FETrackResult& result)
        {
            if (!isValid())
                return  false;
            switch(_values.index())
            {
            case 1:     return  updateImpl(tm,result,std::get<RealsObject>(_values));
            case 2:     return  updateImpl(tm,result,std::get<Real2sObject>(_values));
            case 3:     return  updateImpl(tm,result,std::get<Real3sObject>(_values));
            case 4:     return  updateImpl(tm,result,std::get<Real4sObject>(_values));
            case 5:     return  updateImpl(tm,result,std::get<QuatrsObject>(_values));
            case 6:     return  updateImpl(tm,result,std::get<FloatsObject>(_values));
            case 7:     return  updateImpl(tm,result,std::get<Float2sObject>(_values));
            case 8:     return  updateImpl(tm,result,std::get<Float3sObject>(_values));
            case 9:     return  updateImpl(tm,result,std::get<Float4sObject>(_values));
            case 10:    return  updateImpl(tm,result,std::get<QuatfsObject>(_values));
            case 11:    return  updateImpl(tm,result,std::get<BoolsObject>(_values));
            default:    return  false;
            }
        }

        /// <summary>
        /// 每一帧更新
        /// </summary>
        /// <param name="frame"></param>
        bool    update(size_t keyFrame,const real& tm,FETrackResult& result)
        {
            if (!isValid())
                return  false;
            switch(_values.index())
            {
            case 1:     return  updateImpl(keyFrame,tm,result,std::get<RealsObject>(_values));
            case 2:     return  updateImpl(keyFrame,tm,result,std::get<Real2sObject>(_values));
            case 3:     return  updateImpl(keyFrame,tm,result,std::get<Real3sObject>(_values));
            case 4:     return  updateImpl(keyFrame,tm,result,std::get<Real4sObject>(_values));
            case 5:     return  updateImpl(keyFrame,tm,result,std::get<QuatrsObject>(_values));
            case 6:     return  updateImpl(keyFrame,tm,result,std::get<FloatsObject>(_values));
            case 7:     return  updateImpl(keyFrame,tm,result,std::get<Float2sObject>(_values));
            case 8:     return  updateImpl(keyFrame,tm,result,std::get<Float3sObject>(_values));
            case 9:     return  updateImpl(keyFrame,tm,result,std::get<Float4sObject>(_values));
            case 10:    return  updateImpl(keyFrame,tm,result,std::get<QuatfsObject>(_values));
            case 11:    return  updateImpl(keyFrame,tm,result,std::get<BoolsObject>(_values));
            default:    return  false;
            }
        }
    protected:
        template<typename TValueObject>
        void    sortImpl(TValueObject& vObject)
        {
            using   TValue  =   typename TValueObject::TValue::ValueType;

            auto&   times   =   _times->values();
            auto&   values  =   vObject->values();
            assert(times.size() == values.size());
            if (times.size() != values.size())
                return;
            FrameValues frames(values.size());
            for (size_t i = 0; i < values.size(); i++)
            {
                frames[i]._t    =   times[i];
                frames[i]._v    =   values[i];
            }
            std::sort(frames.begin(), frames.end(), [&](const FrameValue& l, const FrameValue& r) 
            {
                return l._t < r._t;
            });
            for (size_t i = 0; i < values.size(); i++)
            {
                times[i]    =   frames[i]._t;
                values[i]   =   std::get<TValue>(frames[i]._v);
            }
        }
        template<typename TValueObject>
        bool    updateImpl(const real& tm,FETrackResult& result,const TValueObject& values)
        {
            auto    rng =   range();
            if (tm <= rng.x)
                result._value   =   values->values().front();
            else if(tm >= rng.y)
                result._value   =   values->values().back();
            else
            {
                auto    itr =   std::lower_bound(_times->values().begin(), _times->values().end(), tm, [](const real& l, const real& tm)
                {
                    return l < tm;
                });
                if (itr == _times->values().end())
                {
                    result._value   =   values->values().back();
                } 
                else if (itr == _times->values().begin())
                {
                    result._value   =   values->values().front();
                }
                else
                {
                    auto        dist        =   std::distance(_times->values().begin(),itr);
                    auto        itrEnd      =   values->values().begin() + dist;
                    FrameValue  startFrame  =   {*(itr-1), *(itrEnd-1)};
                    FrameValue  endFrame    =   {*itr,     *itrEnd};
                    result._value           =   interpolate(_type,tm,startFrame,endFrame,_tension);
                }
            } 
            return  true;
        }

        template<typename TValueObject>
        bool    updateImpl(size_t keyFrame,const real& tm,FETrackResult& result,const TValueObject& vObject)
        {
            auto&   values  =   vObject->values();
            auto&   times   =   _times->values();

            if (keyFrame == 0 && tm == 0)
                result._value   =   values.front();
            else if(keyFrame >= values.size() - 1)
                result._value   =   values.back();

            FrameValue  startFrame  =   {times[keyFrame + 0],values[keyFrame + 0]};
            FrameValue  endFrame    =   {times[keyFrame + 1],values[keyFrame + 1]};
            result._value           =   interpolate(_type,tm,startFrame,endFrame,_tension);
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
        static  KFValue interpolate(InterpolateType type,const real& dTime, const FrameValue& startFrame,const FrameValue& endFrame,real tension = MD_EASE) 
        {
            /// 如果时间超过了结束帧范围，使用结束帧值
            if (dTime >= endFrame._t)
                return  endFrame._v;
            /// 如果开始帧与结束帧一样，返回开始帧值
            if (startFrame._t == endFrame._t) 
                return  startFrame._v;
            switch (type)
            {
            case IT_Bezier:
                {
                    real    t   =   real(dTime - startFrame._t) / real(endFrame._t - startFrame._t);
                            t   =   std::clamp(t,0.0,1.0);
                    /// 使用三次贝塞尔实现缓入缓出
                    /// 控制点根据tension参数调整曲线形状
                    auto    p0  =   startFrame._v;
                    auto    p3  =   endFrame._v;
                    auto    p1  =   startFrame._v + (endFrame._v - startFrame._v) * (0.5 - tension * 0.3);
                    auto    p2  =   startFrame._v + (endFrame._v - startFrame._v) * (0.5 + tension * 0.3);
                    return  cubicBezierInterpolate(p0,p1,p2,p3,t);
                }
            case IT_Linear:
                {
                    real    t   =   real(dTime - startFrame._t) / real(endFrame._t - startFrame._t);
                    if (std::holds_alternative<quatf>(startFrame._v))
                        return  slerp(std::get<quatf>(startFrame._v),std::get<quatf>(endFrame._v),float(t));
                    else if (std::holds_alternative<quatr>(startFrame._v))
                        return  slerp(std::get<quatr>(startFrame._v),std::get<quatr>(endFrame._v),real(t));
                    else
                        return  (1.0 - t) * startFrame._v + t * endFrame._v;
                }
            case IT_Constant:
                return startFrame._v;
            default:
                return startFrame._v;
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
            /// 三次贝塞尔公式: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
            real    u   =   1.0 - t;
            return  u * u * u * p0 + 
                    3.0 * u * u * t * p1 + 
                    3.0 * u * t * t * p2 + 
                    t * t * t * p3;
        }
    public:
        RealsObject     _times;
        ValueObject     _values;
        real            _tension    =   MD_EASE;
        PropIndex       _propIndex  =   -1;
        InterpolateType _type       =   IT_Linear;
    };

    using   KeyFrameTrack       =   SharedPtr<FEKeyFrameTrack>;
    using   KeyFrameTracks      =   std::vector<KeyFrameTrack>;
}

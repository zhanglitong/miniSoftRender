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

    /// <summary>
    /// 设计上要求clip可以被重用，例如车的四个轮子对象,clip是一样的
    /// 但如果每一个轮子开始转动的时间不一样，其他行为一样，需要把track的时间轴局部化,把起始时间作为被动画对象的属性数据
    /// 创建track时候,开始时间都是从0开始
    /// 就可以实现一份数据，在不同的时间上，被多次复用
    /// </summary>

    class   FE_API FEKeyFrameTrack : public FEObject
    {
    public:
        enum    TrackFlag
        {
            /// <summary>
            /// 标记数据变更
            /// </summary>
            TrackChanged    =   FLAG_LAST,
        };
    public:
        struct  KFOff
        {
            size_t  index       =   ~0ULL;
            /// <summary>
            /// 时间线时间
            /// </summary>
            real    clipTm      =   0.0;
            /// <summary>
            /// 相对于当前帧(index) 偏移时间,用来计算插值
            /// </summary>
            real    offsetTm    =   0.0;
        };
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
        FEKeyFrameTrack(FEContext& ctx,PropIndex index = -1);
        FEKeyFrameTrack(const FEKeyFrameTrack& other);
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
        /// 是否发生变更
        /// </summary>
        /// <returns></returns>
        bool    isChanged() const
        {
            return  flags().hasFlag(TrackChanged);
        }
        void    clearChanged() 
        {
            flags().removeFlag(TrackChanged);
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
            flags().addFlag(TrackChanged);
        }
        void    setValueObject(ValueObject valueObject)
        {
            _values  =   valueObject;
            flags().addFlag(TrackChanged);
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
        auto&   values() const
        {
            return _values;
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
        void    sortKeyFames();
        /// <summary>
        /// 计算帧索引偏移,返回帧号 + 相对当前帧的时间偏移
        /// </summary>
        KFOff   calcFrameOffset(const real& clipTime) const;
        /// <summary>
        /// 每一帧更新
        /// </summary>
        /// <param name="frame"></param>
        bool    update(const real& clipTm,FETrackResult& result);
        /// <summary>
        /// 每一帧更新
        /// </summary>
        /// <param name="frame"></param>
        bool    update(const KFOff& kfOff,FETrackResult& result);
    public:
        /// <summary>
        /// 获取依赖的对象,子类实现
        /// </summary>
        /// <param name="uset"></param>
        /// <returns>返回以来的对象个数</returns>
        virtual size_t  queryDepends(ObjectUSet& uSet) const override;
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chunk">数据头，子类可根据chunk._flags字段控制读取</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
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
        bool    updateImpl(const real& clipTime,FETrackResult& result,const TValueObject& values)
        {
            auto    rng =   range();
            if (clipTime <= rng.x)
                result._value   =   values->values().front();
            else if(clipTime >= rng.y)
                result._value   =   values->values().back();
            else
            {
                auto    itr =   std::lower_bound(_times->values().begin(), _times->values().end(), clipTime, [](const real& l, const real& tm)
                {
                    return l < tm;
                });
                auto        dist        =   std::distance(_times->values().begin(),itr);
                auto        itrEnd      =   values->values().begin() + dist;
                FrameValue  startFrame  =   {*(itr-1), *(itrEnd-1)};
                FrameValue  endFrame    =   {*itr,     *itrEnd};
                real    offset          =   real(clipTime - startFrame._t) / real(endFrame._t - startFrame._t);
                        offset          =   std::clamp(offset,0.0,1.0);
                result._value           =   interpolate(_type,offset,startFrame,endFrame,_tension);
            } 
            return  true;
        }

        template<typename TValueObject>
        bool    updateImpl(const KFOff& kfOff,FETrackResult& result,const TValueObject& vObject)
        {
            auto&   values  =   vObject->values();
            auto&   times   =   _times->values();
            
            if (kfOff.clipTm <= times.front())
            {
                result._value   =   values.front();
                return  true;
            } 
            else if(kfOff.clipTm >= times.back())
            {
                result._value   =   values.back();
                return  true;
            }
            else
            {
                FrameValue  startFrame  =   {times[kfOff.index + 0],values[kfOff.index + 0]};
                FrameValue  endFrame    =   {times[kfOff.index + 1],values[kfOff.index + 1]};
                result._value           =   interpolate(_type,kfOff.offsetTm,startFrame,endFrame,_tension);
                return  true;
            }
        }
    protected:

        template<typename TValueObject>
        static  void    serializeValueObject(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx,const TValueObject& values)  
        {
            UNUSED(writer, chunk, version, ctx);
            writer.writeBuffer(values.data(), sizeof(typename TValueObject::value_type) * values.size());
        }
        /// <summary>
        /// 计算差值
        /// </summary>
        /// <param name="iFrame">帧</param>
        /// <param name="eFrame">另一个关键帧</param>
        /// <param name="tension">贝塞尔插值有效</param>
        /// <returns></returns>
        static  KFValue interpolate(InterpolateType type,const real& dTime, const FrameValue& startFrame,const FrameValue& endFrame,real tension = MD_EASE) 
        {
            switch (type)
            {
            case IT_Bezier:
                {
                    /// 使用三次贝塞尔实现缓入缓出
                    /// 控制点根据tension参数调整曲线形状
                    auto    p0  =   startFrame._v;
                    auto    p3  =   endFrame._v;
                    auto    p1  =   startFrame._v + (endFrame._v - startFrame._v) * (0.5 - tension * 0.3);
                    auto    p2  =   startFrame._v + (endFrame._v - startFrame._v) * (0.5 + tension * 0.3);
                    return  cubicBezierInterpolate(p0,p1,p2,p3,dTime);
                }
            case IT_Linear:
                {
                    if (std::holds_alternative<quatf>(startFrame._v))
                        return  slerp(std::get<quatf>(startFrame._v),std::get<quatf>(endFrame._v),float(dTime));
                    else if (std::holds_alternative<quatr>(startFrame._v))
                        return  slerp(std::get<quatr>(startFrame._v),std::get<quatr>(endFrame._v),real(dTime));
                    else
                        return  (1.0 - dTime) * startFrame._v + dTime * endFrame._v;
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

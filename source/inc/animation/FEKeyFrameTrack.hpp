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
        const   auto&   name() const
        {
            return  _name;
        }
        inline  void    setName(const String& name)
        {
            _name   =   name;
        }
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
        /// 添加关键帧
        /// 在指定时间点插入一个关键帧,值为 val
        /// _times 始终保持升序,采用 lower_bound 定位插入位置
        /// 若该时间点已存在关键帧,则返回 false
        /// </summary>
        /// <param name="time">关键帧时间点</param>
        /// <param name="val">关键帧值,类型需与轨道值数组类型一致</param>
        /// <returns>是否添加成功</returns>
        bool    addKeyFrame(const real& time,const KFValue& val)
        {
            if (!_times)
                _times  =   RealsObject(new FERealsObject(ctx()));
            auto&   times   =   _times->values();
            /// _times 有序,使用 lower_bound 二分查找 O(log n)
            auto    itr     =   std::lower_bound(times.begin(),times.end(),time);
            /// 时间点已存在,返回失败
            if (itr != times.end() && *itr == time)
                return  false;
            /// 直接在有序位置插入,无需再 sortKeyFames()
            size_t  idx =   (size_t)std::distance(times.begin(),itr);
            times.insert(itr,time);
            if (!insertValue(idx,val))
            {
                times.erase(times.begin() + idx);
                return  false;
            }
            flags().addFlag(TrackChanged);
            return  true;
        }
        /// <summary>
        /// 在指定索引位置插入关键帧
        /// 与 addKeyFrame 不同,本函数不按时间排序定位,而是直接插入到 idx 位置
        /// 调用方需自行保证插入后 _times 的有序性(若后续依赖二分查找)
        /// idx 允许等于 times.size(),表示追加到末尾
        /// </summary>
        /// <param name="idx">插入位置索引</param>
        /// <param name="time">关键帧时间点</param>
        /// <param name="val">关键帧值,类型需与轨道值数组类型一致</param>
        /// <returns>是否插入成功(索引越界或类型不匹配返回 false)</returns>
        bool    insertKeyFrame(size_t idx,const real& time,const KFValue& val)
        {
            if (!_times)
                _times  =   RealsObject(new FERealsObject(ctx()));
            auto&   times   =   _times->values();
            /// idx 允许等于 size(追加),但不能超过
            if (idx > times.size())
                return  false;
            times.insert(times.begin() + idx,time);
            if (!insertValue(idx,val))
            {
                times.erase(times.begin() + idx);
                return  false;
            }
            flags().addFlag(TrackChanged);
            return  true;
        }
        /// <summary>
        /// 移除指定时间点的关键帧
        /// _times 升序,使用 lower_bound 定位
        /// 同步移除 _times 与值数组中对应索引的元素
        /// </summary>
        /// <param name="time">关键帧时间点</param>
        /// <returns>是否移除成功(时间点不存在返回 false)</returns>
        bool    removeKeyFrame(const real& time)
        {
            if (!_times)
                return  false;
            auto&   times   =   _times->values();
            /// _times 有序,使用 lower_bound 二分查找 O(log n)
            auto    itr     =   std::lower_bound(times.begin(),times.end(),time);
            if (itr == times.end() || *itr != time)
                return  false;
            size_t  idx =   (size_t)std::distance(times.begin(),itr);
            times.erase(itr);
            eraseValue(idx);
            flags().addFlag(TrackChanged);
            return  true;
        }
        /// <summary>
        /// 通过索引移除关键帧
        /// 直接操作 _times 与值数组中指定索引的元素,无需查找时间
        /// </summary>
        /// <param name="idx">关键帧索引</param>
        /// <returns>是否移除成功(索引越界返回 false)</returns>
        bool    removeKeyFrame(size_t idx)
        {
            if (!_times)
                return  false;
            auto&   times   =   _times->values();
            if (idx >= times.size())
                return  false;
            times.erase(times.begin() + idx);
            eraseValue(idx);
            flags().addFlag(TrackChanged);
            return  true;
        }
        /// <summary>
        /// 更新指定时间点的关键帧值
        /// 若该时间点不存在,返回 false
        /// 用 val 覆盖值数组中对应索引处的元素
        /// </summary>
        /// <param name="time">关键帧时间点</param>
        /// <param name="val">新关键帧值,类型需与轨道值数组类型一致</param>
        /// <returns>是否更新成功</returns>
        bool    updateKeyFrame(const real& time,const KFValue& val)
        {
            if (!_times)
                return  false;
            auto&   times   =   _times->values();
            /// _times 有序,使用 lower_bound 二分查找 O(log n)
            auto    itr     =   std::lower_bound(times.begin(),times.end(),time);
            if (itr == times.end() || *itr != time)
                return  false;
            size_t  idx =   (size_t)std::distance(times.begin(),itr);
            if (!replaceValue(idx,val))
                return  false;
            flags().addFlag(TrackChanged);
            return  true;
        }
        /// <summary>
        /// 通过索引更新关键帧值
        /// 直接覆盖值数组中指定索引处的元素,无需查找时间
        /// </summary>
        /// <param name="idx">关键帧索引</param>
        /// <param name="val">新关键帧值,类型需与轨道值数组类型一致</param>
        /// <returns>是否更新成功(索引越界或类型不匹配返回 false)</returns>
        bool    updateKeyFrame(size_t idx,const KFValue& val)
        {
            if (!_times)
                return  false;
            auto&   times   =   _times->values();
            if (idx >= times.size())
                return  false;
            if (!replaceValue(idx,val))
                return  false;
            flags().addFlag(TrackChanged);
            return  true;
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

        /// <summary>
        /// 将 KFValue 追加到当前轨道的值数组末尾
        /// 要求 KFValue 持有的类型与 _values variant 的当前类型一致
        /// </summary>
        /// <param name="val">关键帧值</param>
        /// <returns>是否成功写入</returns>
        bool    pushValue(const KFValue& val)
        {
            bool    ok  =   false;
            std::visit([this,&ok](auto&& v)
            {
                using   T   =   std::decay_t<decltype(v)>;
                if constexpr      (std::is_same_v<T,real>)    { if(std::holds_alternative<RealsObject>(_values))   { std::get<RealsObject>(_values)->values().push_back(v);   ok=true; } }
                else if constexpr (std::is_same_v<T,real2>)   { if(std::holds_alternative<Real2sObject>(_values))  { std::get<Real2sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,real3>)   { if(std::holds_alternative<Real3sObject>(_values))  { std::get<Real3sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,real4>)   { if(std::holds_alternative<Real4sObject>(_values))  { std::get<Real4sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,quatr>)   { if(std::holds_alternative<QuatrsObject>(_values))  { std::get<QuatrsObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,float>)   { if(std::holds_alternative<FloatsObject>(_values))  { std::get<FloatsObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,float2>)  { if(std::holds_alternative<Float2sObject>(_values)) { std::get<Float2sObject>(_values)->values().push_back(v); ok=true; } }
                else if constexpr (std::is_same_v<T,float3>)  { if(std::holds_alternative<Float3sObject>(_values)) { std::get<Float3sObject>(_values)->values().push_back(v); ok=true; } }
                else if constexpr (std::is_same_v<T,float4>)  { if(std::holds_alternative<Float4sObject>(_values)) { std::get<Float4sObject>(_values)->values().push_back(v); ok=true; } }
                else if constexpr (std::is_same_v<T,quatf>)   { if(std::holds_alternative<QuatfsObject>(_values))  { std::get<QuatfsObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint8>)   { if(std::holds_alternative<BoolsObject>(_values))   { std::get<BoolsObject>(_values)->values().push_back(v);   ok=true; }
                                                                 else if(std::holds_alternative<AlphaObject>(_values)){ std::get<AlphaObject>(_values)->values().push_back(v);ok=true; } }
                else if constexpr (std::is_same_v<T,uint8x4>) { if(std::holds_alternative<RgbaObject>(_values))    { std::get<RgbaObject>(_values)->values().push_back(v);    ok=true; } }
                else if constexpr (std::is_same_v<T,int>)     { if(std::holds_alternative<IntsObject>(_values))   { std::get<IntsObject>(_values)->values().push_back(v);   ok=true; } }
                else if constexpr (std::is_same_v<T,int2>)    { if(std::holds_alternative<Int2sObject>(_values))  { std::get<Int2sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,int3>)    { if(std::holds_alternative<Int3sObject>(_values))  { std::get<Int3sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,int4>)    { if(std::holds_alternative<Int4sObject>(_values))  { std::get<Int4sObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint>)    { if(std::holds_alternative<UintsObject>(_values))  { std::get<UintsObject>(_values)->values().push_back(v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint2>)   { if(std::holds_alternative<Uint2sObject>(_values)) { std::get<Uint2sObject>(_values)->values().push_back(v); ok=true; } }
                else if constexpr (std::is_same_v<T,uint3>)   { if(std::holds_alternative<Uint3sObject>(_values)) { std::get<Uint3sObject>(_values)->values().push_back(v); ok=true; } }
                else if constexpr (std::is_same_v<T,uint4>)   { if(std::holds_alternative<Uint4sObject>(_values)) { std::get<Uint4sObject>(_values)->values().push_back(v); ok=true; } }
            }, val);
            return  ok;
        }
        /// <summary>
        /// 在值数组指定索引处插入 KFValue,与 _times 保持同步
        /// 要求 KFValue 持有的类型与 _values variant 的当前类型一致
        /// </summary>
        /// <param name="idx">插入位置索引</param>
        /// <param name="val">关键帧值</param>
        /// <returns>是否成功写入</returns>
        bool    insertValue(size_t idx,const KFValue& val)
        {
            bool    ok  =   false;
            std::visit([this,idx,&ok](auto&& v)
            {
                using   T   =   std::decay_t<decltype(v)>;
                if constexpr      (std::is_same_v<T,real>)    { if(std::holds_alternative<RealsObject>(_values))   { auto& arr=std::get<RealsObject>(_values)->values();   arr.insert(arr.begin()+idx,v);   ok=true; } }
                else if constexpr (std::is_same_v<T,real2>)   { if(std::holds_alternative<Real2sObject>(_values))  { auto& arr=std::get<Real2sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,real3>)   { if(std::holds_alternative<Real3sObject>(_values))  { auto& arr=std::get<Real3sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,real4>)   { if(std::holds_alternative<Real4sObject>(_values))  { auto& arr=std::get<Real4sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,quatr>)   { if(std::holds_alternative<QuatrsObject>(_values))  { auto& arr=std::get<QuatrsObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,float>)   { if(std::holds_alternative<FloatsObject>(_values))  { auto& arr=std::get<FloatsObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,float2>)  { if(std::holds_alternative<Float2sObject>(_values)) { auto& arr=std::get<Float2sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
                else if constexpr (std::is_same_v<T,float3>)  { if(std::holds_alternative<Float3sObject>(_values)) { auto& arr=std::get<Float3sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
                else if constexpr (std::is_same_v<T,float4>)  { if(std::holds_alternative<Float4sObject>(_values)) { auto& arr=std::get<Float4sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
                else if constexpr (std::is_same_v<T,quatf>)   { if(std::holds_alternative<QuatfsObject>(_values))  { auto& arr=std::get<QuatfsObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint8>)   { if(std::holds_alternative<BoolsObject>(_values))   { auto& arr=std::get<BoolsObject>(_values)->values();   arr.insert(arr.begin()+idx,v);   ok=true; }
                                                                 else if(std::holds_alternative<AlphaObject>(_values)){ auto& arr=std::get<AlphaObject>(_values)->values();arr.insert(arr.begin()+idx,v);ok=true; } }
                else if constexpr (std::is_same_v<T,uint8x4>) { if(std::holds_alternative<RgbaObject>(_values))    { auto& arr=std::get<RgbaObject>(_values)->values();    arr.insert(arr.begin()+idx,v);    ok=true; } }
                else if constexpr (std::is_same_v<T,int>)     { if(std::holds_alternative<IntsObject>(_values))   { auto& arr=std::get<IntsObject>(_values)->values();   arr.insert(arr.begin()+idx,v);   ok=true; } }
                else if constexpr (std::is_same_v<T,int2>)    { if(std::holds_alternative<Int2sObject>(_values))  { auto& arr=std::get<Int2sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,int3>)    { if(std::holds_alternative<Int3sObject>(_values))  { auto& arr=std::get<Int3sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,int4>)    { if(std::holds_alternative<Int4sObject>(_values))  { auto& arr=std::get<Int4sObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint>)    { if(std::holds_alternative<UintsObject>(_values))  { auto& arr=std::get<UintsObject>(_values)->values();  arr.insert(arr.begin()+idx,v);  ok=true; } }
                else if constexpr (std::is_same_v<T,uint2>)   { if(std::holds_alternative<Uint2sObject>(_values)) { auto& arr=std::get<Uint2sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
                else if constexpr (std::is_same_v<T,uint3>)   { if(std::holds_alternative<Uint3sObject>(_values)) { auto& arr=std::get<Uint3sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
                else if constexpr (std::is_same_v<T,uint4>)   { if(std::holds_alternative<Uint4sObject>(_values)) { auto& arr=std::get<Uint4sObject>(_values)->values(); arr.insert(arr.begin()+idx,v); ok=true; } }
            }, val);
            return  ok;
        }
        /// <summary>
        /// 删除值数组中指定索引的元素,与 _times 保持同步
        /// </summary>
        /// <param name="idx">索引</param>
        void    eraseValue(size_t idx)
        {
            switch (_values.index())
            {
            case 1:     std::get<1>(_values)->values().erase(std::get<1>(_values)->values().begin() + idx);   break;
            case 2:     std::get<2>(_values)->values().erase(std::get<2>(_values)->values().begin() + idx);   break;
            case 3:     std::get<3>(_values)->values().erase(std::get<3>(_values)->values().begin() + idx);   break;
            case 4:     std::get<4>(_values)->values().erase(std::get<4>(_values)->values().begin() + idx);   break;
            case 5:     std::get<5>(_values)->values().erase(std::get<5>(_values)->values().begin() + idx);   break;
            case 6:     std::get<6>(_values)->values().erase(std::get<6>(_values)->values().begin() + idx);   break;
            case 7:     std::get<7>(_values)->values().erase(std::get<7>(_values)->values().begin() + idx);   break;
            case 8:     std::get<8>(_values)->values().erase(std::get<8>(_values)->values().begin() + idx);   break;
            case 9:     std::get<9>(_values)->values().erase(std::get<9>(_values)->values().begin() + idx);   break;
            case 10:    std::get<10>(_values)->values().erase(std::get<10>(_values)->values().begin() + idx); break;
            case 11:    std::get<11>(_values)->values().erase(std::get<11>(_values)->values().begin() + idx); break;
            case 12:    std::get<12>(_values)->values().erase(std::get<12>(_values)->values().begin() + idx); break;
            case 13:    std::get<13>(_values)->values().erase(std::get<13>(_values)->values().begin() + idx); break;
            case 14:    std::get<14>(_values)->values().erase(std::get<14>(_values)->values().begin() + idx); break;
            case 15:    std::get<15>(_values)->values().erase(std::get<15>(_values)->values().begin() + idx); break;
            case 16:    std::get<16>(_values)->values().erase(std::get<16>(_values)->values().begin() + idx); break;
            case 17:    std::get<17>(_values)->values().erase(std::get<17>(_values)->values().begin() + idx); break;
            case 18:    std::get<18>(_values)->values().erase(std::get<18>(_values)->values().begin() + idx); break;
            case 19:    std::get<19>(_values)->values().erase(std::get<19>(_values)->values().begin() + idx); break;
            case 20:    std::get<20>(_values)->values().erase(std::get<20>(_values)->values().begin() + idx); break;
            case 21:    std::get<21>(_values)->values().erase(std::get<21>(_values)->values().begin() + idx); break;
            default:    break;
            }
        }
        /// <summary>
        /// 用 val 覆盖值数组中指定索引处的元素
        /// 要求 KFValue 持有的类型与 _values variant 的当前类型一致
        /// </summary>
        /// <param name="idx">索引</param>
        /// <param name="val">新值</param>
        /// <returns>是否成功写入</returns>
        bool    replaceValue(size_t idx,const KFValue& val)
        {
            bool    ok  =   false;
            std::visit([this,idx,&ok](auto&& v)
            {
                using   T   =   std::decay_t<decltype(v)>;
                if constexpr      (std::is_same_v<T,real>)    { if(std::holds_alternative<RealsObject>(_values))   { std::get<RealsObject>(_values)->values()[idx] = v;   ok=true; } }
                else if constexpr (std::is_same_v<T,real2>)   { if(std::holds_alternative<Real2sObject>(_values))  { std::get<Real2sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,real3>)   { if(std::holds_alternative<Real3sObject>(_values))  { std::get<Real3sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,real4>)   { if(std::holds_alternative<Real4sObject>(_values))  { std::get<Real4sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,quatr>)   { if(std::holds_alternative<QuatrsObject>(_values))  { std::get<QuatrsObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,float>)   { if(std::holds_alternative<FloatsObject>(_values))  { std::get<FloatsObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,float2>)  { if(std::holds_alternative<Float2sObject>(_values)) { std::get<Float2sObject>(_values)->values()[idx] = v; ok=true; } }
                else if constexpr (std::is_same_v<T,float3>)  { if(std::holds_alternative<Float3sObject>(_values)) { std::get<Float3sObject>(_values)->values()[idx] = v; ok=true; } }
                else if constexpr (std::is_same_v<T,float4>)  { if(std::holds_alternative<Float4sObject>(_values)) { std::get<Float4sObject>(_values)->values()[idx] = v; ok=true; } }
                else if constexpr (std::is_same_v<T,quatf>)   { if(std::holds_alternative<QuatfsObject>(_values))  { std::get<QuatfsObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,uint8>)   { if(std::holds_alternative<BoolsObject>(_values))   { std::get<BoolsObject>(_values)->values()[idx] = v;   ok=true; }
                                                                 else if(std::holds_alternative<AlphaObject>(_values)){ std::get<AlphaObject>(_values)->values()[idx] = v;ok=true; } }
                else if constexpr (std::is_same_v<T,uint8x4>) { if(std::holds_alternative<RgbaObject>(_values))    { std::get<RgbaObject>(_values)->values()[idx] = v;    ok=true; } }
                else if constexpr (std::is_same_v<T,int>)     { if(std::holds_alternative<IntsObject>(_values))   { std::get<IntsObject>(_values)->values()[idx] = v;   ok=true; } }
                else if constexpr (std::is_same_v<T,int2>)    { if(std::holds_alternative<Int2sObject>(_values))  { std::get<Int2sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,int3>)    { if(std::holds_alternative<Int3sObject>(_values))  { std::get<Int3sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,int4>)    { if(std::holds_alternative<Int4sObject>(_values))  { std::get<Int4sObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,uint>)    { if(std::holds_alternative<UintsObject>(_values))  { std::get<UintsObject>(_values)->values()[idx] = v;  ok=true; } }
                else if constexpr (std::is_same_v<T,uint2>)   { if(std::holds_alternative<Uint2sObject>(_values)) { std::get<Uint2sObject>(_values)->values()[idx] = v; ok=true; } }
                else if constexpr (std::is_same_v<T,uint3>)   { if(std::holds_alternative<Uint3sObject>(_values)) { std::get<Uint3sObject>(_values)->values()[idx] = v; ok=true; } }
                else if constexpr (std::is_same_v<T,uint4>)   { if(std::holds_alternative<Uint4sObject>(_values)) { std::get<Uint4sObject>(_values)->values()[idx] = v; ok=true; } }
            }, val);
            return  ok;
        }

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
        /// <summary>
        /// 名称，显示使用
        /// </summary>
        String          _name;
    };

    using   KeyFrameTrack       =   SharedPtr<FEKeyFrameTrack>;
    using   KeyFrameTracks      =   std::vector<KeyFrameTrack>;
}

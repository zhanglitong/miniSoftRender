#pragma     once
#include    <functional>
#include    "FEAnimClip.hpp"
#include    "FEAnimation.hpp"
#include    "FEObjectsTemplate.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAction, "{0AFD2EDB-C83A-49F4-ACC6-52E6060D54E4}");

    /// lambda,按照动画的在时间线上的偏移量,从小到大排序
    constexpr   auto    AnimationLessFunc = [](const Animation& l, const Animation& r)
    {
        return  l->offset() < r->offset();
    };
    /// 推导出来类型
    using   AnimationLess   =   decltype(AnimationLessFunc);

    /// <summary>
    /// 用来控制动画的播放
    /// 还没有想好如何统一编辑模式与非编辑模式
    /// TODO: 本质上是: 数据发生变化后如何重构cache,避免脏数据
    /// </summary>
    class   FE_API FEAction 
        : public FEObject
        , public FEObjectsTemplate<Animation, AnimationLess>
    {
    public:
        struct  TrackObject
        {
            /// <summary>
            /// keyframe track
            /// </summary>
            KeyFrameTrack   _track;
            /// <summary>
            /// 应用到哪个对象上
            /// </summary>
            Object          _owner;
            /// <summary>
            /// 开始播放时间
            /// </summary>
            real            _offTime;
        };
        using   TrackObjects    =   std::vector<TrackObject>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEAction)
    public:
        enum    PlayMode :uint8_t
        {
            /// <summary>
            /// 播放一次
            /// </summary>
            PT_Once,
            /// <summary>
            /// 循环播放，可设置循环次数
            /// </summary>
            PT_Loop,
            /// <summary>
            /// 乒乓循环模式，可以设置循环次数
            /// </summary>
            PT_PingPong
        };
        enum    TimeMode:uint8_t
        {
            /// <summary>
            /// 正向播放
            /// </summary>
            TM_Default,
            /// <summary>
            /// 倒放
            /// </summary>
            TM_Invert
        };
        enum    PlayStatus:uint8_t
        {
            PS_Running  ,
            PS_Stoped 
        };
        using   Notify  =   std::function<void(FEAction*)>;
        using   Notifys =   std::map<void*,Notify>;
    public:
        FEAction(FEContext& ctx)  ;
        FEAction(const FEAction& other);
        ~FEAction() ;
    public:
        /// <summary>
        /// 编辑模式下,性能较差
        /// </summary>
        /// <returns></returns>
        bool        isCacheMode() const
        {
            return  !_cache.empty();
        }
        /// <summary>
        /// 生成cache,可以提升动画的播放性能,但会影响编辑状态
        /// 所以典型的应用场景是: 在浏览播放动画场景中使用
        /// </summary>
        void        buildCache(); 
        /// <summary>
        /// 清除cache,动画更新过程会恢复到普通模式,性能会有所降低
        /// </summary>
        void        clearCache();
        /// <summary>
        /// 刷新动画范围,在编辑模式下修改关键帧后调用
        /// 重算_range并清空cache,确保后续setClipTime/update使用最新数据
        /// </summary>
        void        refreshRange()
        {
            _range  =   calcRange();
            clearCache();
        }
        /// <summary>
        /// 播放
        /// </summary>
        void        play()
        {
            _status =   PS_Running;
        }
        /// <summary>
        /// 停止
        /// </summary>
        void        stop()
        {
            _status =   PS_Stoped;
        }
       
        /// <summary>
        /// 播放状态
        /// </summary>
        /// <returns></returns>
        auto        status() const
        {
            return  _status;
        }
        /// <summary>
        /// 更新
        /// </summary>
        /// <param name="delta">帧循环时间</param>
        void        update(const real& delta);
        /// <summary>
        /// 设置时间,动画直接跳转到对应的时间上
        /// </summary>
        /// <param name="tm"></param>
        void        setClipTime(const real& clipTime);
    protected:
        /// <summary>
        /// 相同时间线同时计算
        /// </summary>
        /// <param name="delta"></param>
        void        updateBatch(const real& clipTime,const real& delta); 
        /// <summary>
        /// 添加对象通知
        /// </summary>
        virtual void    onAddObject(Animation anim) override
        {
            UNUSED(anim);
            _range  =   calcRange();
        }
        /// <summary>
        /// 移除对象通知
        /// </summary>
        virtual void    onRemoveObject(Animation anim) override
        {
            UNUSED(anim);
            _range  =   calcRange();
        }
        /// <summary>
        /// 计算范围
        /// </summary>
        /// <returns></returns>
        real2       calcRange();
    protected:
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
        PlayMode        _playMode;
        TimeMode        _timeMode;
        PlayStatus      _status;
        /// <summary>
        /// 从开始播放到停止经过过的所有时间和,真实时间
        /// </summary>
        real            _elapseTime;
        /// <summary>
        /// 时间线缩放，快放，慢放
        /// </summary>
        real            _timeScale;
        /// <summary>
        /// 从播放开始计算,不是真实的时间，是逻辑时间，带有缩放控制信息
        /// </summary>
        real            _clipTime;
        /// <summary>
        /// 帧时间范围
        /// </summary>
        real2           _range;
        TrackResults    _result; 
        /// <summary>
        /// 缓冲所有_tracks / object;
        /// animtion 贡献 owner
        /// clip 贡献 track
        /// </summary>
        TrackObjects    _cache;
    };

    using   Action      =   SharedPtr<FEAction>;
    using   Actions     =   std::vector<Action>;
}

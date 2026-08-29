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
    class   FEAction 
        : public FEObject
        , public FEObjectsTemplate<Animation, AnimationLess>
    {
    public:
        enum    ActionFlag
        {
            /// <summary>
            /// 标记数据变更
            /// </summary>
            FLAG_EDIT_MODE    =   FLAG_LAST,
        };
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
            PS_Pause    ,
            PS_Stoped 
        };
        using   Notify  =   std::function<void(FEAction*)>;
        using   Notifys =   std::map<void*,Notify>;
    public:
        FEAction(FEContext& ctx)  
            :FEObject(ctx)
            ,FEObjectsTemplate<Animation, AnimationLess>(AnimationLessFunc)
        {
            _playMode       =   PT_Loop;
            _timeMode       =   TM_Default;
            _status         =   PS_Running;
            _elapseTime     =   0.0;
            _timeScale      =   1;
            _clipTime       =   0;
            _range          =   {};
            flags().removeFlag(FLAG_EDIT_MODE);
        }
        FEAction(const FEAction& other)
            :FEObject(other)
            ,FEObjectsTemplate<Animation, AnimationLess>(other)
        {
            _playMode       =   other._playMode;
            _timeMode       =   other._timeMode;
            _status         =   other._status;
            _elapseTime     =   other._elapseTime;
            _timeScale      =   other._timeScale;
            _clipTime       =   other._clipTime;
            _range          =   other._range;
            setEditMode(other.isEditMode());
        }
        ~FEAction()   =   default;
    public:
        /// <summary>
        /// 编辑模式下,性能较差
        /// </summary>
        /// <returns></returns>
        bool        isEditMode() const
        {
            return  flags().hasFlag(FLAG_EDIT_MODE);
        }
        /// <summary>
        /// 设置是否是编辑模式
        /// </summary>
        /// <param name="bFlag"></param>
        void        setEditMode(bool bFlag)
        {
            if (bFlag)
                flags().addFlag(FLAG_EDIT_MODE);
            else
                flags().removeFlag(FLAG_EDIT_MODE);
            _cache.clear();
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
        /// 暂停
        /// </summary>
        void        pause()
        {
            _status =   PS_Pause;
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
        void        update(const real& delta)
        {
            if (_status != PS_Running || _objects.empty() )
                return;
            _elapseTime +=  delta;
            real    deltaTime   =   delta * _timeScale;
            switch(_timeMode)
            {
            case TM_Default:
                _clipTime   +=  deltaTime;
                break;
            case TM_Invert:
                _clipTime   -=  deltaTime;
                break;
            }
            switch (_playMode)
            {
            case PT_Once:
                {
                    if (_clipTime > _range.y)
                    {
                        _clipTime   =   _range.y;
                        _status     =   PS_Stoped;
                    }   
                    else if (_clipTime < _range.x)
                    {
                        _clipTime   =   _range.x;
                        _status     =   PS_Stoped;
                    }
                }
                break;
            case PT_Loop:
                {
                    if (_clipTime > _range.y)
                    {
                        _clipTime   =   _range.x;
                    }
                    else if (_clipTime < _range.x)
                    {
                        _clipTime   =   _range.y;
                    }
                }
                break;
            case PT_PingPong:
                {
                    if (_timeMode == TM_Default && _clipTime > _range.y)
                    {
                        _clipTime   =   _range.y;
                        _timeMode   =   TM_Invert;
                    }
                    else if (_timeMode == TM_Invert && _clipTime < _range.x)
                    {
                        _clipTime   =   _range.x;
                        _timeMode   =   TM_Default;
                    }
                }
                break;
            }
            if (flags().hasFlag(FLAG_EDIT_MODE))
            {
                for (auto& var : _objects)
                {
                    /// 如果没有到时间,不播放,避免内存有更多检测
                    /// 注意这里不能用
                    real2   range   =   var->range();
                    /// range.x - delta 上一帧花费的时间，避免/丢帧/跳帧
                    /// range.y + delta 上一帧花费的时间，避免/丢帧/跳帧
                    if (_clipTime < range.x - delta || _clipTime > range.y + delta)
                        continue;
                    var->update(_clipTime);
                }
            }
            else
            {
                updateBatch(_clipTime,delta);
            }
        }

        /// <summary>
        /// 相同时间线同时计算
        /// </summary>
        /// <param name="delta"></param>
        void        updateBatch(const real& clipTime,const real& delta)
        {
            if (_cache.empty())
            {
                buildCache();
            }
            ///---相同时间 ---- 相同时间线--- 相同对象---
            RealsObject             timeLine    =   nullptr;
            FEKeyFrameTrack::KFOff  kfValue     =   {};
            Object                  owner       =   nullptr;
            bool                    bNotify     =   false;
            for (auto& var : _cache)
            {
                auto    track   =   var._track;
                auto    range   =   track->range() + real2(var._offTime);
                if (clipTime < (range.x - delta) || clipTime > range.y + delta)
                    continue;
                if (track->times() != timeLine)
                {
                    timeLine    =   track->times();
                    kfValue     =   track->calcFrameOffset(clipTime - var._offTime);
                }
                if (owner != var._owner)
                {
                    if (owner)
                        owner->endSetProp(bNotify);
                    owner   =   var._owner;
                    if (owner)
                        owner->beginSetProp();
                    bNotify =   false;
                }
                FETrackResult   result;
                result._track   =   track;
                result._prop    =   track->propertyIndex();
                result._valid   =   track->update(kfValue,result);
                bNotify         |=   var._owner->setProperty(result._prop ,result._value);
            }
            if (owner)
                owner->endSetProp(bNotify);
        }
    protected:  
        virtual void    buildCache()
        {
            size_t  cnt =   0;
            for (auto& var : _objects)
            {
                cnt +=  var->clip()->objects().size();
            }
            _cache.reserve(cnt);
            for (auto& var : _objects)
            {
                auto&   tracks  =   var->clip()->objects();
                auto    owner   =   var->owner();
                real    offTime =   var->offset();
                for (auto& track : tracks)
                {
                    TrackObject obj =   {track,owner,offTime};
                    _cache.emplace_back(obj);
                }
            }
            /// 先按照时间排序
            /// 在按照时间线对象排序,最后 同一时间段.相同timeLine对象都集中在一起了,在按照对象排序
            std::sort(_cache.begin(),_cache.end(),[](const TrackObject& l,const TrackObject& r)
            {
                if (l._offTime != r._offTime)
                    return  l._offTime < r._offTime;
                else
                    if(l._track->times().get() != r._track->times().get())
                        return  l._track->times().get() < r._track->times().get();
                    else
                        return  l._owner.get() < r._owner.get();
                    
            });
        }   
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
        real2       calcRange()
        {
            real2   result(-1,-1);
            if (_objects.empty())
                return  result;
            else
                result  =   _objects.front()->clip()->range();

            for (size_t i = 1 ;i < _objects.size(); ++ i)
            {
                auto    rng =   _objects[i]->clip()->range();
                result.x    =   (std::min)(result.x,rng.x);
                result.y    =   (std::max)(result.y,rng.y);
            }
            return   result;
        }
    public:
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

#pragma     once
#include    <functional>
#include    "FEClip.hpp"
#include    "FEAnimation.hpp"
#include    "FEObjectsTemplate.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAction, "{0AFD2EDB-C83A-49F4-ACC6-52E6060D54E4}");

    /// lambda 表达式作为类型别名，确保比较方法
    constexpr   auto    AnimationLessFunc = [](const Animation& l, const Animation& r)
    {
        return  l < r ;
    };
    /// 先定义类型别名，确保RFactoryLess是类型名
    using   AnimationLess   =   decltype(AnimationLessFunc);

    /// <summary>
    /// action 用来控制动画的播放与停止
    /// 一个action 中可以控制多个动画的播放，可以认为是对动画的分组控制
    /// </summary>
    class   FEAction 
        : public FEObject
        , public FEObjectsTemplate<Animation, AnimationLess>
    {
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
            _playMode       =   PT_Once;
            _timeMode       =   TM_Default;
            _status         =   PS_Stoped;
            _startTime      =   0.0;
            _clipFrame      =   0;
            _weight         =   1;
            _timeScale      =   1;
            _clipTime       =   0;
            _range          =   {};
        }
        FEAction(const FEAction& other)
            :FEObject(other)
            ,FEObjectsTemplate<Animation, AnimationLess>(other)
        {
            _playMode       =   other._playMode;
            _timeMode       =   other._timeMode;
            _status         =   other._status;
            _startTime      =   other._startTime;
            _clipFrame      =   other._clipFrame;
            _weight         =   other._weight;
            _timeScale      =   other._timeScale;
            _clipTime       =   other._clipTime;
            _range          =   other._range;
        }
        ~FEAction()   =   default;
    public:
        /// <summary>
        /// 播放动画
        /// </summary>
        void        play()
        {
            _status =   PS_Running;
        }
        /// <summary>
        /// 停止播放
        /// </summary>
        void        stop()
        {
            _status =   PS_Stoped;
        }
        /// <summary>
        /// 暂停播放
        /// </summary>
        void        pause()
        {
            _status =   PS_Pause;
        }
        /// <summary>
        /// 获取状态
        /// </summary>
        /// <returns></returns>
        auto        status() const
        {
            return  _status;
        }
        /// <summary>
        /// 更新
        /// </summary>
        /// <param name="frame"></param>
        void        update(const real& delta)
        {
            if (_status != PS_Running || _objects.empty() )
                return;
            _elapseTime +=  delta;
            if (_elapseTime < _startTime)
                return;
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
            _clipFrame  =   _clipTime * _fps;

            switch (_playMode)
            {
            case PT_Once:
                {
                    if (_clipFrame > _range.y)
                    {
                        _clipFrame  =   _range.y;
                        _clipTime   =   real(_clipFrame)/real(_fps);
                        _status     =   PS_Stoped;
                    }   
                    else if (_clipFrame < _range.x)
                    {
                        _clipFrame  =   _range.x;
                        _clipTime   =   real(_clipFrame)/real(_fps);
                        _status     =   PS_Stoped;
                    }
                }
                break;
            case PT_Loop:
                {
                    if (_clipFrame > _range.y)
                    {
                        _clipFrame  =   _range.x;
                        _clipTime   =   real(_clipFrame)/real(_fps);
                    }
                    else if (_clipFrame < _range.x)
                    {
                        _clipFrame  =   _range.y;
                        _clipTime   =   real(_clipFrame)/real(_fps);
                    }
                }
                break;
            case PT_PingPong:
                break;
            }
            for (auto& var :_objects)
            {
                var->update(_clipFrame);
            }
        }
    protected:
        /// <summary>
        /// 子类可以重写，添加对象的回调函数
        /// </summary>
        virtual void    onAddObjects()
        {
            _range  =   calcRange();
        }
        /// <summary>
        /// 移除对象通知，子类重写
        /// </summary>
        virtual void    onRemoveObjects()
        {
            _range  =   calcRange();
        }
        /// <summary>
        /// 计算帧范围
        /// </summary>
        /// <returns></returns>
        uint2       calcRange()
        {
            uint2   result(0,0);
            for (auto& var : _objects)
            {
                auto    rng =   var->clip()->range();
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
        /// 帧率,每秒播放动画的帧率
        /// </summary>
        uint            _fps    =   30;
        /// <summary>
        /// 根据 _clipTime 计算出来的实际帧
        /// </summary>
        real            _clipFrame;
        /// <summary>
        /// 从什么时间开始播放,当调用了play函数后，开始计算
        /// </summary>
        real            _startTime;
        /// <summary>
        /// 从开始播放到停止经过过的所有时间和
        /// </summary>
        real            _elapseTime;
        real            _weight;
        /// <summary>
        /// 时间线缩放，快放，慢放
        /// </summary>
        real            _timeScale;
        /// <summary>
        /// 从0开始计算
        /// </summary>
        real            _clipTime;
        /// <summary>
        /// 帧范围
        /// </summary>
        uint2           _range;
        TrackResults    _result; 
    };

    using   Action      =   SharedPtr<FEAction>;
    using   Actions     =   std::vector<Action>;
}

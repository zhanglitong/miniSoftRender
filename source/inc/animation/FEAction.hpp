#pragma     once
#include    <functional>
#include    "FEAnimClip.hpp"
#include    "FEAnimation.hpp"
#include    "FEObjectsTemplate.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAction, "{0AFD2EDB-C83A-49F4-ACC6-52E6060D54E4}");

    /// lambda 琛ㄨ揪寮忎綔涓虹被鍨嬪埆鍚嶏紝纭繚姣旇緝鏂规硶
    constexpr   auto    AnimationLessFunc = [](const Animation& l, const Animation& r)
    {
        return  l < r ;
    };
    /// 鍏堝畾涔夌被鍨嬪埆鍚嶏紝纭繚RFactoryLess鏄被鍨嬪悕
    using   AnimationLess   =   decltype(AnimationLessFunc);

    /// <summary>
    /// action 鐢ㄦ潵鎺у埗鍔ㄧ敾鐨勬挱鏀句笌鍋滄
    /// 涓€涓猘ction 涓彲浠ユ帶鍒跺涓姩鐢荤殑鎾斁锛屽彲浠ヨ涓烘槸瀵瑰姩鐢荤殑鍒嗙粍鎺у埗
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
            _playMode       =   PT_Loop;
            _timeMode       =   TM_Default;
            _status         =   PS_Running;
            _elapseTime     =   0.0;
            _startTime      =   0.0;
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
            _elapseTime     =   other._elapseTime;
            _startTime      =   other._startTime;
            _timeScale      =   other._timeScale;
            _clipTime       =   other._clipTime;
            _range          =   other._range;
        }
        ~FEAction()   =   default;
    public:
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
        /// 鏇存柊
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
            for (auto& var :_objects)
            {
                var->update(_clipTime);
            }
        }
    protected:
        /// <summary>
        /// 瀛愮被鍙互閲嶅啓锛屾坊鍔犲璞＄殑鍥炶皟鍑芥暟
        /// </summary>
        virtual void    onAddObjects()
        {
            _range  =   calcRange();
        }
        /// <summary>
        /// 绉婚櫎瀵硅薄閫氱煡锛屽瓙绫婚噸鍐?
        /// </summary>
        virtual void    onRemoveObjects()
        {
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
        /// 从什么时间开始播放,当调用了play函数后，开始计算
        /// </summary>
        real            _startTime;
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
    };

    using   Action      =   SharedPtr<FEAction>;
    using   Actions     =   std::vector<Action>;
}

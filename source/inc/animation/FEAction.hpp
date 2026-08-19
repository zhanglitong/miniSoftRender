#pragma     once
#include    <functional>
#include    "FEClip.hpp"
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
            /// 鎾斁涓€娆?
            /// </summary>
            PT_Once,
            /// <summary>
            /// 寰幆鎾斁锛屽彲璁剧疆寰幆娆℃暟
            /// </summary>
            PT_Loop,
            /// <summary>
            /// 涔掍箵寰幆妯″紡锛屽彲浠ヨ缃惊鐜鏁?
            /// </summary>
            PT_PingPong
        };
        enum    TimeMode:uint8_t
        {
            /// <summary>
            /// 姝ｅ悜鎾斁
            /// </summary>
            TM_Default,
            /// <summary>
            /// 鍊掓斁
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
        /// 鎾斁鍔ㄧ敾
        /// </summary>
        void        play()
        {
            _status =   PS_Running;
        }
        /// <summary>
        /// 鍋滄鎾斁
        /// </summary>
        void        stop()
        {
            _status =   PS_Stoped;
        }
        /// <summary>
        /// 鏆傚仠鎾斁
        /// </summary>
        void        pause()
        {
            _status =   PS_Pause;
        }
        /// <summary>
        /// 鑾峰彇鐘舵€?
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
        /// 璁＄畻甯ц寖鍥?
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
        /// 甯х巼,姣忕鎾斁鍔ㄧ敾鐨勫抚鐜?
        /// </summary>
        uint            _fps    =   30;
        /// <summary>
        /// 鏍规嵁 _clipTime 璁＄畻鍑烘潵鐨勫疄闄呭抚
        /// </summary>
        real            _clipFrame;
        /// <summary>
        /// 浠庝粈涔堟椂闂村紑濮嬫挱鏀?褰撹皟鐢ㄤ簡play鍑芥暟鍚庯紝寮€濮嬭绠?
        /// </summary>
        real            _startTime;
        /// <summary>
        /// 浠庡紑濮嬫挱鏀惧埌鍋滄缁忚繃杩囩殑鎵€鏈夋椂闂村拰
        /// </summary>
        real            _elapseTime;
        real            _weight;
        /// <summary>
        /// 鏃堕棿绾跨缉鏀撅紝蹇斁锛屾參鏀?
        /// </summary>
        real            _timeScale;
        /// <summary>
        /// 浠?寮€濮嬭绠?
        /// </summary>
        real            _clipTime;
        /// <summary>
        /// 甯ц寖鍥?
        /// </summary>
        uint2           _range;
        TrackResults    _result; 
    };

    using   Action      =   SharedPtr<FEAction>;
    using   Actions     =   std::vector<Action>;
}

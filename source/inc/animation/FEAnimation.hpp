#pragma     once
#include    <functional>

#include    "../FEComponent.hpp"
#include    "FEAnimClip.hpp"


namespace FE
{

    class   FEAction;

    using   Action  =   SharedPtr<FEAction>;

    DEFINE_CLASS_UUID(FEAnimation, "{D0749EE9-7126-4A57-B4F2-84798E4F40F2}");

    /// <summary>
    /// 对象中包含需要被动画的对象
    /// 以及clip信息，clip 动画片段，包含实际的动画关键帧组数据
    /// 可以实现动画数据与动画对象分离,让动画数据有复用价值
    /// 典型应用：一个对象，在时间线上，可以断开
    /// 一个节点对象可以有多个动画
    /// </summary>
    
    class   FEAnimation : public FEComponent
    {
    public:
        enum    ClipFlag
        {
            /// <summary>
            /// 标记数据变更
            /// </summary>
            AnimationChanged    =   (FLAG_ACTOR <<1),
        };
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimation)
    public:
        FEAnimation(FEContext& ctx)  
            :FEComponent(ctx,true)
        {}
        FEAnimation(const FEAnimation& other)
            :FEComponent(other)
        {
            _clip       =   other._clip;
            _results    =   other._results;
            _offset     =   other._offset;
        }
        ~FEAnimation()   =   default;
    public:
        /// <summary>
        /// 关联所有者
        /// </summary>
        /// <param name="owner"></param>
        virtual void    attach(Object owner) override
        {
            _owner  =   owner;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 取消关键
        /// </summary>
        virtual void    detach() override
        {
            _owner  =   nullptr;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 是否发生变更
        /// </summary>
        /// <returns></returns>
        inline  bool    isChanged() const
        {
            if (flags().hasFlag(AnimationChanged))
                return  true;
            else if(_clip)
                return  _clip->isChanged();
            else
                return  false;
        }
        /// <summary>
        /// 动画所属action,即被哪一个action控制
        /// 主要用作分组控制，一批动画可以播放，另一批停止
        /// </summary>
        /// <returns></returns>
        inline  Action  action() const
        {
            return  _action;
        }
        /// <summary>
        /// 设置action
        /// </summary>
        /// <param name="action"></param>
        inline  void    setAction(Action action)
        {
            _action =   action;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 获取起始时间,对于时间线
        /// </summary>
        /// <returns></returns>
        inline  real    offset() const
        {
            return  _offset;
        }
        /// <summary>
        /// 设置起始播放时间,相对于时间线
        /// </summary>
        /// <param name="offset"></param>
        inline  void    setOffset(const real& offset)
        {
            _offset =   offset;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 获取范围,时间播放的范围
        /// </summary>
        /// <returns></returns>
        inline  real2   range() const
        {
            if (_clip == nullptr)
                return  real2();
            else
                return  _clip->range() + real2(_offset,_offset);
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        inline  bool    isValid() const
        {
            return  _clip != nullptr &&  owner() != nullptr;
        }
        inline  auto    clip() const
        {
            return  _clip;
        }
        inline  void    setClip(AnimClip clip)
        {
            _clip   =   clip;
            flags().addFlag(AnimationChanged);
        }
        inline  void    clearChanged() 
        {
            flags().removeFlag(AnimationChanged);
        }
        /// <summary>
        /// 组件每一帧更新
        /// 返回值表示，是否已经修改了 _owner
        /// 如果修改了 返回true,没有修改返回false
        /// </summary>
        /// <param name="deltaTm"></param>
        /// <returns>true/false</returns>
        virtual bool    update(const real& tmDelta) override
        {
            if (!isValid())
                return  false;
            _clip->update(tmDelta,_results);
            _owner->beginSetProp();

            bool    bModify =   false;

            for (auto& var: _results)
            {
                if (!var._valid)
                    continue;
                bModify |=  _owner->setProperty(var._prop,var._value);
            }
            _owner->endSetProp(bModify);
            return  bModify;
        }
    public:
        /// <summary>
        /// 表示一个 Action 类型的成员变量，用于存储要执行的操作。
        /// </summary>
        Action          _action;
        /// <summary>
        /// AnimClip 类型的变量，用于保存动画剪辑。
        /// </summary>
        AnimClip        _clip;
        /// <summary>
        /// 动画在大时间线上的起始时间，标记了动画从什么时间开始播放
        /// </summary>
        real            _offset    =    0;
        TrackResults    _results; 
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}

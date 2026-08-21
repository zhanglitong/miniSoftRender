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
        }
        ~FEAnimation()   =   default;
    public:

        /// <summary>
        /// 动画所属action,即被哪一个action控制
        /// 主要用作分组控制，一批动画可以播放，另一批停止
        /// </summary>
        /// <returns></returns>
        Action      action() const
        {
            return  _action;
        }
        /// <summary>
        /// 设置action
        /// </summary>
        /// <param name="action"></param>
        void        setAction(Action action)
        {
            _action =   action;
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        bool        isValid() const
        {
            return  _clip != nullptr &&  owner() != nullptr;
        }
        AnimClip    clip() const
        {
            return  _clip;
        }
        void        setClip(AnimClip clip)
        {
            _clip   =   clip;
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
        Action          _action;
        AnimClip        _clip;
        TrackResults    _results; 
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}

#pragma     once  
#include    "../FEComponentSys.hpp"
#include    "FEPlayback.hpp"
namespace   FE 
{      
    DEFINE_CLASS_UUID(FEAnimationSys, "{B9034F58-6040-45E5-AA5D-6954A55971AD}");
    /// <summary>
    /// 管理多个Action
    /// </summary>
    class   FE_API  FEAnimationSys :public FEComponentSys     
    {    
    public:
        using   PlaybackMap =   std::map<String, Playback>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimationSys)
    public:       
        FEAnimationSys(FEContext& ctx)
            :FEComponentSys(ctx)   
        {
            /// 动画先于约束求解:约束系统读取动画采样后的世界变换
            priority(FEFactory::PT_Update).setOrder(MaxInt16 - 2);
        }         
        FEAnimationSys(const FEAnimationSys& other)            
            :FEComponentSys(other)         
        {}
    public:
        inline  auto&   playbacks()
        {
            return  _playbacks;
        }
        inline  auto&   playbacks() const
        {
            return  _playbacks;
        }
        /// <summary>
        /// 创建或者获取
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        inline  auto    getOrCreate(const String& name = "default")
        {
            auto        itr     =   _playbacks.find(name);
            if (itr != _playbacks.end())
                return  itr->second;
            Playback    playback    =   new FEPlayback(_ctx);
            _playbacks[name]        =   playback;
            return  playback;
        }
    public:
        /// <summary>
        /// 添加组件
        /// </summary>
        /// <param name="com"></param>
        /// <returns></returns>
        virtual size_t  addObject(Component com) override
        {
            auto    playback    =   getOrCreate();
            return  playback->addObject(com->as<FEAnimation>());
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            auto        playback    =   getOrCreate();
            Animations  anims(coms.size());
            for (size_t i = 0; i < coms.size(); ++i)
            {
                anims[i]    =   (FEAnimation*)(coms[i]->as<FEAnimation>());
            }
            return  playback->addObjects(anims);
        }
        /// <summary>
        /// 删除Animation
        /// </summary>
        /// <param name="com"></param>
        /// <returns></returns>
        virtual size_t  removeObject(Component com) override
        {
            if (com == nullptr)
                return  0;
            auto    playback    =   getOrCreate();
            return  playback->removeObject(com->as<FEAnimation>());
        }
        /// <summary>
        /// 删除多个Animation
        /// </summary>
        /// <param name="coms"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(const Components& coms) override
        {
            if (coms.empty())
                return  0;
            auto        playback    =   getOrCreate();
            Animations  anims(coms.size());
            for (size_t i = 0; i < coms.size(); ++i)
            {
                anims[i]    =   (FEAnimation*)(coms[i]->as<FEAnimation>());
            }
            return  playback->removeObjects(anims);
        }

        virtual void    clear()
        {
            _playbacks.clear();
        }
        /// <summary>
        /// 组件每一帧更新
        /// </summary>
        /// <param name="deltaTm"></param>
        /// <returns></returns>
        virtual void    update(const real& tmDelta) override
        {
            for (auto& var : _playbacks)
            {
                var.second->update(tmDelta);
            }
        }
        /// <summary>
        /// 设置时间(时间线时间)
        /// </summary>
        /// <param name="clipTime"></param>
        virtual void    setClipTime(const real& clipTime)
        {
            for (auto& var : _playbacks)
            {
                var.second->setClipTime(clipTime);
            }
        }
        /// <summary>
        /// 刷新所有动画的范围,在编辑模式下修改关键帧后调用
        /// </summary>
        virtual void    refreshRange()
        {
            for (auto& var : _playbacks)
            {
                var.second->refreshRange();
            }
        }
    protected:
        PlaybackMap     _playbacks;
    };

    using   AnimSys   =   SharedPtr<FEAnimationSys>;
}
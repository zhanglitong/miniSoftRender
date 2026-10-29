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
            if (com == nullptr)
                return  0;
            FEAnimation*    anim    =   (FEAnimation*)(com->as<FEAnimation>());
            /// 按组件记录的播放器名登记,便于撤销后重做回到原播放器
            return  getOrCreate(anim->playbackName())->addObject(anim);
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            size_t  count   =   0;
            for (size_t i = 0; i < coms.size(); ++i)
            {
                count   +=  addObject(coms[i]);
            }
            return  count;
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
            return  removeFromPlaybacks({com});
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
            return  removeFromPlaybacks(coms);
        }

        virtual void    clear()
        {
            /// 清除播放器前把被动画覆盖的节点本地变换恢复到原始值
            for (auto& var : _playbacks)
            {
                for (auto& anim : var.second->objects())
                    anim->restoreLocal();
            }
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
            /// 采样结果直接写入节点本地变换(与约束求解过程一致,节点不再反向依赖动画组件)
            /// 播放暂停时也执行,保证编辑器跳帧后节点处于最新采样姿态
            for (auto& var : _playbacks)
            {
                for (auto& anim : var.second->objects())
                    anim->applyTransform();
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
        /// <summary>
        /// 从所有播放器中摘除这些组件对应的动画:
        /// 组件可能被登记在任意命名播放器下,删除时只遍历已有播放器,不创建新播放器
        /// </summary>
        /// <param name="coms">要摘除的动画组件</param>
        /// <returns>返回实际移除的动画数</returns>
        size_t  removeFromPlaybacks(const Components& coms)
        {
            Animations  anims(coms.size());
            for (size_t i = 0; i < coms.size(); ++i)
            {
                anims[i]    =   (FEAnimation*)(coms[i]->as<FEAnimation>());
            }
            size_t  count   =   0;
            for (auto& var : _playbacks)
            {
                count   +=  var.second->removeObjects(anims);
            }
            return  count;
        }
        PlaybackMap     _playbacks;
    };

    using   AnimSys   =   SharedPtr<FEAnimationSys>;
}
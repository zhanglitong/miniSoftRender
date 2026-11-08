#pragma     once  
#include    <unordered_map>
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
            priority(FEFactory::PT_Update).setOrder(FEFactory::UO_Animation);
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
        /// 查找节点上登记的第一个动画组件(按播放器顺序,查找由播放器完成)
        /// </summary>
        Animation   animationOf(Object owner)
        {
            if (owner == nullptr)
                return  nullptr;
            for (auto& var : _playbacks)
            {
                if (var.second == nullptr)
                    continue;
                Animation   anim    =   var.second->animationOf(owner);
                if (anim != nullptr)
                    return  anim;
            }
            return  nullptr;
        }
        /// <summary>
        /// 编辑写值:只修改目标对象(节点)属性,不写关键帧;
        /// 打帧由时间线的“打帧”动作负责
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="prop">属性索引</param>
        /// <param name="value">属性值</param>
        /// <returns>true 表示有修改</returns>
        bool    setNodeProperty(Object node,int prop,const KFValue& value)
        {
            if (node == nullptr)
                return  false;
            Animation   anim    =   animationOf(node);
            if (anim != nullptr)
            {
                /// 播放中禁止编辑:播放驱动会在下一帧用采样值覆盖编辑结果,
                /// 而且编辑值对应的时间与播放时间不一致,写进去没有意义
                Playback    playback    =   playbackOf(anim);
                if (playback != nullptr && playback->status() == FEPlayback::PS_Running)
                    return  false;
                return  anim->setProperty(prop,value);
            }
            return  node->setProperty(prop,value);
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
            /// 播放器名同时记录在对象上,编辑器重命名时据此同步索引键
            playback->setName(name);
            _playbacks[name]        =   playback;
            return  playback;
        }
    public:
        /// <summary>
        /// 添加动画组件:playbackName 为播放器名,默认 "default";
        /// 已归属某个播放器的动画保持原归属(打开工程后由播放器恢复的分组不被冲掉),
        /// 显式指定其它名字时才从原播放器移动到目标播放器
        /// </summary>
        /// <param name="com">动画组件</param>
        /// <param name="playbackName">播放器名,空串表示 default</param>
        /// <returns>返回登记成功的动画数</returns>
        virtual size_t  addObject(Component com,const String& playbackName = "default")
        {
            if (com == nullptr)
                return  0;
            FEAnimation*    anim    =   (FEAnimation*)(com->as<FEAnimation>());
            if (anim == nullptr)
                return  0;
            const   String  targetName  =   playbackName.empty() ? String("default") : playbackName;
            auto            itr         =   _owners.find(anim);
            if (itr != _owners.end())
            {
                /// 默认名(场景分发路径):保持既有分组,不移动
                if (targetName == "default")
                    return  0;
                Playback    owner   =   itr->second;
                Playback    target  =   getOrCreate(targetName);
                if (owner == target)
                    return  0;
                if (owner != nullptr)
                    owner->removeObject(anim);
                itr->second =   target;
                return  target->addObject(anim);
            }
            Playback    target  =   getOrCreate(targetName);
            const   size_t  added   =   target->addObject(anim);
            _owners[anim]   =   target;
            return  added;
        }
        /// <summary>
        /// 登记一个已存在的播放器(从工程文件加载时使用):
        /// 同名播放器已存在时自动加后缀,避免覆盖
        /// </summary>
        /// <param name="playback">播放器对象</param>
        /// <returns>1 登记成功,0 已登记或对象无效</returns>
        size_t  addPlayback(Playback playback)
        {
            if (playback == nullptr)
                return  0;
            const   String  name    =   playback->name().empty() ? String("default") : playback->name();
            auto            itr     =   _playbacks.find(name);
            if (itr != _playbacks.end() && itr->second == playback)
                return  0;
            String  unique  =   name;
            int     index   =   1;
            while (_playbacks.find(unique) != _playbacks.end())
            {
                unique  =   name + std::to_string(index);
                ++index;
            }
            playback->setName(unique);
            _playbacks[unique]  =   playback;
            /// 建立动画到播放器的反查索引,避免逐个线性扫描
            for (const Animation& anim : playback->objects())
            {
                if (anim != nullptr)
                    _owners[anim.get()] =   playback;
            }
            return  1;
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            /// 按 clip 名分组后批量登记:逐个 addObject 在大规模实例下是 O(n^2),
            /// 批量路径由播放器内部"追加 + 排序 + 去重"完成
            std::map<String,Animations> groups;
            for (size_t i = 0; i < coms.size(); ++i)
            {
                Component   com     =   coms[i];
                FEAnimation* anim   =   (com == nullptr) ? nullptr : (FEAnimation*)(com->as<FEAnimation>());
                if (anim == nullptr)
                    continue;
                /// 已归属某个播放器(例如打开工程后恢复的分组)保持原归属
                if (_owners.find(anim) != _owners.end())
                    continue;
                const   String& clipName    =   anim->name();
                groups[clipName.empty() ? String("default") : clipName].push_back(anim);
            }
            size_t  count   =   0;
            for (auto& var : groups)
            {
                Playback    playback    =   getOrCreate(var.first);
                count   +=  playback->addObjects(var.second);
                for (const Animation& anim : var.second)
                    _owners[anim.get()] =   playback;
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
            _playbacks.clear();
            _owners.clear();
        }
        /// <summary>
        /// 组件每一帧更新:只有播放中的播放器才推进并采样,
        /// 采样结果由动画自身在 update 内实时写入节点本地变换;
        /// 暂停时不做任何处理,节点保持当前姿态
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
        /// <summary>
        /// 查找动画所属的播放器:动画是被播放器管理的对象,最多归属一个播放器
        /// </summary>
        /// <param name="anim">动画组件</param>
        /// <returns>所属播放器,未登记时返回 nullptr</returns>
        Playback    playbackOf(const Animation& anim) const
        {
            if (anim == nullptr)
                return  nullptr;
            auto    itr =   _owners.find(anim.get());
            return  (itr == _owners.end()) ? nullptr : itr->second;
        }
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
            for (const Animation& anim : anims)
            {
                if (anim != nullptr)
                    _owners.erase(anim.get());
            }
            return  count;
        }
        PlaybackMap     _playbacks;
        /// <summary>
        /// 动画 -> 所属播放器 的反查索引:大规模实例下避免逐播放器线性扫描
        /// </summary>
        std::unordered_map<const FEAnimation*,Playback>    _owners;
    };

    using   AnimSys   =   SharedPtr<FEAnimationSys>;
}
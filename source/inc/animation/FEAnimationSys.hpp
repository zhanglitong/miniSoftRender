#pragma     once  
#include    "../FEComponentSys.hpp"
#include    "FEAction.hpp"
namespace   FE 
{      
    DEFINE_CLASS_UUID(FEAnimationSys, "{B9034F58-6040-45E5-AA5D-6954A55971AD}");
    /// <summary>
    /// 管理多个Action
    /// </summary>
    class   FEAnimationSys :public FEComponentSys     
    {    
    public:
        using   ActionMap   =   std::map<String, Action>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimationSys)
    public:
    public:         
        FEAnimationSys(FEContext& ctx)
            :FEComponentSys(ctx)   
        {}         
        FEAnimationSys(const FEAnimationSys& other)            
            :FEComponentSys(other)         
        {}
    public:
        inline  auto&   actions()
        {
            return  _actions;
        }
        inline  auto&   actions() const
        {
            return  _actions;
        }
        /// <summary>
        /// 创建或者获取
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        inline  auto    getOrCreate(const String& name = "default")
        {
            auto    itr     =   _actions.find(name);
            if (itr != _actions.end())
                return  itr->second;
            Action  action  =   new FEAction(_ctx);
            _actions[name]  =   action;
            return  action;
        }
    public:
        /// <summary>
        /// 添加组件
        /// </summary>
        /// <param name="com"></param>
        /// <returns></returns>
        virtual size_t  addObject(Component com) override
        {
            auto    action  =   getOrCreate();
            return  action->addObject(com->as<FEAnimation>());
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            auto        action  =   getOrCreate();
            Animations  anims(coms.size());
            for (size_t i = 0; i < coms.size(); ++i)
            {
                anims[i]    =   (FEAnimation*)(coms[i]->as<FEAnimation>());
            }
            return  action->addObjects(anims);
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
            auto    action  =   getOrCreate();
            return  action->removeObject(com->as<FEAnimation>());
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
            auto        action  =   getOrCreate();
            Animations  anims(coms.size());
            for (size_t i = 0; i < coms.size(); ++i)
            {
                anims[i]    =   (FEAnimation*)(coms[i]->as<FEAnimation>());
            }
            return  action->removeObjects(anims);
        }
        /// <summary>
        /// 组件每一帧更新
        /// </summary>
        /// <param name="deltaTm"></param>
        /// <returns></returns>
        virtual void    update(const real& tmDelta) override
        {
            for (auto& var : _actions)
            {
                var.second->update(tmDelta);
            }
        }
    protected:
        ActionMap   _actions;
    };

    using   AnimSys   =   SharedPtr<FEAnimationSys>;
}
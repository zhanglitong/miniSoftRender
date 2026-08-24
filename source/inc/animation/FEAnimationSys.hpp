#pragma     once  
#include    "../FEComponentSys.hpp"
#include    "FEAction.hpp"
namespace   FE 
{      
    DEFINE_CLASS_UUID(FEAnimationSys, "{B9034F58-6040-45E5-AA5D-6954A55971AD}");

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
        auto&   actions()
        {
            return  _actions;
        }
        auto&   actions() const
        {
            return  _actions;
        }
        /// <summary>
        /// 创建或者获取
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        auto    getOrCreate(const String& name = "default")
        {
            auto    itr     =   _actions.find(name);
            if (itr != _actions.end())
                return  itr->second;
            Action  action  =   new FEAction(_ctx);
            _actions[name]  =   action;
            return  action;
        }
    public:
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

        virtual size_t  removeObject(Component com) override
        {
            UNUSED(com);
            return  0;
        }
        virtual size_t  removeObjects(const Components& coms) override
        {
            UNUSED(coms);
            return  0;
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
        Components  _objects;
        ActionMap   _actions;
    };

    using   AnimSys   =   SharedPtr<FEAnimationSys>;
}
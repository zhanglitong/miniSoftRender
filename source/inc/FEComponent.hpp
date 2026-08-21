#pragma     once  
#include    "FEObject.h"  
#include    "FEPriority.hpp"
namespace   FE 
{     
    class   FEComponent :public FEObject     
    {     
    public:         
        FEComponent(FEContext& ctx,bool bActor = false)             
            :FEObject(ctx)    
            ,_type(bActor)
        {}         
        FEComponent(const FEComponent& other)            
            :FEObject(other)         
        {
            _owner      =   other._owner;
            _priority   =   other._priority;
            _type       =   other._type;
        } 
        inline  bool    isActor() const
        {
            return  _type;
        }
        /// <summary>
        /// 获取优先级
        /// </summary>
        /// <returns></returns>
        const  auto&    priority() const
        {
            return  _priority;
        }
        inline  auto&   priority()
        {
            return  _priority;
        }
        /// <summary>
        /// 关联所有者
        /// </summary>
        /// <param name="owner"></param>
        inline  void    attach(Object owner)
        {
            _owner  =   owner;
        }
        /// <summary>
        /// 取消关键
        /// </summary>
        inline  void    detach()
        {
            _owner  =   nullptr;
        }
        /// <summary>
        /// 获取所有者
        /// </summary>
        /// <returns></returns>
        inline  Object  owner() const
        {
            return  _owner;
        }
        /// <summary>
        /// 是否可用，首先检测是否可用，在调用更新函数@ref update();
        /// </summary>
        /// <returns></returns>
        inline  bool    isEnable() const
        {
            return  flags().hasFlag(FLAG_ENABLE);
        }
        /// <summary>
        /// 设置是否可用
        /// </summary>
        /// <param name="bFlag"></param>
        inline  void    setEnable(bool bFlag)
        {
            if (bFlag)
                flags().addFlag(FLAG_ENABLE);
            else
                flags().removeFlag(FLAG_ENABLE);
        }
        /// <summary>
        /// 组件每一帧更新
        /// 返回值表示，是否已经修改了 _owner
        /// 如果修改了 返回true,没有修改返回false
        /// </summary>
        /// <param name="deltaTm"></param>
        /// <returns></returns>
        virtual bool    update(const real& deltaTm)
        {
            UNUSED(deltaTm);
            return  false;
        }
        
    protected:
        /// <summary>
        /// 组件的拥有者
        /// </summary>
        Object      _owner;
        /// <summary>
        /// 优先级
        /// </summary>
        FEPriority  _priority;
        /// <summary>
        /// true: Actor
        /// false:Reactor
        /// </summary>
        bool        _type   =   false;
    };

    using   Component   =   SharedPtr<FEComponent>;
    using   Components  =   std::vector<Component>;
}
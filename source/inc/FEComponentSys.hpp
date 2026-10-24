#pragma     once  
#include    "FEComponent.hpp"
#include    "FEFactory.hpp"
#include    "FEObjectsTemplate.hpp"
#include    "node/FENode.hpp"
namespace   FE 
{     

    /// <summary>
    /// 是组件系统的基类
    /// 典型应用: 
    ///     1.动画系统，动画系统继承FEComponentSys
    ///     2.物理系统，动画系统继承FEComponentSys
    /// </summary>
    class   FE_API  FEComponentSys :public FEFactory     
    {     
    public:         
        FEComponentSys(FEContext& ctx)
            :FEFactory(ctx)   
            ,_updateList(ObjectLessFunc)
        {
            priority(FEFactory::PT_Update).setPriority(EPriority::EP_Last);
            priority(FEFactory::PT_Update).setOrder(MaxInt16);
        }         
        FEComponentSys(const FEComponentSys& other)            
            :FEFactory(other)         
            ,_updateList(ObjectLessFunc)
        {
        } 

    public:
        /// <summary>
        /// 更新列表:组件系统在 update 中把受影响的节点加入该列表,
        /// 由场景在系统更新完成后显式调用 updateObjectList 统一刷新
        /// </summary>
        inline  auto&   updateList()
        {
            return  _updateList;
        }
        const   auto&   updateList() const
        {
            return  _updateList;
        }
        virtual size_t  addObject(Component  com)
        {
            UNUSED(com);
            return  0;
        }
        virtual size_t  addObjects(const Components&  coms)
        {
            UNUSED(coms);
            return  0;
        }
        virtual size_t  removeObject(Component com)
        {
            UNUSED(com);
            return  0;
        }
        virtual size_t  removeObjects(const Components& coms)
        {
            UNUSED(coms);
            return  0;
        }
        /// <summary>
        /// 组件每一帧更新
        /// </summary>
        /// <param name="deltaTm"></param>
        virtual void    update(const real& deltaTm)
        {
            UNUSED(deltaTm);
        }
        /// <summary>
        /// 显式刷新更新列表中的节点:依次执行 update + fireChanged,然后清空列表
        /// 由场景在组件系统更新完成后调用,顺序在 update 之后
        /// </summary>
        virtual void    updateObjectList()
        {
            for (auto var : _updateList.objects())
            {
                auto    node    =   var->cast<FENode>();
                if (node)
                {
                    node->update();
                    node->fireChanged();
                }
            }
            _updateList.clearObjects();
        }
        virtual void    clear()
        {}
    protected:
        /// <summary>
        /// 组件系统中计算出来需要更新的对象
        /// </summary>
        ObjectList          _updateList;
    public:
        template<typename TObject>
        static  uint    countObject(const Node&  node)
        {
            uint    result  =   0;
            auto&   coms    =   node->components(); 
            for (auto& com : coms)
            {
                if (com->cast<TObject>() != nullptr)
                    ++result;
            }
            auto&   childs  =   node->children();
            for (auto object : childs)
            {
                Node    child   =   object->as<FENode>();
                if (node == nullptr)
                    continue;
                else
                    result  +=  countObject<TObject>(child);
            }
            return  result;
        }

        template<typename TObject>
        static  uint    countObjects(const Nodes&  nodes)
        {
            uint    result  =   0;
            for (auto& node : nodes)
            {
                result  +=  countObject<TObject>(node);
            }
            return  result;
        }

        template<typename TObject>
        static  void    collectObject(const Node&  node,Components& result)
        {
            auto&   coms    =   node->components(); 
            for (auto& com : coms)
            {
                auto    obj     =   com->cast<TObject>();
                if (obj != nullptr)
                    result.push_back(com);
            }
            auto&   childs      =   node->children();
            for (auto object : childs)
            {
                Node    child   =   object->as<FENode>();
                if (node == nullptr)
                    continue;
                else
                    collectObject<TObject>(child,result);
            }
        }

        template<typename TObject>
        static  void    collectObjects(const Nodes&  nodes,Components& result)
        {
            for (auto& node : nodes)
            {
                collectObject<TObject>(node,result);
            }
        }
    };

    using   ComponentSys    =   SharedPtr<FEComponentSys>;
    using   ComponentSyss   =   std::vector<Component>;
}
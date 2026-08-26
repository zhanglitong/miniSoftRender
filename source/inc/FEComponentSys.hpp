#pragma     once  
#include    "FEComponent.hpp"
#include    "FEFactory.hpp"
namespace   FE 
{     
    /// <summary>
    /// 是组件系统的基类
    /// 典型应用: 
    ///     1.动画系统，动画系统继承FEComponentSys
    ///     2.物理系统，动画系统继承FEComponentSys
    /// </summary>
    class   FEComponentSys :public FEFactory     
    {     
    public:         
        FEComponentSys(FEContext& ctx)
            :FEFactory(ctx)   
        {}         
        FEComponentSys(const FEComponentSys& other)            
            :FEFactory(other)         
        {} 

    public:
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
        /// <returns></returns>
        virtual void    update(const real& deltaTm)
        {
            UNUSED(deltaTm);
        }
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
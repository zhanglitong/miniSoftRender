#pragma     once

#include    "FENode.hpp"
  

namespace   FE
{
    template<typename TNotify,typename TContainer = std::map<void*,TNotify>>
    class   TNotifyWrapper
    {   
    public:
        TNotifyWrapper()
        {}
        TNotifyWrapper(const TNotifyWrapper& other)
            :_notifys(other._notify)
        {}
        /// <summary>
        /// 调用通知函数
        /// </summary>
        /// <typeparam name="...Args"></typeparam>
        /// <param name="...args"></param>
        template<class ... Args>
        inline  void    fireNotify(Args&&... args)
        {
            for (auto& var : _notifys)
            {
                var.second(std::forward<Args>(args)...);
            }
        }
        inline  void    addNotify(void* pKey,const TNotify& notify)
        {
            assert(notify);
            if (notify) _notifys[pKey] =   notify;
        }
        inline  void    removeNotify(void* pKey)
        {
            _notifys.erase(pKey);
        }
        inline  void    clearNotify()
        {
            _notifys.clear();
        }
        inline  TNotify query(void* pKey) const
        {
            auto    itr =   _notifys.find(pKey);
            if (itr != _notifys.end())
                return  itr->second;
            else
                return  {};
        }
        /// <summary>
        /// 获取所有的通知函数
        /// </summary>
        /// <returns></returns>
        const  auto&    notifies() const
        {
            return  _notifys;
        }
    protected:
        TContainer  _notifys;
    };

    DEFINE_CLASS_UUID(FENodeTree,"{D623E1BB-9386-45F0-9AD0-4274585D1F40}");

    using   NAddNode        =   std::function<void(Node)>;
    using   NRemoveNode     =   std::function<void(Node)>;
    using   NotifyClear     =   std::function<void(const Nodes&)>;

    using   NAddNodes       =   TNotifyWrapper<NAddNode,        std::map<void*,NRemoveNode>>;
    using   NRemoveNodes    =   TNotifyWrapper<NRemoveNode,     std::map<void*,NRemoveNode>>;
    using   NotifyClears    =   TNotifyWrapper<NotifyClear,     std::map<void*,NRemoveNode>>;

    class   FENodeTree :public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FENodeTree)
    public:
        FENodeTree(FEContext& ctx)
            :FEObject(ctx)
        {}
        FENodeTree(const FENodeTree& other)
            :FEObject(other)
        {
            _topLevelNodes  =   other._topLevelNodes;
        }
        inline  auto&   addNodeEvents()
        {
            return  _NAddNodes;
        }
        const   auto&   addNodeEvents() const
        {
            return  _NAddNodes;
        }
        inline  auto&   removeNodeEvents()
        {
            return  _NRemoveNodes;  
        }
        const   auto&   removeNodeEvents() const
        {
            return  _NRemoveNodes;  
        }
        inline  auto&   clearEvents()
        {
            return  _NotifyClears;
        }
        const   auto&   clearEvents() const
        {
            return  _NotifyClears;
        }
    public:
        const   Nodes&  topLevelNodes() const
        {
            return  _topLevelNodes;
        }
        inline  void    addToplevelNode(const Node& node)
        {
            _topLevelNodes.push_back(node);
            _NAddNodes.fireNotify(node);
        }
        inline  void    addToplevelNodes(const Nodes& nodes)
        {
            _topLevelNodes.insert(_topLevelNodes.end(), nodes.begin(), nodes.end());
            for (auto& var : nodes)
            {
                _NAddNodes.fireNotify(var);
            }
        }
        inline  void    removeToplevelNode(const Node& node)
        {
            auto    itr =   std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node);
            if (itr != _topLevelNodes.end())
            {
                _topLevelNodes.erase(itr);
                _NRemoveNodes.fireNotify(node);
            }
                
        }
        inline  void    removeToplevelNodes(const Nodes& nodes)
        {
            for (const auto& node : nodes)
            {
                removeToplevelNode(node);
            }
        }
        inline  void    removeNode(const Node& node)
        {
            if (node == nullptr)
                return;
            auto    parent = node->parent();
            if(parent && parent->removeChild(node))
                _NRemoveNodes.fireNotify(node);
            else
                removeToplevelNode(node);
        }
        inline  void    removeNodes(const Nodes& nodes)
        {
            for (const auto& node : nodes)
            {
                removeNode(node);
            }
        }
        inline  void    clear()
        {
            _topLevelNodes.clear();
        }
    protected:
        Nodes           _topLevelNodes;

        NAddNodes       _NAddNodes   ;
        NRemoveNodes    _NRemoveNodes;
        NotifyClears    _NotifyClears;
    };

    using   NodeTree    =   SharedPtr<FENodeTree>;
}


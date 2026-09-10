#pragma     once

#include    "../FEDelegate.hpp"
#include    "FENode.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FENodeTree,"{D623E1BB-9386-45F0-9AD0-4274585D1F40}");

    using   NAddNodes       =   FETMultiDelegate<void(const FENode*)>;
    using   NRemoveNodes    =   FETMultiDelegate<void(const FENode*)>;
    using   NChangedNodes   =   FETMultiDelegate<void(const FENode*)>;
    using   NotifyClears    =   FETMultiDelegate<void()>;

    /// <summary>
    /// 节点树管理
    /// 没有选择通知(使用NChangedNodes);
    /// 当节点修改了状态，会触发NChangedNodes
    /// </summary>
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
        inline  auto&   eventsAddNode()
        {
            return  _NAddNodes;
        }
        const   auto&   eventsAddNode() const
        {
            return  _NAddNodes;
        }
        inline  auto&   eventsRemoveNode()
        {
            return  _NRemoveNodes;  
        }
        const   auto&   eventsRemoveNode() const
        {
            return  _NRemoveNodes;  
        }
        /// <summary>
        /// 获取节点属性变更通知对象(读写)
        /// </summary>
        /// <returns>节点属性变更通知集合</returns>
        inline  auto&   eventsChangedNode()
        {
            return  _NChangedNodes;  
        }
        /// <summary>
        /// 获取节点属性变更通知对象(只读)
        /// </summary>
        /// <returns>节点属性变更通知集合</returns>
        const   auto&   eventsChangedNode() const
        {
            return  _NChangedNodes;  
        }
        
        inline  auto&   eventsClear()
        {
            return  _NotifyClears;
        }
        const   auto&   eventsClear() const
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
            _NAddNodes(node.get());
        }
        inline  void    addToplevelNodes(const Nodes& nodes)
        {
            _topLevelNodes.insert(_topLevelNodes.end(), nodes.begin(), nodes.end());
            for (auto& var : nodes)
            {
                _NAddNodes(var.get());
            }
        }
        inline  void    removeToplevelNode(const Node& node)
        {
            auto    itr =   std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node);
            if (itr != _topLevelNodes.end())
            {
                _topLevelNodes.erase(itr);
                _NRemoveNodes(node.get());
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
                _NRemoveNodes(node.get());
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
        /// <summary>
        /// 清空场景内容
        /// </summary>
        inline  void    clear()
        {
            _NotifyClears();
            for (auto var : _topLevelNodes)
            {
                var->clear();
            }
            _topLevelNodes.clear();
        }
    protected:
        Nodes           _topLevelNodes;
        NAddNodes       _NAddNodes   ;
        NRemoveNodes    _NRemoveNodes;
        NChangedNodes   _NChangedNodes;
        NotifyClears    _NotifyClears;
    };

    using   NodeTree    =   SharedPtr<FENodeTree>;
}


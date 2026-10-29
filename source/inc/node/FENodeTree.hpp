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
    /// 节点树管理:持有场景的顶层节点列表,并向订阅者广播节点增删、属性变更与清空事件。
    /// 只有顶层节点登记在本对象中,子节点的增删由 FENode 自身转发到同一组事件;
    /// 没有单独的选择通知(选中状态由外部维护),节点状态改变时触发 eventsChangedNode
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
        /// <summary>
        /// 获取节点添加通知对象(读写):顶层节点加入或子节点挂到父节点时触发
        /// </summary>
        /// <returns>节点添加通知集合</returns>
        inline  auto&   eventsAddNode()
        {
            return  _NAddNodes;
        }
        /// <summary>
        /// 获取节点添加通知对象(只读)
        /// </summary>
        /// <returns>节点添加通知集合</returns>
        const   auto&   eventsAddNode() const
        {
            return  _NAddNodes;
        }
        /// <summary>
        /// 获取节点移除通知对象(读写):顶层节点移除或子节点从父节点摘除时触发
        /// </summary>
        /// <returns>节点移除通知集合</returns>
        inline  auto&   eventsRemoveNode()
        {
            return  _NRemoveNodes;  
        }
        /// <summary>
        /// 获取节点移除通知对象(只读)
        /// </summary>
        /// <returns>节点移除通知集合</returns>
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
        
        /// <summary>
        /// 获取节点树清空通知对象(读写):clear() 开头触发
        /// </summary>
        /// <returns>清空通知集合</returns>
        inline  auto&   eventsClear()
        {
            return  _NotifyClears;
        }
        /// <summary>
        /// 获取节点树清空通知对象(只读)
        /// </summary>
        /// <returns>清空通知集合</returns>
        const   auto&   eventsClear() const
        {
            return  _NotifyClears;
        }
        
    public:
        /// <summary>
        /// 获取所有顶层节点(只读)
        /// </summary>
        /// <returns>顶层节点列表</returns>
        const   Nodes&  topLevelNodes() const
        {
            return  _topLevelNodes;
        }
        /// <summary>
        /// 追加一个顶层节点,并触发 eventsAddNode;空指针或已在列表中则直接忽略
        /// </summary>
        /// <param name="node">要添加的节点</param>
        inline  void    addToplevelNode(const Node& node)
        {
            if (node == nullptr)
                return;
            if (std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node) != _topLevelNodes.end())
                return;
            _topLevelNodes.push_back(node);
            _NAddNodes(node.get());
        }
        /// <summary>
        /// 批量追加顶层节点:逐个走 addToplevelNode,每成功加入一个触发一次 eventsAddNode,
        /// 空指针与重复项被跳过
        /// </summary>
        /// <param name="nodes">要添加的节点列表</param>
        inline  void    addToplevelNodes(const Nodes& nodes)
        {
            for (const auto& node : nodes)
            {
                addToplevelNode(node);
            }
        }
        /// <summary>
        /// 从顶层列表中移除节点(不在列表中则跳过),移除成功触发 eventsRemoveNode
        /// </summary>
        /// <param name="node">要移除的节点</param>
        inline  void    removeToplevelNode(const Node& node)
        {
            auto    itr =   std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node);
            if (itr != _topLevelNodes.end())
            {
                _topLevelNodes.erase(itr);
                _NRemoveNodes(node.get());
            }
                
        }
        /// <summary>
        /// 批量从顶层列表中移除节点
        /// </summary>
        /// <param name="nodes">要移除的节点列表</param>
        inline  void    removeToplevelNodes(const Nodes& nodes)
        {
            for (const auto& node : nodes)
            {
                removeToplevelNode(node);
            }
        }
        /// <summary>
        /// 调整顶层节点在列表中的顺序(仅移动列表位置,父子关系不变)
        /// </summary>
        /// <param name="node">要移动的顶层节点</param>
        /// <param name="index">目标位置(按移除该节点后的列表计算;SIZE_MAX=末尾)</param>
        /// <returns>节点为空或不在顶层列表中时返回 false</returns>
        inline  bool    moveToplevelNode(const Node& node, size_t index)
        {
            if (node == nullptr)
                return  false;
            auto    itr =   std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node);
            if (itr == _topLevelNodes.end())
                return  false;
            Node    item    =   *itr;
            _topLevelNodes.erase(itr);
            if (index == SIZE_MAX || index >= _topLevelNodes.size())
                _topLevelNodes.push_back(item);
            else
                _topLevelNodes.insert(_topLevelNodes.begin() + index, item);
            return  true;
        }
        /// <summary>
        /// 移除节点:有父节点时交给父节点 removeChild 摘除(其内部 onRemoveChild 已广播
        /// eventsRemoveNode,这里不再重复通知),否则从顶层列表移除
        /// </summary>
        /// <param name="node">要移除的节点,空指针直接返回</param>
        inline  void    removeNode(const Node& node)
        {
            if (node == nullptr)
                return;
            auto    parent  =   node->parent();
            if (parent != nullptr && parent->removeChild(node))
                return;
            removeToplevelNode(node);
        }
        /// <summary>
        /// 改变节点的父节点,并同步维护顶层集合(唯一入口)。
        /// FENode::reparent 只改父子关系、不碰 _topLevelNodes,因此编辑器拖拽、MCP、
        /// 脚本、撤销命令等所有改层级的地方都必须走本接口
        /// </summary>
        /// <param name="node">要移动的节点</param>
        /// <param name="parent">新父节点,nullptr 表示提升为顶层</param>
        /// <param name="index">在新父节点子列表中的插入位置(SIZE_MAX=追加末尾);同父时表示重排</param>
        /// <returns>节点为空、挂到自身或自身子孙下时返回 false</returns>
        inline  bool    setParent(Node node, FENode* parent, size_t index = SIZE_MAX)
        {
            if (node == nullptr)
                return  false;
            /// 防环:新父节点不能是节点自身,也不能是它的子孙
            for (FENode* p = parent; p != nullptr; p = p->parent().get())
            {
                if (p == node.get())
                    return  false;
            }
            /// 有父节点时不应留在顶层集合里;不在集合中时本调用是空操作,不会重复发通知
            if (parent != nullptr)
                removeToplevelNode(node);
            node->reparent(parent,index);
            /// 提升为顶层:reparent 不会登记顶层集合,这里补上(已在集合中时自动跳过)
            if (parent == nullptr)
                addToplevelNode(node);
            return  true;
        }
        /// <summary>
        /// 批量移除节点,逐个走 removeNode
        /// </summary>
        /// <param name="nodes">要移除的节点列表</param>
        inline  void    removeNodes(const Nodes& nodes)
        {
            for (const auto& node : nodes)
            {
                removeNode(node);
            }
        }
        /// <summary>
        /// 清空节点树:先触发 eventsClear,再清空各顶层节点的组件与子节点(子节点摘除会
        /// 触发 eventsRemoveNode),最后清空顶层列表。
        /// 遍历用快照:节点清空时广播的移除事件里订阅者可能回头改本列表
        /// </summary>
        inline  void    clear()
        {
            _NotifyClears();
            Nodes   nodes   =   _topLevelNodes;
            for (auto& node : nodes)
            {
                if (node != nullptr)
                    node->clear();
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


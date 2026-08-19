#pragma     once

#include    "FENode.hpp"
  

namespace   FE
{
    DEFINE_CLASS_UUID(FENodeTree,"{D623E1BB-9386-45F0-9AD0-4274585D1F40}");

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
        const   Nodes&  topLevelNodes() const
        {
            return  _topLevelNodes;
        }
        inline  void    addToplevelNode(const Node& node)
        {
            _topLevelNodes.push_back(node);
        }
        inline  void    addToplevelNodes(const Nodes& nodes)
        {
            _topLevelNodes.insert(_topLevelNodes.end(), nodes.begin(), nodes.end());
        }
        inline  void    removeToplevelNode(const Node& node)
        {
            auto    itr =   std::find(_topLevelNodes.begin(), _topLevelNodes.end(), node);
            if (itr != _topLevelNodes.end())
                _topLevelNodes.erase(itr);
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
            auto    parent = node->parent();
            parent->removeChild(node);
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
        Nodes   _topLevelNodes;
    };

    using   NodeTree    =   SharedPtr<FENodeTree>;
}


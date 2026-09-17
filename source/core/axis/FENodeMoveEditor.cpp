
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeMoveEditor.h"

namespace   FE
{
   
    FENodeMoveEditor::FENodeMoveEditor(FEContext& ctx)
        :FEEditAxisMove(ctx)
    { 
        /// 默认优先级最高,最先执行
        _priority.setPriority(EP_Fist);
        _priority.setOrder(0);

        /// 增加节点通知
        ctx.scene()->nodeTree().eventsAddNode() += {this,[this](const FENode* )
        {
            sync();
        }};
        /// 移除节点通知
        ctx.scene()->nodeTree().eventsRemoveNode() += {this,[this](const FENode* obj)
        {
            auto    itr =   std::remove_if(_nodes.begin(),_nodes.end(),[obj](const Node& e) { return e.get() == obj; });
            _nodes.erase(itr,_nodes.end());
            sync();
        }};
        /// 清除节点通知
        ctx.scene()->nodeTree().eventsClear() += {this,[this]()
        {
            _nodes  =   {};
            sync();
        }};
        /// 节点属性更改通知
        ctx.scene()->nodeTree().eventsChangedNode() += {this,[this](const FENode*)
        {
            sync();
        }};
    }
    FENodeMoveEditor::FENodeMoveEditor(const FENodeMoveEditor& other)
        :FEEditAxisMove(other)
    {
        _priority = other.priority();
    }
    FENodeMoveEditor::~FENodeMoveEditor()
    {
        /// 移除节点通知
        _ctx.scene()->nodeTree().eventsAddNode()        -= {this};
        /// 移除节点通知
        _ctx.scene()->nodeTree().eventsRemoveNode()     -= {this};
        /// 移除节点通知
        _ctx.scene()->nodeTree().eventsClear()          -= {this};
        /// 移除节点属性更改通知
        _ctx.scene()->nodeTree().eventsChangedNode()    -= {this};
    }

    void    FENodeMoveEditor::setNodes(const Nodes& nodes)
    {
        _nodes   =   nodes;
        if (_nodes.empty())
            mDelegate()     -=  this;
        else
            mDelegate()     +=  {this,&FENodeMoveEditor::onMAxis};
        sync();
    }
    void    FENodeMoveEditor::sync()
    {
        aabb3dr box;
        for (auto& var: _nodes)
        {
            box.merge(var->globalAabb());
        }
        setTranslation(box.center());
    }
    void    FENodeMoveEditor::onMAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisMove& sender)
    {
        for (auto node : _nodes)
        {
            auto    trans   =   node->localTranslation() + relativeOffset;
            node->setLocalTranslation(trans); 
            node->update();
            node->fireChanged();
        }
    }
}

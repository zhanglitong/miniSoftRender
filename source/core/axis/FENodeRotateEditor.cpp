
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeRotateEditor.h"

namespace   FE
{
    FENodeRotateEditor::FENodeRotateEditor(FEContext& ctx)
        :FEEditAxisRotate(ctx)
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
    FENodeRotateEditor::FENodeRotateEditor(const FENodeRotateEditor& other)
        :FEEditAxisRotate(other)
    {
        _priority = other.priority();
    }

    void    FENodeRotateEditor::setNodes(const Nodes& nodes)
    {
        _nodes   =   nodes;
        if (_nodes.empty())
            mDelegate()     -=  this;
        else
            mDelegate()     +=  {this,&FENodeRotateEditor::onRAxis};
        sync();
    }
    void    FENodeRotateEditor::sync()
    {
        aabb3dr box;
        for (auto& var: _nodes)
        {
            box.merge(var->globalAabb());
        }
        setTranslation(box.center());
    }
    void    FENodeRotateEditor::onRAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& axis
                                        , real relativeOffsetAngle
                                        , real absoluteOffsetAngle
                                        , FEEditAxisRotate& sender)
    {
        (void)status;
        (void)absoluteOffsetAngle;
        real3       pivot   =   position();
        quatr       delta   =   FE::angleAxis(relativeOffsetAngle, normalize(axis));
        for (auto node : _nodes)
        {
            ///围绕编辑器位置旋转节点
            real3   worldPos    =   node->globalTranslation();
            real3   offset      =   worldPos - pivot;
            real3   newWorldPos =   pivot + delta * offset;
            node->setGlobalTranslation(newWorldPos);
            ///旋转节点朝向
            quatr   worldRot    =   node->globalRotation();
            node->setGlobalRotation(normalize(delta * worldRot));
            node->update();
            node->fireChanged();
        }
    }
}

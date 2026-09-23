
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
            auto    itr =   std::remove_if(_objects.begin(),_objects.end(),[obj](const Object& e) { return e.get() == obj; });
            _objects.erase(itr,_objects.end());
            sync();
        }};
        /// 清除节点通知
        ctx.scene()->nodeTree().eventsClear() += {this,[this]()
        {
            _objects    =   {};
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

    void    FENodeMoveEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        if (_objects.empty())
            mDelegate()     -=  this;
        else
            mDelegate()     +=  {this,&FENodeMoveEditor::onMAxis};
        sync();
    }
    void    FENodeMoveEditor::sync()
    {
        aabb3dr box;
        for (auto& var: _objects)
        {
            auto    node    =   var->cast<FENode>();
            if (node)
            {
                box.merge(node->globalAabb());
                continue;
            }
            auto    com     =   var->cast<FEComponent>();
            if (com && com->owner())
            {
                node    =   com->owner()->cast<FENode>(); 
                if (node)
                {
                    box.merge(node->globalAabb());
                }
            }
        }
        setTranslation(box.center());
    }
    void    FENodeMoveEditor::onMAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisMove& sender)
    {
        for (auto object : _objects)
        {
            auto    node    =   object->cast<FENode>();
            if (node)
            {
                auto    trans   =   node->localTranslation() + relativeOffset;
                node->setLocalTranslation(trans); 
                node->update();
                node->fireChanged();
                continue;
            }
            /// 如果有transform接口，通过接口设置数据
            auto    transform   =   (FETransform*)object->queryInterface("FETransform");
            if (transform)
            {
                auto    trans   =   transform->position() + relativeOffset;
                transform->setPosition(trans);
            }
            /// 如果是组件,获取owner
            /// 触发更新
            auto    com     =   object->cast<FEComponent>();
            if (com && com->owner())
            {
                node    =   com->owner()->cast<FENode>();
                if (node)
                {
                    node->flags().addFlag(FENode::FLAG_PROP_TRANS);
                    node->update();
                    node->fireChanged();
                }
            }
        }
    }
}

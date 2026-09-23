
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
    FENodeRotateEditor::~FENodeRotateEditor()
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
    FENodeRotateEditor::FENodeRotateEditor(const FENodeRotateEditor& other)
        :FEEditAxisRotate(other)
    {
        _priority = other.priority();
    }

    void    FENodeRotateEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        if (_objects.empty())
            mDelegate()     -=  this;
        else
            mDelegate()     +=  {this,&FENodeRotateEditor::onRAxis};
        sync();
    }
    void    FENodeRotateEditor::sync()
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
        auto        rad     =   DEG2RAD(relativeOffsetAngle);
        quatr       delta   =   FE::angleAxis(rad, normalize(axis));
        for (auto object : _objects)
        {
            auto    node    =   object->cast<FENode>();
            if (!node)
            {
                auto    com =   object->cast<FEComponent>();
                if (com && com->owner())
                    node    =   com->owner()->cast<FENode>();
            }
            if (!node)
                continue;
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

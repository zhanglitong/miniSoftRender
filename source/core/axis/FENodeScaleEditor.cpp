
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeScaleEditor.h"

namespace   FE
{
    FENodeScaleEditor::FENodeScaleEditor(FEContext& ctx)
        :FEEditAxisScale(ctx)
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
    FENodeScaleEditor::FENodeScaleEditor(const FENodeScaleEditor& other)
        :FEEditAxisScale(other)
    {
        _priority = other.priority();
    }
    FENodeScaleEditor::~FENodeScaleEditor()
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

    void    FENodeScaleEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        if (_objects.empty())
            mDelegate()     -=  this;
        else
            mDelegate()     +=  {this,&FENodeScaleEditor::onSAxis};
        sync();
    }
    void    FENodeScaleEditor::sync()
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
    void    FENodeScaleEditor::onSAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisScale& sender)
    {
        (void)status;
        (void)absoluteOffset;
        real3       pivot   =   position();
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
            ///围绕编辑器位置缩放节点
            real3   worldPos    =   node->globalTranslation();
            real3   offset      =   worldPos - pivot;
            real3   newWorldPos =   pivot + offset * relativeOffset;
            node->setGlobalTranslation(newWorldPos);
            ///缩放节点大小
            real3   localScale  =   real3(node->localScaling()) * relativeOffset;
            node->setLocalScaling(localScale);
            node->update();
            node->fireChanged();
        }
    }
}

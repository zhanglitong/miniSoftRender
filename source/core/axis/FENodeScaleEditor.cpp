
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

        mDelegate()     +=  {this,&FENodeScaleEditor::onSAxis};
    }
    FENodeScaleEditor::FENodeScaleEditor(const FENodeScaleEditor& other)
        :FEEditAxisScale(other)
    {
        _priority       =   other.priority();
        mDelegate()     +=  {this,&FENodeScaleEditor::onSAxis};
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
        mDelegate()                                     -=  this;
    }

    void    FENodeScaleEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        sync();
    }
    void    FENodeScaleEditor::sync()
    {
        aabb3dr box;
        for (auto& var: _objects)
        {
            auto    val =   var->getProperty(PROP_G_TRANSFORM_XYZ);
            if (std::holds_alternative<real3>(val))
                box.merge(std::get<real3>(val));
        }
        setTranslation(box.center());
    }
    void    FENodeScaleEditor::onSAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisScale& sender)
    {
        UNUSED(status,absoluteOffset,sender);
        real3       pivot   =   position();
        for (auto object : _objects)
        {
            bool    bModify =   false;
            /// 读取全局位置,围绕编辑器位置缩放
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            real3   globalPos   =   std::get<real3>(gval);
            real3   offset      =   globalPos - pivot;
            real3   newGlobalPos=   pivot + offset * relativeOffset;
            real3   posDelta    =   newGlobalPos - globalPos;
            /// 读取局部位置,叠加位置增量
            auto    lval    =   object->getProperty(PROP_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(lval))
                continue;
            real3   localPos    =   std::get<real3>(lval);
            real3   newLocalPos =   localPos + posDelta;
            /// 读取局部缩放,乘以缩放系数
            auto    sval    =   object->getProperty(PROP_SCALE_XYZ);
            real3   curScale;
            if (std::holds_alternative<real3>(sval))
                curScale    =   std::get<real3>(sval);
            else if (std::holds_alternative<float3>(sval))
                curScale    =   real3(std::get<float3>(sval));
            else
                continue;
            real3   newScale    =   curScale * relativeOffset;

            object->beginSetProp();
            if  (newLocalPos != localPos)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            if  (newScale != curScale)
                bModify |=  object->setProperty(PROP_SCALE_XYZ,newScale);
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}

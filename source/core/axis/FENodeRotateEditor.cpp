
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

        mDelegate()     +=  {this,&FENodeRotateEditor::onRAxis};
    }
    FENodeRotateEditor::FENodeRotateEditor(const FENodeRotateEditor& other)
        :FEEditAxisRotate(other)
    {
        _priority       =   other.priority();
        mDelegate()     +=  {this,&FENodeRotateEditor::onRAxis};
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
        mDelegate()                                     -=  this;
    }

    void    FENodeRotateEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        sync();
    }
    void    FENodeRotateEditor::sync()
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
    void    FENodeRotateEditor::onRAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& axis
                                        , real relativeOffsetAngle
                                        , real absoluteOffsetAngle
                                        , FEEditAxisRotate& sender)
    {
        UNUSED(status,absoluteOffsetAngle,sender);
        real3       pivot   =   position();
        auto        rad     =   DEG2RAD(relativeOffsetAngle);
        quatr       delta   =   FE::angleAxis(rad, normalize(axis));
        quatf       deltaF  =   quatf((float)delta.w,(float)delta.x,(float)delta.y,(float)delta.z);
        for (auto object : _objects)
        {
            bool    bModify =   false;
            /// 读取全局位置,围绕编辑器位置旋转
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            real3   globalPos   =   std::get<real3>(gval);
            real3   offset      =   globalPos - pivot;
            real3   newGlobalPos=   pivot + delta * offset;
            real3   posDelta    =   newGlobalPos - globalPos;
            /// 读取局部位置,叠加位置增量
            auto    lval    =   object->getProperty(PROP_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(lval))
                continue;
            real3   localPos    =   std::get<real3>(lval);
            real3   newLocalPos =   localPos + posDelta;
            /// 读取局部旋转,叠加旋转增量
            auto    rval    =   object->getProperty(PROP_QUAT);
            if (!std::holds_alternative<quatf>(rval))
                continue;
            quatf   curRot      =   std::get<quatf>(rval);
            quatf   newRot      =   normalize(deltaF * curRot);

            object->beginSetProp();
            if  (newLocalPos != localPos)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            if  (newRot != curRot)
                bModify |=  object->setProperty(PROP_QUAT,newRot);
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}


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
        /// 开始/结束没有增量角度,只有 Editting 才有
        if (relativeOffsetAngle == 0.0)
            return;
        real3       axisDir     =   normalize(axis);
        /// 禁用编辑轴:与某主轴对齐且该轴被禁用时忽略本次旋转
        if (_axisSetting != nullptr && _axisSetting->axisDisabled(axisDir))
            return;

        const   real3   editorPos   =   position();
        auto            rad         =   DEG2RAD(relativeOffsetAngle);
        quatr           delta       =   FE::angleAxis(rad, axisDir);
        quatf           deltaF      =   quatf((float)delta.w,(float)delta.x,(float)delta.y,(float)delta.z);
        for (size_t i = 0; i < _objects.size(); ++i)
        {
            Object  object  =   _objects[i];
            bool    bModify =   false;
            /// 编辑过程从 owner 读取数据;旋转中心由编辑轴配置决定
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            real3   globalPos   =   std::get<real3>(gval);
            real3   pivot       =   _axisSetting ? _axisSetting->rotationCenter(editorPos,globalPos)
                                                : editorPos;
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
            /// 节点的累计欧拉角账本由节点自己维护:用四元数写入的同时把账本展开到
            /// 上一次账本附近,拖拽超过 180° 甚至多圈都能继续累加
            quatf   curRot      =   std::get<quatf>(rval);
            quatf   newRot      =   normalize(deltaF * curRot);
            FENode* node        =   object->cast<FENode>();

            object->beginSetProp();
            /// 编辑只写对象自身属性,不经过动画系统:动画数据由打帧动作单独记录
            if  (newLocalPos != localPos)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            if  (newRot != curRot)
            {
                if (node != nullptr)
                    node->setLocalRotationAccumulated(quatr(newRot));
                else
                    bModify |=  object->setProperty(PROP_QUAT,newRot);
                bModify =   true;
            }
            /// 绕轴心的位置变化已经写入节点本地位置(随关键帧一起保存),
            /// 回放时按采样值直接还原,不再额外记录旋转中心
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}


#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeRotateEditor.h"
#include    "../../inc/animation/FEAnimationSys.hpp"

namespace   FE
{
    /// <summary>
    /// 世界点转换到节点父空间:动画的 _pivot 与 _position 同处父空间
    /// P = G * L^-1,故 p_parent = L * G^-1 * world
    /// </summary>
    static  real3   parentSpacePivot(FENode& node,const real3& world)
    {
        mat4r   localM  =   FE::makeTransform<real>(node.localTranslation(),
                                                    float3(node.localScaling()),
                                                    quatf(node.localRotation()));
        real4   point   =   FE::inverse(node.globalTransform()) * real4(world,real(1.0));
        point           =   localM * point;
        return  real3(point);
    }

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
        real3       axisDir     =   normalize(axis);
        /// 禁用编辑轴:与某主轴对齐且该轴被禁用时忽略本次旋转
        if (_axisSetting != nullptr && _axisSetting->axisDisabled(axisDir))
            return;

        const   real3   editorPos   =   position();
        auto            animSys     =   _ctx.scene()->animationSystem();
        auto            rad         =   DEG2RAD(relativeOffsetAngle);
        quatr           delta       =   FE::angleAxis(rad, axisDir);
        quatf           deltaF      =   quatf((float)delta.w,(float)delta.x,(float)delta.y,(float)delta.z);
        for (auto object : _objects)
        {
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
            quatf   curRot      =   std::get<quatf>(rval);
            quatf   newRot      =   normalize(deltaF * curRot);

            object->beginSetProp();
            /// 关键帧写入:同时更新 owner(节点)与动画本地副本
            if  (newLocalPos != localPos)
            {
                if (animSys != nullptr)
                    bModify |=  animSys->setNodeProperty(object,PROP_TRANSFORM_XYZ,newLocalPos);
                else
                    bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            }
            if  (newRot != curRot)
            {
                if (animSys != nullptr)
                    bModify |=  animSys->setNodeProperty(object,PROP_QUAT,newRot);
                else
                    bModify |=  object->setProperty(PROP_QUAT,newRot);
            }
            /// 旋转中心写入动画本地数据,驱动阶段按该点计算节点变换;
            /// 绕对象自身位置旋转时补偿量为 0,无需写入(写入只会引入浮点噪声,
            /// 之后每次 applyTransform 都按该轴心补偿,造成位置漂移)
            if (animSys != nullptr && pivot != globalPos)
            {
                auto    anim    =   animSys->animationOf(object);
                auto    node    =   object->cast<FENode>();
                if (anim != nullptr && node != nullptr)
                    anim->setRotationCenter(parentSpacePivot(*node,pivot));
            }
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}


#include    "../../inc/FEContext.hpp"
#include    "../../inc/FECamera.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeRotateEditor.h"

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 按编辑轴配置生成抓手的变换矩阵:位置由调用者决定,朝向由坐标系类型决定
        /// </summary>
        mat4r   calcAxisTransform(const FEAxisSetting* setting,Objects& objects
                                , FEContext& ctx,const real3& pos)
        {
            quatr   rot(1.0,0.0,0.0,0.0);
            if (setting != nullptr)
            {
                FENode* node    =   objects.empty() ? nullptr : objects.front()->cast<FENode>();
                switch(setting->coordType)
                {
                case    ACT_Parent:
                    if  (node != nullptr && node->parent() != nullptr)
                        rot =   node->parent()->globalRotation();
                    break;
                case    ACT_Local:
                    if  (node != nullptr)
                        rot =   node->globalRotation();
                    break;
                case    ACT_Screen:
                {
                    FECamera&   cam     =   ctx.activeCamera();
                    real3       dir     =   normalize(cam.getDir());
                    real3       right   =   normalize(cross(dir,cam.getUp()));
                    real3       up      =   normalize(cross(right,dir));
                    mat4r       mat(1.0);
                    mat[0]  =   real4(right,0.0);
                    mat[1]  =   real4(up,0.0);
                    mat[2]  =   real4(dir,0.0);
                    mat[3]  =   real4(pos,1.0);
                    return  mat;
                }
                default:
                    break;
                }
            }
            mat4r   mat     =   mat4_cast(rot);
                    mat[3]  =   real4(pos,1.0);
            return  mat;
        }
    }

    FENodeRotateEditor::FENodeRotateEditor(FEContext& ctx)
        :FEEditAxisRotate(ctx)
    {
        /// 默认优先级最高,最先执行
        _priority.setPriority(EP_Fist);
        _priority.setOrder(0);

        /// 增加节点通知
        ctx.nodeTree().eventsAddNode() += {this,[this](const FENode* )
        {
            sync();
        }};
        /// 移除节点通知
        ctx.nodeTree().eventsRemoveNode() += {this,[this](const FENode* obj)
        {
            auto    itr =   std::remove_if(_objects.begin(),_objects.end(),[obj](const Object& e) { return e.get() == obj; });
            _objects.erase(itr,_objects.end());
            sync();
        }};
        /// 清除节点通知
        ctx.nodeTree().eventsClear() += {this,[this]()
        {
            _objects    =   {};
            sync();
        }};
        /// 节点属性更改通知
        /// 旋转拖拽期间跳过 sync:否则抓手中心会跟着节点中心漂移,导致旋转结果错误
        ctx.nodeTree().eventsChangedNode() += {this,[this](const FENode*)
        {
            if  (isRotatting())
                return;
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
        _ctx.nodeTree().eventsAddNode()        -= {this};
        /// 移除节点通知
        _ctx.nodeTree().eventsRemoveNode()     -= {this};
        /// 移除节点通知
        _ctx.nodeTree().eventsClear()          -= {this};
        /// 移除节点属性更改通知
        _ctx.nodeTree().eventsChangedNode()    -= {this};
        mDelegate()                                     -=  this;
    }

    void    FENodeRotateEditor::setObjects(const Objects& objects)
    {
        _objects   =   objects;
        sync();
    }
    void    FENodeRotateEditor::applyTransform(const real3& pos)
    {
        setTransform(calcAxisTransform(_axisSetting,_objects,_ctx,pos));
    }
    void    FENodeRotateEditor::sync()
    {
        /// 拖拽期间不能重新计算中心:节点绕轴心移动会改变包围盒中心,造成中心漂移
        if  (isRotatting())
            return;
        /// 中心类型为用户时,抓手放在自定义轴心处,保证旋转围绕抓手所在位置进行
        if (_axisSetting != nullptr && _axisSetting->centerType == ACE_User)
        {
            applyTransform(_axisSetting->userCenter);
            return;
        }
        aabb3dr box;
        real3   sum(0.0);
        size_t  count   =   0;
        for (auto& var: _objects)
        {
            auto    val =   var->getProperty(PROP_G_TRANSFORM_XYZ);
            if (std::holds_alternative<real3>(val))
            {
                const   real3   pos =   std::get<real3>(val);
                box.merge(pos);
                sum     +=  pos;
                count   ++;
            }
        }
        /// 轴位置类型:包围盒中心 / 位移均值
        real3   pos =   (count > 0 && _axisSetting != nullptr
                        && _axisSetting->posType == APT_Translation)
                        ?   sum / real(count)
                        :   box.center();
        applyTransform(pos);
    }
    void    FENodeRotateEditor::recordRotationCenter()
    {
        /// 编辑轴模式只改自定义轴向,不涉及节点姿态,无需记录
        if  (_axisSetting != nullptr && _axisSetting->editType == AET_Axis)
            return;
        const   real3   editorPos   =   position();
        for (auto& var : _objects)
        {
            FENode* node    =   var->cast<FENode>();
            if  (node == nullptr)
                continue;
            auto    gval    =   var->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            const   real3   globalPos   =   std::get<real3>(gval);
            const   real3   pivot       =   _axisSetting
                                            ? _axisSetting->rotationCenter(editorPos,globalPos)
                                            : editorPos;
            /// 轴心换算到父空间:与局部位置/旋转增量使用同一空间;
            /// 写入节点属性后,动画回放据此把位置重建到圆弧上
            real3   parentPivot =   pivot;
            if  (node->parent() != nullptr)
                parentPivot     =   real3(inverse(node->parent()->globalTransform()) * real4(pivot,1.0));
            node->setRotationCenter(parentPivot);
        }
    }
    void    FENodeRotateEditor::onRAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& axis
                                        , real relativeOffsetAngle
                                        , real absoluteOffsetAngle
                                        , FEEditAxisRotate& sender)
    {
        UNUSED(absoluteOffsetAngle,sender);
        /// 编辑状态通知:宿主在拖动开始/结束时记录撤销起点与终点
        if (_editNotify)
            _editNotify(status);
        /// 拖拽开始时把本次旋转的轴心写入节点属性(节点姿态本身不参与)
        if  (status == EditStatus::EditStart)
        {
            recordRotationCenter();
            return;
        }
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
        /// <summary>
        /// 编辑类型为"编辑轴"时只编辑自定义轴向:让该向量绕当前拖拽轴旋转并保持长度,
        /// 不修改节点。轴心由移动工具编辑
        /// </summary>
        if (_axisSetting != nullptr && _axisSetting->editType == AET_Axis)
        {
            const   real    axisLen     =   length(_axisSetting->customAxis);
            real3           newAxis     =   delta * _axisSetting->customAxis;
            const   real    newLen      =   length(newAxis);
            if  (axisLen > 0.0 && newLen > 0.0)
                _axisSetting->customAxis   =   newAxis * (axisLen / newLen);
            _ctx.requireNextFrame();
            return;
        }

        for (size_t i = 0; i < _objects.size(); ++i)
        {
            Object  object  =   _objects[i];
            FENode* node    =   object->cast<FENode>();
            if  (node == nullptr)
                continue;
            bool    bModify =   false;
            /// 编辑过程从 owner 读取数据;旋转中心由编辑轴配置决定
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            real3   globalPos   =   std::get<real3>(gval);
            real3   pivot       =   _axisSetting ? _axisSetting->rotationCenter(editorPos,globalPos)
                                                : editorPos;
            /// 绕轴心旋转:世界位置随旋转一起更新(位置变化随关键帧保存);
            /// 局部位置由父级逆变换换算,与交互结果保持一致
            real3   newGlobalPos=   pivot + delta * (globalPos - pivot);
            auto    lval    =   object->getProperty(PROP_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(lval))
                continue;
            real3   localPos    =   std::get<real3>(lval);
            mat4r   parentInv(1.0);
            if  (node->parent() != nullptr)
                parentInv   =   inverse(node->parent()->globalTransform());
            real3   newLocalPos =   real3(parentInv * real4(newGlobalPos,1.0));
            /// 读取局部旋转,叠加旋转增量
            auto    rval    =   object->getProperty(PROP_QUAT);
            if (!std::holds_alternative<quatf>(rval))
                continue;
            /// 世界空间的旋转增量需要先换算到父级旋转空间,再乘当前局部四元数
            quatr   localDelta  =   delta;
            if  (node->parent() != nullptr)
            {
                const   quatr   parentRot   =   node->parent()->globalRotation();
                localDelta  =   glm::inverse(parentRot) * delta * parentRot;
            }
            /// 节点的累计欧拉角账本由节点自己维护:用四元数写入的同时把账本展开到
            /// 上一次账本附近,拖拽超过 180° 甚至多圈都能继续累加
            quatf   curRot      =   std::get<quatf>(rval);
            quatr   newRot      =   glm::normalize(localDelta * quatr(curRot));
            quatf   newRotF     =   quatf((float)newRot.w,(float)newRot.x,(float)newRot.y,(float)newRot.z);

            object->beginSetProp();
            /// 编辑只写对象自身属性,不经过动画系统:动画数据由打帧动作单独记录
            if  (newLocalPos != localPos)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            if  (newRotF != curRot)
            {
                node->setLocalRotationAccumulated(newRot);
                bModify =   true;
            }
            /// 绕轴心的位置变化已经写入节点本地位置(随关键帧一起保存);
            /// 回放时需要按旋转重建位置,由动画的绕轴心记录负责
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}

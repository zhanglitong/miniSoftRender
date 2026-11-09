
#include    "../../inc/FEContext.hpp"
#include    "../../inc/FECamera.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeMoveEditor.h"

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

   
    FENodeMoveEditor::FENodeMoveEditor(FEContext& ctx)
        :FEEditAxisMove(ctx)
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
        /// 拖拽期间跳过 sync, 避免 editor 位置变动导致 calcMove 参考偏移 → 抖动
        ctx.nodeTree().eventsChangedNode() += {this,[this](const FENode*)
        {
            if  (isAxisSelected())
                return;
            sync();
        }};

        mDelegate()     +=  {this,&FENodeMoveEditor::onMAxis};
    }
    FENodeMoveEditor::FENodeMoveEditor(const FENodeMoveEditor& other)
        :FEEditAxisMove(other)
    {
        _priority       =   other.priority();
        mDelegate()     +=  {this,&FENodeMoveEditor::onMAxis};
    }
    FENodeMoveEditor::~FENodeMoveEditor()
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

    void    FENodeMoveEditor::setObjects(const Objects& objects)
    {
        /// 这里注意，进来后就
        _objects   =   objects;
        sync();
    }
    void    FENodeMoveEditor::applyTransform(const real3& pos)
    {
        setTransform(calcAxisTransform(_axisSetting,_objects,_ctx,pos));
    }
    void    FENodeMoveEditor::sync()
    {
        /// 拖拽期间抓手实时跟随节点:控制的移动量都落在当前拾取平面内,
        /// 平面的参考点随之平移但平面本身不变,因此不会产生偏移抖动
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

    void    FENodeMoveEditor::onMAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisMove& sender)
    {
        UNUSED(absoluteOffset,sender);
        /// 编辑状态通知:宿主在拖动开始/结束时记录撤销起点与终点
        if (_editNotify)
            _editNotify(status);
        /// 禁用轴上的增量归零:平面拖动时只保留允许轴的分量
        real3   offset  =   relativeOffset;
        if  (_axisSetting != nullptr)
        {
            if  (_axisSetting->axisDisabled(0))  offset.x    =   real(0.0);
            if  (_axisSetting->axisDisabled(1))  offset.y    =   real(0.0);
            if  (_axisSetting->axisDisabled(2))  offset.z    =   real(0.0);
        }
        /// <summary>
        /// 编辑类型为"编辑轴"且中心类型为用户时:拖动只修改自定义轴心,不移动节点。
        /// 编辑完成后切回"对象"时,旋转/缩放按该轴心进行操作
        /// </summary>
        if (_axisSetting != nullptr && _axisSetting->editType == AET_Axis)
        {
            if (_axisSetting->centerType == ACE_User)
            {
                _axisSetting->userCenter += offset;
                /// 抓手跟随新的轴心位置
                applyTransform(_axisSetting->userCenter);
            }
            return;
        }

        /// 实际发生平移后,节点绕轴心旋转的位置关系不再成立:清除节点上的旋转轴心
        if  (status == EditStatus::Editting && offset != real3(0.0))
        {
            for (auto& object : _objects)
            {
                FENode* node    =   object->cast<FENode>();
                if  (node != nullptr)
                    node->clearRotationCenter();
            }
        }

        /// 编辑只写对象自身的属性,不经过动画系统:
        /// 动画数据由打帧动作单独记录,编辑过程不应触碰动画本地副本
        for (auto object : _objects)
        {
            bool    bModify =   false;
            /// 世界位置用于换算父级空间下的局部目标位置
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            auto    lval    =   object->getProperty(PROP_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(lval))
                continue;

            real3   curGlobal   =   std::get<real3>(gval);
            real3   newGlobal   =   curGlobal + offset;
            real3   curVal      =   std::get<real3>(lval);
            /// 父节点存在时,世界目标位置需要逆变换回局部空间
            FENode* node        =   object->cast<FENode>();
            mat4r   parentInv(1.0);
            if  (node != nullptr && node->parent() != nullptr)
                parentInv   =   inverse(node->parent()->globalTransform());
            real3   nValue      =   real3(parentInv * real4(newGlobal,1.0));

            object->beginSetProp();
            /// 仅在实际有变化时才 setProperty (跳过 EditStart/EditEnd 的 0 偏移)
            if  (nValue != curVal)
                bModify =   object->setProperty(PROP_TRANSFORM_XYZ,nValue);
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}

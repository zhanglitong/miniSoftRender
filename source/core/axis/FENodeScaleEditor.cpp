
#include    "../../inc/FEContext.hpp"
#include    "../../inc/FECamera.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/axis/FENodeScaleEditor.h"

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
        /// 缩放拖拽期间跳过 sync:否则缩放的枢轴会跟随节点漂移
        ctx.scene()->nodeTree().eventsChangedNode() += {this,[this](const FENode*)
        {
            if  (isScaleing())
                return;
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
        /// 拖拽期间枢轴必须保持稳定
        if  (isScaleing())
            return;
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
        setTransform(calcAxisTransform(_axisSetting,_objects,_ctx,pos));
    }
    void    FENodeScaleEditor::onSAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisScale& sender)
    {
        UNUSED(absoluteOffset,sender);
        /// 编辑状态通知:宿主在拖动开始/结束时记录撤销起点与终点
        if (_editNotify)
            _editNotify(status);
        /// "编辑轴"模式只用于编辑轴本身,不缩放节点
        if  (_axisSetting != nullptr && _axisSetting->editType == AET_Axis)
            return;
        /// 禁用轴上的缩放系数保持 1
        real3   factor  =   relativeOffset;
        if  (_axisSetting != nullptr)
        {
            if  (_axisSetting->axisDisabled(0))  factor.x    =   real(1.0);
            if  (_axisSetting->axisDisabled(1))  factor.y    =   real(1.0);
            if  (_axisSetting->axisDisabled(2))  factor.z    =   real(1.0);
        }
        /// 实际发生缩放后,节点绕轴心旋转的位置关系不再成立:清除节点上的旋转轴心
        if  (status == EditStatus::Editting && factor != real3(1.0))
        {
            for (auto& object : _objects)
            {
                FENode* node    =   object->cast<FENode>();
                if  (node != nullptr)
                    node->clearRotationCenter();
            }
        }
        /// 编辑只写对象自身的属性,不经过动画系统:动画数据由打帧动作单独记录
        const   real3   editorPos   =   position();
        for (auto object : _objects)
        {
            bool    bModify =   false;
            /// 读取全局位置;枢轴由中心类型决定
            auto    gval    =   object->getProperty(PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(gval))
                continue;
            real3   globalPos   =   std::get<real3>(gval);
            real3   pivot       =   _axisSetting ? _axisSetting->rotationCenter(editorPos,globalPos)
                                                : editorPos;
            real3   newGlobalPos=   pivot + (globalPos - pivot) * factor;
            /// 读取局部位置,用父级逆变换把世界目标换算成局部目标
            auto    lval    =   object->getProperty(PROP_TRANSFORM_XYZ);
            if (!std::holds_alternative<real3>(lval))
                continue;
            real3   localPos    =   std::get<real3>(lval);
            FENode* node        =   object->cast<FENode>();
            mat4r   parentInv(1.0);
            if  (node != nullptr && node->parent() != nullptr)
                parentInv   =   inverse(node->parent()->globalTransform());
            real3   newLocalPos =   real3(parentInv * real4(newGlobalPos,1.0));
            /// 读取局部缩放,乘以缩放系数
            auto    sval    =   object->getProperty(PROP_SCALE_XYZ);
            real3   curScale;
            if (std::holds_alternative<real3>(sval))
                curScale    =   std::get<real3>(sval);
            else if (std::holds_alternative<float3>(sval))
                curScale    =   real3(std::get<float3>(sval));
            else
                continue;
            real3   newScale    =   curScale * factor;

            object->beginSetProp();
            if  (newLocalPos != localPos)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,newLocalPos);
            if  (newScale != curScale)
                bModify |=  object->setProperty(PROP_SCALE_XYZ,newScale);
            object->endSetProp(_ctx.deltaTime(),bModify);
        }
    }
}

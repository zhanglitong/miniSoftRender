
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
        /// 拖拽期间跳过 sync, 避免 editor 位置变动导致 calcMove 参考偏移 → 抖动
        ctx.scene()->nodeTree().eventsChangedNode() += {this,[this](const FENode*)
        {
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
        _ctx.scene()->nodeTree().eventsAddNode()        -= {this};
        /// 移除节点通知
        _ctx.scene()->nodeTree().eventsRemoveNode()     -= {this};
        /// 移除节点通知
        _ctx.scene()->nodeTree().eventsClear()          -= {this};
        /// 移除节点属性更改通知
        _ctx.scene()->nodeTree().eventsChangedNode()    -= {this};
        mDelegate()                                     -=  this;
    }

    void    FENodeMoveEditor::setObjects(const Objects& objects)
    {
        /// 这里注意，进来后就
        _objects   =   objects;
        sync();
    }
    void    FENodeMoveEditor::sync()
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

    void    FENodeMoveEditor::onMAxis(
                                        FEEditAxis::EditStatus status
                                        , const real3& relativeOffset
                                        , const real3& absoluteOffset
                                        , FEEditAxisMove& sender)
    {
        UNUSED(status,absoluteOffset,sender);

        for (auto object : _objects)
        {
            
            bool    bModify =   false;
            auto    val     =   object->getProperty(PROP_TRANSFORM_XYZ);

            if (!std::holds_alternative<real3>(val))
                continue;
            object->beginSetProp();
            real3   curVal  =   std::get<real3>(val);
            real3   nValue  =   curVal + relativeOffset;
            /// 仅在实际有变化时才 setProperty (跳过 EditStart/EditEnd 的 0 偏移)
            if  (nValue != curVal)
                bModify |=  object->setProperty(PROP_TRANSFORM_XYZ,nValue);
            object->endSetProp(bModify);
        }
    }
}

#pragma     once
#include    "../FEComponentSys.hpp"
#include    "../FEObjectsTemplate.hpp"
#include    "../node/FENode.hpp"
#include    "../graphic/FEScene.h"
#include    "FEConstraint.hpp"
#include    <map>

namespace FE
{
    /// <summary>
    /// 按照优先级排序,优先级相同的按照order排序
    /// </summary>
    constexpr   auto    ConstComLessFunc = [](const Constraint& l, const Constraint& r)
    {
        if( l->priority().priority() == r->priority().priority())
            return  l->priority().order() < r->priority().order();
        else
            return  l->priority().priority() < r->priority().priority();
    };
    /// <summary>
    /// 推导出来类型
    /// </summary>
    using   ConstComLess    =   decltype(ConstComLessFunc);

    DEFINE_CLASS_UUID(FEConstraintSys, "{5ED9E0B6-D627-4E1A-BAE4-4716FD31052F}");
    class   FEConstraintSys : public FEComponentSys
    {
    public:
        using   ConstComs   =   FEObjectsTemplate<Constraint, ConstComLess>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintSys)
    public:       
        FEConstraintSys(FEContext& ctx)
            :FEComponentSys(ctx)
            ,_objects(ConstComLessFunc)
        {
            /// 约束在动画之后求解(动画 MaxInt16-2,约束 MaxInt16-1)
            priority(FEFactory::PT_Update).setOrder(MaxInt16 - 1);
        }
        FEConstraintSys(const FEConstraintSys& other)
            :FEComponentSys(other)
            ,_objects(ConstComLessFunc)
        {
            priority(FEFactory::PT_Update).setOrder(MaxInt16 - 1);
        }
        /// <summary>
        /// 根据Id查找
        /// </summary>
        /// <param name="objectId"></param>
        /// <returns></returns>
        inline  auto    query(const FEUuid& objectId)const ->Component 
        {
            return  _objects.query(objectId).get();
        }
        virtual size_t  addObject(Component  com) override
        {
            if (com->cast<FEConstraint>())
                return  _objects.addObject(com->cast<FEConstraint>());
            else
                return  0;
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            size_t  result = 0;
            for (auto var : coms)
            {
                result += addObject(var);
            }
            return  result;
        }
        virtual size_t  removeObject(Component com) override
        {
            if (com->cast<FEConstraint>())
                return  _objects.removeObject(com->cast<FEConstraint>());
            else
                return  0;
        }
        virtual size_t  removeObjects(const Components& coms) override
        {
            size_t  result  = 0;
            for (auto var : coms)
            {
                result += removeObject(var);
            }
            return  result;
        }
        /// <summary>
        /// 清空
        /// </summary>
        virtual void    clear() override
        {
            _objects.clearObjects();
        }
        /// <summary>
        /// 每帧遍历约束并求解：
        /// 取 owner 节点当前世界变换 -> solve() -> 将结果世界位置写回节点，
        /// 并加入 scene 更新列表(引擎随后统一 node->update + fireChanged，
        /// 刷新实例数据)。禁用约束跳过
        /// 同一节点挂多个约束时,按优先级链式传递:
        ///   后一个约束的 currentWorld 使用前一个约束的 solved 结果,
        ///   使 Follow(EP_Second) 写入的旋转能被 Path(EP_Last) 保留
        /// </summary>
        /// <param name="tmDelta"></param>
        virtual void    update(const real& tmDelta) override
        {
            _time   +=  tmDelta;
            std::map<FENode*, FETransform>  lastSolved;
            FEConstraint::SolvedNodes       solvedNodes;
            for (auto& con : _objects.objects())
            {
                if (!con->isEnable())
                    continue;
                Node    node    =   con->owner() ? con->owner()->cast<FENode>() : nullptr;
                if (node == nullptr)
                    continue;
                auto    it      =   lastSolved.find(node.get());
                FETransform current   =   (it != lastSolved.end()) ? it->second : node->globalFETransform();
                /// 求解本约束影响的所有节点(默认仅 owner,驱动约束包含多个被驱动节点)
                solvedNodes.clear();
                if (con->solveNodes(node,current,_time,solvedNodes) == 0)
                    continue;
                for (auto& var : solvedNodes)
                {
                    auto    target  =   var._node;
                    if (target == nullptr)
                        continue;
                    target->setGlobalTranslation(var._transform.position());
                    target->setGlobalRotation(quatr(var._transform.rotation()));
                    /// 缩放仅在结果与当前全局缩放不一致时写回,避免无谓的浮点误差累积
                    if (var._transform.scale() != target->globalFETransform().scale())
                    {
                        target->setGlobalScaling(real3(var._transform.scale()));
                        target->flags().addFlag(FENode::FLAG_PROP_SCALE);
                    }
                    lastSolved[target.get()]  =   var._transform;
                    /// 加入本系统的更新列表,由场景在 update 之后显式刷新
                    updateList().addObject(Object(target.get()));
                }
            }
        }
    protected:
        ConstComs   _objects;
        /// <summary>
        /// 约束求解累计时间(秒)，作为 solve 的时间参数
        /// </summary>
        real        _time   =   0;
    };
}


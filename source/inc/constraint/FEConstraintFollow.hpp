#pragma     once

#include    "FEConstraint.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEConstraintFollow, "{C8D9E0F1-1234-5678-9ABC-DEF012345678}");

    /// <summary>
    /// 跟随约束：使节点完整跟随目标节点的变换(位置 + 旋转)
    /// 偏移记录在目标本地坐标系中(offset 会随目标旋转而旋转),
    /// 与 FEConstraintTrans(仅位置,世界空间偏移) 不同
    ///
    /// 典型场景：被约束节点同时挂跟随约束 + 路径约束(FEConstraintPath)
    ///   - 跟随约束先执行(EP_Second): 写入 target 旋转 + 本地偏移位置
    ///   - 路径约束后执行(EP_Last): 覆盖位置为路径点,保留跟随写入的旋转
    ///   - 最终: 节点沿路径移动,同时旋转跟随目标
    ///
    /// 用法:
    ///   ConstraintFollow follow = new FEConstraintFollow(ctx);
    ///   follow->setTargetNode(leaderNode);
    ///   node->addComponent(follow.get());
    /// </summary>
    class   FE_API  FEConstraintFollow : public FEConstraint
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintFollow)
    public:
        FEConstraintFollow(FEContext& ctx)
            :FEConstraint(ctx)
        {
            /// 跟随约束优先级 EP_Second,先于 EP_Last(路径等)执行
            _priority.setPriority(EPriority::EP_Second);
            _priority.setOrder(0);
        }
        FEConstraintFollow(const FEConstraintFollow& other)    =   default;
        ~FEConstraintFollow()                                =   default;
    public:
        /// <summary>
        /// 固定目标点(零初始化)
        /// </summary>
        inline  const real3&   target() const
        {
            return  _target;
        }
        /// <summary>
        /// 设置固定目标点
        /// </summary>
        inline  void    setTarget(const real3& point)
        {
            _target =   point;
        }
        /// <summary>
        /// 绑定的目标节点(可能为空)；有效时每帧取其完整世界变换
        /// </summary>
        inline  Node    targetNode() const
        {
            return  _targetNode;
        }
        /// <summary>
        /// 绑定/解绑目标节点；需要保持当前偏移时可随后调用 recordOffset
        /// </summary>
        inline  void    setTargetNode(Node node)
        {
            _targetNode =   node;
        }
        /// <summary>
        /// 是否在附加到节点时自动记录偏移(默认 true)
        /// </summary>
        inline  bool    maintainOffset() const
        {
            return  _maintainOffset;
        }
        /// <summary>
        /// 设置附加时是否自动记录偏移
        /// </summary>
        inline  void    setMaintainOffset(bool bFlag)
        {
            _maintainOffset =   bFlag;
        }
    public:
        /// <summary>
        /// 求解: 把节点变换对齐到 target * offset
        /// 位置 = target.pos + target.rot * offset.pos(本地系偏移,随目标旋转)
        /// 旋转 = target.rot * offset.rot
        /// </summary>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const override
        {
            UNUSED(time);
            UNUSED(currentWorld);
            FETransform   desired   =   currentWorld;
            FETransform   targetXform   =   effectiveTargetTransform();
            quatr   targetRot   =   quatr(targetXform.rotation());
            /// 位置 = target.pos + target.rot * offset.pos
            desired.setPosition(targetXform.position() + targetRot * _offset.position());
            /// 旋转 = target.rot * offset.rot
            desired.setRotation(quatf(targetRot * quatr(_offset.rotation())));
            return  desired;
        }
        /// <summary>
        /// 附加到节点时按 maintainOffset 自动记录偏移,
        /// 使约束激活瞬间不发生位移/旋转跳变
        /// </summary>
        virtual void    attach(Object owner) override
        {
            FEConstraint::attach(owner);
            if (_maintainOffset)
            {
                auto    node    =   _owner ? _owner->cast<FENode>() : nullptr;
                if (node)
                    recordOffset(node->globalFETransform());
            }
        }
        /// <summary>
        /// 记录当前相对目标的本地系偏移:
        ///   offset.pos = inverse(target.rot) * (node.pos - target.pos)
        ///   offset.rot = inverse(target.rot) * node.rot
        /// 这样 target 改变前 solve() 不会改变节点变换
        /// </summary>
        inline  void    recordOffset(const FETransform& currentWorld)
        {
            FETransform targetXform =   effectiveTargetTransform();
            quatr   targetRot   =   quatr(targetXform.rotation());
            real3   relPos      =   currentWorld.position() - targetXform.position();
            _offset.setPosition(glm::inverse(targetRot) * relPos);
            _offset.setRotation(quatf(glm::inverse(targetRot) * quatr(currentWorld.rotation())));
        }
    protected:
        /// <summary>
        /// 当前生效的目标完整变换：优先取目标节点世界变换,否则用固定目标点构造
        /// </summary>
        inline  FETransform  effectiveTargetTransform() const
        {
            if (_targetNode)
                return  _targetNode->globalFETransform();
            FETransform t;
            t.setPosition(_target);
            return  t;
        }
    protected:
        /// <summary>
        /// 固定目标点(零初始化)
        /// </summary>
        real3       _target         =   real3(0);
        /// <summary>
        /// 绑定的目标节点；有效时优先于 _target
        /// </summary>
        Node        _targetNode;
        /// <summary>
        /// 附加时是否自动记录偏移
        /// </summary>
        bool        _maintainOffset =   true;
    };

    using   ConstraintFollow =   SharedPtr<FEConstraintFollow>;
}

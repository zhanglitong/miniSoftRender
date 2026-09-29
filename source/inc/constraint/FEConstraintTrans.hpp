#pragma     once

#include    "FEConstraint.hpp"
#include    "../node/FENode.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEConstraintTrans, "{BB054C42-4B55-4AC6-8A3A-8878BA5E45D2}");

    /// <summary>
    /// 平移约束：使节点位置吸附到目标点 + _offset.position()
    /// 目标有两种来源：
    ///   - 绑定目标节点 setTargetNode()：每帧取该节点的世界坐标(典型跟随用法)
    ///   - 固定目标点 setTarget()
    /// 设置了目标节点时优先使用目标节点
    /// 默认 maintainOffset 为 true，组件附加到节点(attach)时自动记录
    /// 当前世界位置与目标的偏移，使吸附瞬间不发生位移；
    /// 设为 false 或显式调用 recordOffset 可手动控制
    /// </summary>
    class   FE_API  FEConstraintTrans :public FEConstraint
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintTrans)
    public:
        FEConstraintTrans(FEContext& ctx)
            :FEConstraint(ctx)
        {}
        FEConstraintTrans(const FEConstraintTrans& other)    =   default;
        ~FEConstraintTrans()                                =   default;
    public:
        /// <summary>
        /// 目标点
        /// </summary>
        inline  const real3&   target() const
        {
            return  _target;
        }
        /// <summary>
        /// 设置目标点；需要保持当前偏移时可随后调用 recordOffset
        /// </summary>
        inline  void    setTarget(const real3& point)
        {
            _target =   point;
        }
        /// <summary>
        /// 绑定的目标节点(可能为空)；有效时每帧取其世界坐标作为目标
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
        /// <summary>
        /// 调用该函数，返回约束结果
        /// </summary>
        /// <param name="currentWorld"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const override
        {
            (void)time;
            FETransform   desired   =   currentWorld;
            desired.setPosition(effectiveTarget() + _offset.position());
            return                  desired;
        }
        /// <summary>
        /// 关联所有者：附加到节点时按 maintainOffset 自动记录偏移，
        /// 使约束激活瞬间不发生位移
        /// </summary>
        /// <param name="owner"></param>
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
        /// 根据当前位姿和当前目标记录平移偏移
        /// 这样目标改变前 solve() 不会改变对象位置
        /// </summary>
        /// <param name="currentWorld"></param>
        inline  void    recordOffset(const FETransform& currentWorld)
        {
            _offset.setPosition(currentWorld.position() - effectiveTarget());
        }
    protected:
        /// <summary>
        /// 当前生效的目标位置：优先取目标节点世界坐标，否则用固定目标点
        /// </summary>
        inline  real3   effectiveTarget() const
        {
            return  _targetNode ? _targetNode->globalTranslation() : _target;
        }
    protected:
        /// <summary>
        /// 目标点(零初始化)
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

    using   ConstraintTrans =   SharedPtr<FEConstraintTrans>;
}

#pragma     once

#include    <algorithm>

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
    /// 另可逐轴限制取值范围：某轴 setAxisLimit(true) 后，求解结果在该轴被夹到
    /// [minValue,maxValue] 区间内(默认三轴均不限制，行为与旧版一致)
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
        /// 各轴取值范围下限
        /// </summary>
        inline  const real3&   minValue() const
        {
            return  _min;
        }
        /// <summary>
        /// 各轴取值范围上限
        /// </summary>
        inline  const real3&   maxValue() const
        {
            return  _max;
        }
        /// <summary>
        /// 设置各轴取值范围下限
        /// </summary>
        inline  void    setMinValue(const real3& value)
        {
            _min    =   value;
        }
        /// <summary>
        /// 设置各轴取值范围上限
        /// </summary>
        inline  void    setMaxValue(const real3& value)
        {
            _max    =   value;
        }
        /// <summary>
        /// 指定轴是否限制取值范围
        /// </summary>
        inline  bool    axisLimit(int axis) const
        {
            return  (axis >= 0 && axis < 3) ? _axisLimit[axis] : false;
        }
        /// <summary>
        /// 设置指定轴是否限制取值范围
        /// </summary>
        inline  void    setAxisLimit(int axis,bool bFlag)
        {
            if (axis >= 0 && axis < 3)
                _axisLimit[axis]    =   bFlag;
        }
        /// <summary>
        /// 调用该函数，返回约束结果；勾选的轴把结果夹到 [最小值,最大值] 内
        /// </summary>
        /// <param name="currentWorld"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const override
        {
            UNUSED(time);
            FETransform   desired   =   currentWorld;
            real3         position  =   effectiveTarget() + _offset.position();
            for (int i = 0; i < 3; ++i)
            {
                if (!_axisLimit[i])
                    continue;
                const   real    low     =   std::min(_min[i],_max[i]);
                const   real    high    =   std::max(_min[i],_max[i]);
                position[i] =   std::min(std::max(position[i],low),high);
            }
            desired.setPosition(position);
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
        /// <summary>
        /// 各轴取值范围下限(默认 -10)
        /// </summary>
        real3       _min            =   real3(-10.0f,-10.0f,-10.0f);
        /// <summary>
        /// 各轴取值范围上限(默认 10)
        /// </summary>
        real3       _max            =   real3(10.0f,10.0f,10.0f);
        /// <summary>
        /// 各轴是否限制取值范围(默认不限制,行为与旧版一致)
        /// </summary>
        bool        _axisLimit[3]   =   {   false,false,false   };
    };

    using   ConstraintTrans =   SharedPtr<FEConstraintTrans>;
}

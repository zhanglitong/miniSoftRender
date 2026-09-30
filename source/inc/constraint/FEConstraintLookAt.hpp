#pragma     once

#include    "FEConstraint.hpp"
#include    "../node/FENode.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEConstraintLookAt, "{3C7A5D91-8F24-4B6E-9A35-D21C84F0E7BA}");

    /// <summary>
    /// 朝向约束：使节点旋转对准目标(local +Z 轴指向目标方向)
    /// 目标有两种来源：
    ///   - 绑定目标节点 setTargetNode()：每帧取该节点的世界坐标(典型跟随用法)
    ///   - 固定目标点 setTarget()
    /// 设置了目标节点时优先使用目标节点
    /// upAxis 为朝向计算的上参考轴(默认 +Y)
    /// 注意：forward 与 upAxis 平行时结果未定义(万向锁)，使用时避免该姿态
    /// </summary>
    class   FE_API  FEConstraintLookAt :public FEConstraint
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintLookAt)
    public:
        FEConstraintLookAt(FEContext& ctx)
            :FEConstraint(ctx)
        {}
        FEConstraintLookAt(const FEConstraintLookAt& other)    =   default;
        ~FEConstraintLookAt()                                =   default;
    public:
        /// <summary>
        /// 目标点
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
        /// 绑定的目标节点(可能为空)；有效时每帧取其世界坐标作为目标
        /// </summary>
        inline  Node    targetNode() const
        {
            return  _targetNode;
        }
        /// <summary>
        /// 绑定/解绑目标节点
        /// </summary>
        inline  void    setTargetNode(Node node)
        {
            _targetNode =   node;
        }
        /// <summary>
        /// 上参考轴
        /// </summary>
        inline  const real3&   upAxis() const
        {
            return  _upAxis;
        }
        /// <summary>
        /// 设置上参考轴(勿与目标方向平行)
        /// </summary>
        inline  void    setUpAxis(const real3& up)
        {
            _upAxis =   up;
        }
        /// <summary>
        /// 调用该函数，返回约束结果(仅改旋转，不改平移与缩放)
        /// </summary>
        /// <param name="currentWorld"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const override
        {
            (void)time;
            
            real3       forward(1,0,0);
            real3       delta   =   effectiveTarget() - currentWorld.position();
            if (effectiveTarget() != currentWorld.position())
                forward =   normalize(effectiveTarget() - currentWorld.position());

            real3       right   =   normalize(cross(_upAxis, forward));
            real3       up      =   cross(forward, right);

            quatr       lookRot =   quatFromBasis(right, up, forward);
            FETransform desired =   currentWorld;
            desired.setRotation(quatf(glm::normalize(lookRot * quatr(_offset.rotation()))));
            return      desired;
        }
    protected:
        /// <summary>
        /// 当前生效的目标位置：优先取目标节点世界坐标，否则用固定目标点
        /// </summary>
        inline  real3   effectiveTarget() const
        {
            return  _targetNode ? _targetNode->globalTranslation() : _target;
        }
        /// <summary>
        /// 这是经典的 Shepperd 方法：根据矩阵迹（trace）的大小选择四个分支之一，避免除以接近零的数，数值上最稳定
        /// 由三个正交基向量构造四元数
        /// right, up, forward 为列向量，构成旋转矩阵 R
        /// </summary>
        /// <param name="right"></param>
        /// <param name="up"></param>
        /// <param name="forward"></param>
        /// <returns></returns>
        static  quatr   quatFromBasis(const real3& right, const real3& up, const real3& forward)
        {
            /// 矩阵元素（行主序）
            real    m00 = right.x,   m01 = up.x,   m02 = forward.x;
            real    m10 = right.y,   m11 = up.y,   m12 = forward.y;
            real    m20 = right.z,   m21 = up.z,   m22 = forward.z;

            real    trace = m00 + m11 + m22;
            quatr q;

            if (trace > 0.0f)
            {
                real s = std::sqrt(trace + 1.0f) * 2.0f;  // s = 4w
                q.w = 0.25f * s;
                q.x = (m21 - m12) / s;
                q.y = (m02 - m20) / s;
                q.z = (m10 - m01) / s;
            }
            else if (m00 > m11 && m00 > m22)
            {
                real s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;  // s = 4x
                q.w = (m21 - m12) / s;
                q.x = 0.25f * s;
                q.y = (m01 + m10) / s;
                q.z = (m02 + m20) / s;
            }
            else if (m11 > m22)
            {
                real s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;  // s = 4y
                q.w = (m02 - m20) / s;
                q.x = (m01 + m10) / s;
                q.y = 0.25f * s;
                q.z = (m12 + m21) / s;
            }
            else
            {
                real s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;  // s = 4z
                q.w = (m10 - m01) / s;
                q.x = (m02 + m20) / s;
                q.y = (m12 + m21) / s;
                q.z = 0.25f * s;
            }
            return glm::normalize(q);
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
        /// 朝向计算的上参考轴(默认世界 +Y)
        /// </summary>
        real3       _upAxis         =   real3(0, 1, 0);
    };

    using   ConstraintLookAt    =   SharedPtr<FEConstraintLookAt>;
}

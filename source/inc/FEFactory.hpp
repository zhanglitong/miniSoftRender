#pragma     once

#include    "FEObject.h"
#include    "FEPriority.hpp"

namespace   FE
{
    class   FE_API FEFactory :public FEObject
    {
    public:
        enum    PriorityType:uint32_t
        {
            /// <summary>
            /// 更新优先级
            /// </summary>
            PT_Update,
            /// <summary>
            /// 绘制优先级
            /// </summary>
            PT_Render,
            /// <summary>
            /// 拾取优先级
            /// </summary>
            PT_Pickup,
            PT_Max,
        };
        using   Prioritys     =   std::array<FEPriority,PT_Max>;
        /// <summary>
        /// 组件系统在 PT_Update 阶段内的执行顺序约定(数值小者先执行):
        /// 动画采样 -> 约束求解 -> 蒙皮,后一阶段依赖前一阶段写好的节点/关节世界矩阵。
        /// 依赖骨骼姿态的系统统一引用这里的取值,不要再各自写魔法数
        /// </summary>
        enum    UpdateOrder : int16_t
        {
            /// <summary>
            /// 未声明依赖的普通系统:默认排在骨骼管线之前,
            /// 需要读取骨骼姿态的系统必须显式声明为 UO_Skin 之后的值
            /// </summary>
            UO_Default      =   MaxInt16 - 3,
            /// <summary>
            /// 采样动画关键帧,写节点本地变换
            /// </summary>
            UO_Animation    =   MaxInt16 - 2,
            /// <summary>
            /// 约束求解,读取动画采样后的世界变换
            /// </summary>
            UO_Constraint   =   MaxInt16 - 1,
            /// <summary>
            /// 蒙皮,读取最终的关节世界矩阵
            /// </summary>
            UO_Skin         =   MaxInt16,
        };
        static_assert(UO_Default < UO_Animation,"默认系统必须排在动画之前");
        static_assert(UO_Animation < UO_Constraint,"动画必须在约束之前执行");
        static_assert(UO_Constraint < UO_Skin,"约束必须在蒙皮之前执行");
    public:
        enum  FactoryFlag
        {
            /// <summary>
            /// 添加对象
            /// </summary>
            FLAG_ADD_OBJECT     =   FlagBit::FLAG_LAST,
            /// <summary>
            /// 移除对象
            /// </summary>
            FLAG_REMOVE_OBJECT  =   (FLAG_ADD_OBJECT)<<1,
            /// <summary>
            /// 对象修改
            /// </summary>
            FLAG_MODIFY_OBJECT  =   (FLAG_REMOVE_OBJECT)<<1,
        };
        FEFactory(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEFactory(const FEFactory& other)
            :FEObject(other)
        {
            _prioritys  =   other._prioritys;
        }
        /// <summary>
        /// 返回优先级
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        FEPriority  operator[](size_t index) const
        {
            return  _prioritys[index];
        }
        FEPriority  priority(size_t index) const
        {
            return  _prioritys[index%_prioritys.size()];
        }
        FEPriority& riority(size_t index)
        {
            return  _prioritys[index%_prioritys.size()];
        }
    protected:
        Prioritys   _prioritys;
    };

    using   Factory     =   SharedPtr<FEFactory>;
    using   Factorys    =   std::vector<Factory>;
   
}


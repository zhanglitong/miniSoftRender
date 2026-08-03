#pragma     once

#include    "FEObject.h"
#include    "FEPriority.hpp"

namespace   FE
{
    class   FEFactory :public FEObject
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
        FEPriority      operator[](size_t index) const
        {
            return  _prioritys[index];
        }
        FEPriority      priority(size_t index) const
        {
            return  _prioritys[index%_prioritys.size()];
        }
    protected:
        Prioritys   _prioritys;
    };

    using   Factory     =   SharedPtr<FEFactory>;
    using   Factorys    =   std::vector<Factory>;
   
}


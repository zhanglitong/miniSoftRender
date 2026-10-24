#pragma     once

#include    "../FEContext.hpp"
#include    "../FEDelegate.hpp"
#include    "../FEComponent.hpp"
#include    "../FETransform.hpp"
#include    "../node/FENode.hpp"
#include    <vector>

namespace FE
{

    class   FE_API  FEConstraint : public FEComponent
    {
    public:
        FEConstraint(FEContext& ctx)
            :FEComponent(ctx)
        {
            /// 优先级最低，最后执行
            _priority.setPriority(EPriority::EP_Last);
            _priority.setOrder(MaxInt16);
        }
    public:
        /// <summary>
        /// 单个节点的求解结果(期望世界变换)
        /// </summary>
        struct  SolvedNode
        {
            Node        _node;      ///< 需要应用变换的节点
            FETransform _transform; ///< 期望世界变换
        };
        using   SolvedNodes     =   std::vector<SolvedNode>;
    public:
        /// <summary>
        /// 调用该函数，返回约束结果
        /// </summary>
        /// <param name="cur"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& cur,const real& time) const = 0;

        /// <summary>
        /// 求解本约束影响的所有节点: 默认仅处理 owner 自身
        /// 多节点约束(如驱动约束)重写该接口,返回 owner 与被驱动节点的期望世界变换,
        /// 由约束系统统一写回并刷新这些节点
        /// </summary>
        /// <param name="owner">约束所有者节点(由约束系统传入)</param>
        /// <param name="cur">owner 当前世界变换</param>
        /// <param name="time">约束系统累计时间</param>
        /// <param name="results">求解结果(调用方保证已清空)</param>
        /// <returns>求解的节点个数</returns>
        virtual size_t  solveNodes(Node owner,const FETransform& cur,const real& time,SolvedNodes& results) const
        {
            if (owner == nullptr)
                return  0;
            results.push_back({owner,solve(cur,time)});
            return  1;
        }

        /// <summary>
        /// 迭代类约束（IK 等）返回 true 表示还需继续迭代
        /// </summary>
        /// <returns></returns>
        virtual     bool    isIterative() const { return false; }
    protected:
        /// <summary>
        /// 创建时记录的偏移
        /// </summary>
        FETransform     _offset;            
    };

    using   Constraint    =   SharedPtr<FEConstraint>;
}


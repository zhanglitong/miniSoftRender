#pragma     once

#include    "FEConstraint.hpp"

namespace FE
{
    /// <summary>
    /// 平移约束：使节点位置吸附到 targetPoint + _offset.trans
    /// 默认 _offset.trans 为 0，即直接吸附到 targetPoint
    /// 调用 recordOffset 可在创建时记录相对偏移，使吸附瞬间不发生位移
    /// </summary>
    class   FEConstraintTrans :public FEConstraint
    {
    public:
        /// <summary>
        /// 目标点
        /// </summary>
        real3       targetPoint;

    public:
        /// <summary>
        /// 调用该函数，返回约束结果
        /// </summary>
        /// <param name="currentWorld"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& currentWorld,const real& time) const
        {
            (void)time;
            FETransform   desired     =   currentWorld;
            desired.setPosition(targetPoint + _offset.position());
            return                  desired;
        }

        /// <summary>
        /// 创建/激活约束时调用，根据当前位姿和目标点记录平移偏移
        /// 这样 targetPoint 改变前 solve() 不会改变对象位置
        /// </summary>
        /// <param name="currentWorld"></param>
        inline  void    recordOffset(const FETransform& currentWorld)
        {
            _offset.setPosition(currentWorld.position() - targetPoint);
        }
    };
}

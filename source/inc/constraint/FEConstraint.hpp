#pragma     once

#include    "../FEContext.hpp"
#include    "../FEDelegate.hpp"
#include    "../FEComponent.hpp"
#include    "../FETransform.hpp"

namespace FE
{

    class   FEConstraint : public FEComponent
    {
    public:
        /// <summary>
        /// 调用该函数，返回约束结果
        /// </summary>
        /// <param name="cur"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& cur,const real& time) const = 0;

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
}


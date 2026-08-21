#pragma     once

#include    "FEFactoryTemplate.hpp"
#include    "FEComponentSys.hpp"

namespace   FE
{
    /// lambda 表达式作为类型别名，确保RFactoryLess是类型名
    constexpr auto    ComSysLessLambda = [](const ComponentSys& l, const ComponentSys& r)
    {
        return  l->classId() < r->classId();
    };
    /// 先定义类型别名，确保RFactoryLess是类型名
    using   ComSysLessFunc    =   decltype(ComSysLessLambda);

    class   FEComponentSysMgr :public FEFactoryTemplate<ComponentSys, ComSysLessFunc>
    {
    public:
        FEComponentSysMgr(FEContext& ctx)
            :FEFactoryTemplate(ctx,ComSysLessLambda)
        {}
        FEComponentSysMgr(const FEComponentSysMgr& other)
            :FEFactoryTemplate(other)
        {}
        /// <summary>
        /// 根据类型Id查询
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        inline  auto    query(const FEUuid& classId) const
        {
            auto        itr =   std::lower_bound(_objects.begin(), _objects.end(), classId,[](const ComponentSys& l,const FEUuid& classId)
            {
                return  l->classId() < classId;
            });
            if (itr != _objects.end() && (*itr)->classId() == classId)
                return  (*itr);
            else
                return  ComponentSys();
        }
    };
}


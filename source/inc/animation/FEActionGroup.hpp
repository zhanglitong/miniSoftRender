#pragma     once
#include    "FEAction.hpp"
namespace FE
{
    /// lambda 表达式作为类型别名，确保比较方法
    constexpr   auto    ActionLessFunc = [](const Action& l, const Action& r)
    {
        return  l < r ;
    };
    /// 定义类型别名
    using   ActionLess  =   decltype(ActionLessFunc);

    DEFINE_CLASS_UUID(FEActionGroup, "{536BDD3A-F2BD-43DA-B5AB-AA7E6EC7F9C3}");

    class   FEActionGroup 
        : public FEObject
        , public FEObjectsTemplate<Action, ActionLess>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEActionGroup)
    public:
        FEActionGroup(FEContext& ctx)  
            :FEObject(ctx)
            ,FEObjectsTemplate<Action, ActionLess>(ActionLessFunc)
        {}
        FEActionGroup(const FEActionGroup& other)
            :FEObject(other)
            ,FEObjectsTemplate<Action, ActionLess>(other)
        {}
        ~FEActionGroup()   =   default;
    public:
        /// <summary>
        /// 更新所有action
        /// </summary>
        /// <param name="frame"></param>
        void    update(const real& tmDelta)
        {
            for (auto& var : _objects)
            {
                var->update(tmDelta);
            }
        }
    };
    using   ActionGroup   =   SharedPtr<FEActionGroup>;
}

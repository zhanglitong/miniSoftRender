#pragma     once
#include    "FEAction.hpp"
namespace FE
{
    /// lambda 琛ㄨ揪寮忎綔涓虹被鍨嬪埆鍚嶏紝纭繚姣旇緝鏂规硶
    constexpr   auto    ActionLessFunc = [](const Action& l, const Action& r)
    {
        return  l < r ;
    };
    /// 瀹氫箟绫诲瀷鍒悕
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
        /// 鏇存柊鎵€鏈塧ction
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

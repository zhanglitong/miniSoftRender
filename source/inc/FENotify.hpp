#pragma     once

#include    "FEObject.h"

namespace   FE
{
    
    /// <summary>
    /// 标记状态发生变化通知
    /// </summary>
    using   NFlagChange     =   std::function<void(Object sender)>;
    using   NFlagChangeMap  =   std::map<void*,NFlagChange>;

    DEFINE_CLASS_UUID(FENotify,"{AD26120E-AF65-4426-BC73-9047217D55EE}");

    class   FENotify :public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FENotify)
    public:
        FENotify(FEContext& ctx)
            :FEObject(ctx)
        {}
        FENotify(const FENotify& other)
            :FEObject(other)
        {}
        void    addFlagChangedNotify(void* pKey,const NFlagChange& notify)
        {
            _NFlagChanges[pKey] =   notify;
        }
        void    removeFlagChangedNotify(void* pKey,const NFlagChange& notify)
        {

            (void)pKey;
            (void)notify;
            _NFlagChanges.erase(pKey);
        }

        void    fireNotify()
        {
            onFlagChanged(this);
        }
    protected:
        void    onFlagChanged(Object sender) const
        {
            for (auto& var : _NFlagChanges)
            {
                if (!var.second)    
                    continue;
                var.second(sender);
            }
        }
    public:
        NFlagChangeMap  _NFlagChanges;
    };

    using   Notify      =   SharedPtr<FENotify>;
   
}


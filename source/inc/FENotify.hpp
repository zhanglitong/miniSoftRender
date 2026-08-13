#pragma     once

#include    "FEObject.h"

namespace   FE
{
    
    /// <summary>
    /// 标记状态发生变化通知
    /// </summary>
    using   NChange     =   std::function<void(Object sender)>;
    using   NChangeMap  =   std::map<void*,NChange>;

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
        {
            _NChanges   =   other._NChanges;
        }
        void    addNotify(void* pKey,const NChange& notify)
        {
            _NChanges[pKey] =   notify;
        }
        void    removeNotify(void* pKey,const NChange& notify)
        {

            (void)pKey;
            (void)notify;
            _NChanges.erase(pKey);
        }
        void    fireNotify()
        {
            onChanged(this);
        }
    protected:
        void    onChanged(Object sender) const
        {
            for (auto& var : _NChanges)
            {
                if (!var.second)    
                    continue;
                var.second(sender);
            }
        }
    public:
        NChangeMap  _NChanges;
    };

    using   Notify      =   SharedPtr<FENotify>;
   
}


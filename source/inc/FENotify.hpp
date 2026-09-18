#pragma     once

#include    "FEObject.h"
#include    "FEDelegate.hpp"

namespace   FE
{
    
    /// <summary>
    /// 标记状态发生变化通知
    /// </summary>
    using   NChanges    =   FETMultiDelegate<void(Object sender)>;

    DEFINE_CLASS_UUID(FENotify,"{AD26120E-AF65-4426-BC73-9047217D55EE}");

    class   FE_API  FENotify :public FEObject
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
        inline  NChanges&   notify() 
        {
            return  _NChanges;
        }
        const   NChanges&   notify() const
        {
            return  _NChanges;
        }
        void    fireNotify()
        {
            onChanged(this);
        }
    protected:
        void    onChanged(Object sender) const
        {
            _NChanges(sender);
        }
    public:
        NChanges    _NChanges;
    };

    using   Notify      =   SharedPtr<FENotify>;
   
}


#pragma     once
#include    "FEObject.h"
#include    "FENotify.hpp"
#include    "FEPickup.hpp"

namespace   FE
{
    class   FEAnchor : public FENotify
    {
    public:
        FEAnchor(FEContext& ctx)
            :FENotify(ctx)
        {}
        FEAnchor(const FEAnchor& other)
            :FENotify(other)
        {
            _pickup =   other._pickup;
        }
        real3       point() const
        {
            return  _pickup.point;
        }
        Object      object() const
        {
            return  _pickup.object;
        }
        void        setPickup(const FEPickup& pick)
        {
            _pickup =   pick;
        }
    protected:
        FEPickup    _pickup;
    };

    using   Anchor  =   SharedPtr<FEAnchor>;
}

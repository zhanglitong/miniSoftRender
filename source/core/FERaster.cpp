#include    "FERaster.h"

namespace   FE
{
    FERaster::FERaster(FEContext& ctx)
        :FEObject(ctx)
    {}

    FERaster::FERaster(const FERaster& other)
        :FEObject(other)
    {}

}
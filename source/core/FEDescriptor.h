#pragma     once

#include    "../inc/FEObject.h"
#include    "FEStruct.h"

namespace   FE
{
    class   FE_API  FEDescriptor :public FEObject
    {
    public:
        FEDescriptor();
    protected:
        FEBinding   _binds;
    };

    using   DSPtr   =   SharedPtr<FEDescriptor>;
}
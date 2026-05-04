#pragma     once

#include    "FEDefine.h"

namespace   FE
{
    class   FE_API  FEObject :public std::enable_shared_from_this<FEObject>
    {
    public:
    };

    using   Object  =   SharedPtr<FEObject>;
    using   Objects =   std::vector<Object>;
}


#pragma     once
#include    <string>
#include    "../inc/FEObject.h"
#include    "../inc/FERect.hpp"
#include    "FEStruct.h"

namespace   FE
{

    class   FEImage:public FEObject
    {
    public:
        bool    create(const FEImageCreateInfo& info);
    protected:
        FEImageCreateInfo   _cInfo;
    };
}
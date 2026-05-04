
#include    "FEImage.h"

namespace   FE
{

    bool    FEImage::create(const FEImageCreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    };
}
#include    "SWDSetLayout.h"
#include    "SWDevice.h"
#include    "SWDSet.h"

namespace   FE
{
    SWDSetLayout::~SWDSetLayout()
    {
    }

    bool    SWDSetLayout::create(const FEDSetLayout::CreateInfo& cInf)
    {
        _cInfo  =   cInf;
        return  true;
    }

    DSet    SWDSetLayout::createDSet()
    {
        return  {};
    }
}

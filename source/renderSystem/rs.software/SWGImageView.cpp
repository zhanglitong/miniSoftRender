#include    "SWGImage.h"
#include    "SWGImageView.h"
#include    "SWDevice.h"

namespace   FE
{
    SWGImageView::~SWGImageView()
    {
    }

    bool    SWGImageView::create(const FEGImageView::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }
}

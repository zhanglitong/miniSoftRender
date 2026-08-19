#include    "SWGImage.h"
#include    "SWDevice.h"
#include    "SWGImageView.h"

namespace   FE
{
    SWGImage::~SWGImage()
    {
    }

    bool    SWGImage::create(const FEGImage::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    GImgView    SWGImage::createView()
    {
        GImgView                    view    =   new SWGImageView(_ctx);
        SWGImageView::CreateInfo    info    =   {};
        info._image         =   this;
        if (view->create(info))
            return  view;
        else
            return  nullptr;
    }
}

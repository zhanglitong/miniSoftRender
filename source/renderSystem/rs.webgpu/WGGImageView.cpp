#include    "WGGImageView.h"
#include    "WGGImage.h"
#include    "WGDevice.h"

namespace   FE
{
    WGGImageView::~WGGImageView()
    {
        if (_native)
        {
            wgpuTextureViewRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGGImageView::create(const CreateInfo& info)
    {
        if (!info._image)
            return false;

        auto* wgImage = const_cast<WGGImage*>(static_cast<const WGGImage*>(info._image.get()));
        if (!wgImage)
            return false;

        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.nextInChain        =   nullptr;
        viewDesc.format             =   wgDevice.getWGPUTextureFormat(wgImage->cInfo()._format);
        viewDesc.dimension          =   WGPUTextureViewDimension_2D;
        viewDesc.aspect             =   WGPUTextureAspect_All;
        viewDesc.baseMipLevel       =   0;
        viewDesc.mipLevelCount      =   1;
        viewDesc.baseArrayLayer     =   0;
        viewDesc.arrayLayerCount    =   1;

        _native =   wgpuTextureCreateView((WGPUTexture)wgImage->native(),&viewDesc);
        return _native != nullptr;
    }
}

#include    "WGGImage.h"
#include    "WGDevice.h"
#include    "WGGImageView.h"

namespace   FE
{
    WGGImage::~WGGImage()
    {
        if (_native && !_ref)
        {
            wgpuTextureDestroy(_native);
            _native =   nullptr;
        }
    }

    bool WGGImage::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        WGPUTextureDescriptor textureDesc = {};
        textureDesc.nextInChain =   nullptr;
        textureDesc.size = { info._width,info._height,info._depth };
        textureDesc.mipLevelCount =   info._mips;
        textureDesc.sampleCount =   1;
        textureDesc.dimension =   WGPUTextureDimension_2D;
        textureDesc.format =   wgDevice.getWGPUTextureFormat(info._format);
        textureDesc.usage =   WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment;
        textureDesc.viewFormatCount =   0;
        textureDesc.viewFormats =   nullptr;

        _native =   wgpuDeviceCreateTexture(wgDevice.device(),&textureDesc);
        return _native != nullptr;
    }

    GImgView WGGImage::createView()
    {
        auto view = new WGGImageView(_ctx);
        FEGImageView::CreateInfo info;
        info._image =   this;
        if (!view->create(info))
        {
            delete view;
            return nullptr;
        }
        return view;
    }
}
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

        WGPUTextureFormat wgpuFormat = system2Native(info._format);
        if (wgpuFormat == WGPUTextureFormat_Undefined)
        {
            wgpuFormat = wgDevice.getWGPUTextureFormat(info._format);
        }

        if (wgpuFormat == WGPUTextureFormat_Undefined)
        {
            LOG_ERR("WGGImage::create: unsupported format %d", (int)info._format);
            return false;
        }

        /// depthOrArrayLayers:2D 数组/立方体由 _layer 表示(Vulkan 后端同样以 _layer 表示,
        /// 立方体贴图为 6);_depth > 1 时沿用旧的 depth 语义(WebGPU 当前只走 2D 维度)
        const uint32_t  layers  =   info._depth > 1 ? info._depth : (info._layer > 0 ? info._layer : 1);

        WGPUTextureDescriptor textureDesc = {};
        textureDesc.nextInChain     =   nullptr;
        textureDesc.size            =   { info._width,info._height,layers };
        textureDesc.mipLevelCount   =   info._mips > 0 ? info._mips : 1;
        textureDesc.sampleCount     =   1;
        textureDesc.dimension       =   WGPUTextureDimension_2D;
        textureDesc.format          =   wgpuFormat;
        textureDesc.usage           =   WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment;
        textureDesc.viewFormatCount =   0;
        textureDesc.viewFormats     =   nullptr;
        _native                     =   wgpuDeviceCreateTexture(wgDevice.device(),&textureDesc);
        if (!_native)
        {
            LOG_ERR("WGGImage::create: wgpuDeviceCreateTexture failed (format=%d, size=%ux%ux%u)",
                (int)wgpuFormat, info._width, info._height, info._depth);
            return false;
        }
        return true;
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
    GImgView WGGImage::createView(uint baseMip,uint levelCount)
    {
        auto view = new WGGImageView(_ctx);
        FEGImageView::CreateInfo info;
        info._image         =   this;
        info._baseMip       =   baseMip;
        info._levelCount    =   levelCount;
        if (!view->create(info))
        {
            delete view;
            return nullptr;
        }
        return view;
    }
}

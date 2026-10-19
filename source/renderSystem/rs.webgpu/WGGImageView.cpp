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

        const auto& imgInfo =   wgImage->cInfo();
        /// 视图格式必须与纹理格式一致:与 WGGImage::create 使用同一解析顺序
        /// (优先 system2Native,回退设备格式表)
        WGPUTextureFormat   viewFormat  =   system2Native(imgInfo._format);
        if (viewFormat == WGPUTextureFormat_Undefined)
            viewFormat  =   wgDevice.getWGPUTextureFormat(imgInfo._format);

        /// _layer == 6 视为立方体贴图(与 Vulkan 后端 VKGImageView 约定一致);
        /// 视图暴露全部 mip 与面,立方体贴图需要 arrayLayerCount=6 且 dimension=Cube
        const bool      bCube   =   (imgInfo._layer == 6);
        const uint32_t  mips    =   imgInfo._mips > 0 ? imgInfo._mips : 1;

        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.nextInChain        =   nullptr;
        viewDesc.format             =   viewFormat;
        viewDesc.dimension          =   bCube ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;
        viewDesc.aspect             =   WGPUTextureAspect_All;
        viewDesc.baseMipLevel       =   info._baseMip;
        viewDesc.mipLevelCount      =   info._levelCount > 0
                                            ? info._levelCount
                                            : mips;
        viewDesc.baseArrayLayer     =   0;
        viewDesc.arrayLayerCount    =   bCube ? 6 : 1;

        _native =   wgpuTextureCreateView((WGPUTexture)wgImage->native(),&viewDesc);
        return _native != nullptr;
    }
}

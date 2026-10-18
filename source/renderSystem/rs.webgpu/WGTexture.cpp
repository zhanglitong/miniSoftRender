#include    "WGTexture.h"
#include    "WGDevice.h"
#include    "FEFormatHelper.hpp"
#include    <vector>
#include    <cstring>

namespace   FE
{
    WGTexture::~WGTexture()
    {
        if (_sampler)
        {
            wgpuSamplerRelease(_sampler);
            _sampler    =   nullptr;
        }
        _view       =   nullptr;
        _image      =   nullptr;
        _native     =   nullptr;
    }

    bool    WGTexture::create(const FETexture::CreateInfo& info)
    {
        _cInfo  =   info;
        auto&   wgDevice    =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        /// 1. 创建图像(传输目标 + 采样)
        _image  =   new WGGImage(_ctx);
        FEGImage::CreateInfo    imgInfo     =   {};
        imgInfo._format                     =   info._format;
        imgInfo._type                       =   IT_2D;
        imgInfo._layout                     =   IL_UNDEFINED;
        imgInfo._usage                      =   USAGE_TRANSFER_DST | USAGE_SAMPLED;
        imgInfo._width                      =   info._width;
        imgInfo._height                     =   info._height;
        imgInfo._depth                      =   1;
        imgInfo._mips                       =   info._mips > 0 ? info._mips : 1;
        imgInfo._layer                      =   info._cube ? 6 : 1;
        if (!_image->create(imgInfo))
        {
            LOG_ERR("WGTexture.create: image create failed");
            return  false;
        }

        /// 2. 创建图像视图(立方体以 _layer==6 判定维度)
        _view   =   _image->createView();
        if (_view == nullptr)
        {
            LOG_ERR("WGTexture.create: view create failed");
            return  false;
        }
        _native =   (WGPUTextureView)_view->native();

        /// 3. 创建采样器(mip>1 时 mipmapFilter=Linear,maxLod=mips-1)
        const float maxLod  =   float(imgInfo._mips > 1 ? imgInfo._mips - 1 : 1);
        WGPUSamplerDescriptor    samplerDesc =   {};
        samplerDesc.nextInChain =   nullptr;
        samplerDesc.label       =   { nullptr,0 };
        samplerDesc.addressModeU=   system2Native(info._addressMode);
        samplerDesc.addressModeV=   system2Native(info._addressMode);
        samplerDesc.addressModeW=   system2Native(info._addressMode);
        samplerDesc.magFilter   =   system2Native(info._magFilter);
        samplerDesc.minFilter   =   system2Native(info._minFilter);
        samplerDesc.mipmapFilter=   imgInfo._mips > 1 ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
        samplerDesc.lodMinClamp =   0.0f;
        samplerDesc.lodMaxClamp =   maxLod;
        samplerDesc.compare     =   WGPUCompareFunction_Undefined;
        samplerDesc.maxAnisotropy=  1;
        _sampler    =   wgpuDeviceCreateSampler(wgDevice.device(),&samplerDesc);

        LOG_DBG("WGTexture.create %ux%u ok",info._width,info._height);
        return  _sampler != nullptr;
    }

    bool    WGTexture::update(const void* data,uint64 size)
    {
        if (_image == nullptr || _native == nullptr || data == nullptr || size == 0)
            return  false;

        auto&   wgDevice    =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        WGPUQueue   queue   =   wgDevice.queue();
        WGPUTexture texture =   (WGPUTexture)_image->native();
        if (queue == nullptr || texture == nullptr)
            return  false;

        const auto& imgInfo     =   _image->cInfo();
        const uint  mipLevels   =   imgInfo._mips > 0 ? imgInfo._mips : 1;
        const uint  layers      =   imgInfo._layer > 0 ? imgInfo._layer : 1;
        const uint  bpp         =   FEFormatHelper::sizeOf(_cInfo._format);

        /// 数据排布:面主序、面内 mip 顺序(与 FETexture::CreateInfo 约定一致),
        /// 每像素字节数由格式决定。逐 (层, mip) 通过 wgpuQueueWriteTexture 上传。
        const uint8*    src     =   static_cast<const uint8*>(data);
        uint64          offset  =   0;
        std::vector<uint8>  padded;

        for (uint layer = 0; layer < layers; ++layer)
        {
            uint    mipW    =   _cInfo._width;
            uint    mipH    =   _cInfo._height;
            for (uint mip = 0; mip < mipLevels; ++mip)
            {
                const uint32    rowByte     =   mipW * bpp;
                const uint32    rows        =   mipH;
                const uint64    levelBytes  =   uint64(rowByte) * rows;
                if (offset + levelBytes > size)
                {
                    LOG_ERR("WGTexture.update: data too small (need %llu, have %llu)",
                            (uint64)(offset + levelBytes),(uint64)size);
                    return  false;
                }

                /// WebGPU 要求 bytesPerRow 为 256 的倍数(仅在多于一行时)。
                /// 行字节不是 256 倍数且有多行时,按 256 对齐重新打包到临时缓冲。
                uint32          copyRowByte =   rowByte;
                const uint8*    levelData   =   src + offset;
                if (rows > 1 && (rowByte % 256) != 0)
                {
                    copyRowByte =   (rowByte + 255) / 256 * 256;
                    padded.resize(uint64(copyRowByte) * rows);
                    for (uint32 r = 0; r < rows; ++r)
                        std::memcpy(padded.data() + uint64(r) * copyRowByte,
                                    src + offset + uint64(r) * rowByte,rowByte);
                    levelData   =   padded.data();
                }

                WGPUTexelCopyTextureInfo    dst =   {};
                dst.texture     =   texture;
                dst.mipLevel    =   mip;
                dst.origin      =   { 0,0,layer };
                dst.aspect      =   WGPUTextureAspect_All;

                WGPUTexelCopyBufferLayout   layout =    {};
                layout.offset       =   0;
                layout.bytesPerRow  =   copyRowByte;
                layout.rowsPerImage =   rows;

                WGPUExtent3D    writeSize   =   { mipW,mipH,1 };
                wgpuQueueWriteTexture(queue,&dst,levelData,
                                      uint64(copyRowByte) * rows,&layout,&writeSize);

                offset  +=  levelBytes;
                mipW    =   mipW > 1 ? mipW >> 1 : 1;
                mipH    =   mipH > 1 ? mipH >> 1 : 1;
            }
        }

        LOG_DBG("WGTexture.update %llu bytes ok",(uint64)size);
        return  true;
    }
}

#include    <vector>

#include    "FETextureHelper.hpp"
#include    "../../inc/graphic/FEDevice.h"
#include    "../FEImageHelper.hpp"

namespace   FE
{
    TexPtr  FETextureHelper::create(FEContext& ctx,const FEImage& image,
                                    FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        const auto&     cInfo       =   image.cInfo();
        const uint32    width       =   cInfo._extent.x;
        const uint32    height      =   cInfo._extent.y;
        const uint32    layers      =   cInfo._layers ? cInfo._layers : 1;
        const uint32    levels      =   cInfo._levels ? cInfo._levels : 1;
        const uint32    pixelByte   =   FEFormatHelper::sizeOf(cInfo._format);
        /// 仅支持 2D(1层) 或 cubemap(6层,面为正方形)
        const bool      cube        =   (layers == 6);
        if  (width == 0 || height == 0 || pixelByte == 0)
            return  nullptr;
        if  ((!cube && layers != 1) || (cube && width != height))
            return  nullptr;

        /// 图像每行按 _align 对齐,GPU 需要紧凑行,这里逐行搬运
        std::vector<uint8>  pixels;
        for (uint32 l = 0; l < layers; ++l)
        {
            for (uint32 m = 0; m < levels; ++m)
            {
                const auto  plane   =   image.plane(m,l);
                if (plane._buffer == nullptr)
                    return  nullptr;
                const size_t    rowByte =   size_t(plane.width()) * pixelByte;
                for (uint32 y = 0; y < plane.height(); ++y)
                {
                    const uint8*    row =   plane.dataOffset<uint8>(y,0);
                    pixels.insert(pixels.end(),row,row + rowByte);
                }
            }
        }

        TexPtr  texture =   ctx.device().createTexture();
        if (texture == nullptr)
            return  nullptr;

        FETexture::CreateInfo   texInfo;
        texInfo._width      =   width;
        texInfo._height     =   height;
        texInfo._format     =   cInfo._format;
        texInfo._magFilter  =   magFilter;
        texInfo._minFilter  =   minFilter;
        texInfo._addressMode=   addressMode;
        texInfo._mips       =   levels;
        texInfo._cube       =   cube;
        if  (!texture->create(texInfo))
            return  nullptr;
        if  (!texture->update(pixels.data(),pixels.size()))
            return  nullptr;
        return  texture;
    }

    TexPtr  FETextureHelper::load(FEContext& ctx,const char* fileName,
                                  FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        ImagePtr    image   =   FEImageHelper::load(ctx,fileName);
        if (image == nullptr)
            return  nullptr;
        return  create(ctx,*image,magFilter,minFilter,addressMode);
    }
}

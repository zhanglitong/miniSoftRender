#include    <vector>
#include    <cmath>
#include    <algorithm>

#include    "FETextureHelper.hpp"
#include    "../../inc/graphic/FEDevice.h"
#include    "../../inc/FEConst.h"
#include    "../FEImageHelper.hpp"

namespace   FE
{
    namespace
    {
        /// cubemap 面内像素方向(面序 +X,-X,+Y,-Y,+Z,-Z,与 Vulkan/GL 规范一致)
        inline  float3  cubeFaceDir(int face,float a,float b)
        {
            switch (face)
            {
            case 0:  return float3( 1.0f, -b, -a);   /// +X
            case 1:  return float3(-1.0f, -b,  a);   /// -X
            case 2:  return float3( a,  1.0f,  b);   /// +Y
            case 3:  return float3( a, -1.0f, -b);   /// -Y
            case 4:  return float3( a, -b,  1.0f);   /// +Z
            default: return float3(-a, -b, -1.0f);   /// -Z
            }
        }
    }

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

    ImagePtr    FETextureHelper::cubeImageFromEquirect(FEContext& ctx,const FEImage& equirect,uint size)
    {
        const auto&     cInfo   =   equirect.cInfo();
        const uint32    eqW     =   cInfo._extent.x;
        const uint32    eqH     =   cInfo._extent.y;
        const uint32    channel =   FEFormatHelper::channelOf(cInfo._format);
        /// 输入必须是浮点格式(每通道 4 字节)且至少 3 通道
        const bool      bFloat  =   (FEFormatHelper::sizeOf(cInfo._format) == channel * uint32(sizeof(float)));
        if  (size == 0 || eqW == 0 || eqH == 0 || channel < 3 || !bFloat)
            return  nullptr;
        const auto      eqPlane =   equirect.plane(0,0);
        if  (eqPlane._buffer == nullptr)
            return  nullptr;

        /// equirect 采样(双线性,u 环绕,v 钳制;线性空间)
        /// 约定:图像顶部 = +Y(天顶),经度 phi = atan2(d.z, d.x)
        auto    sampleEq    =   [&](const float3& d) -> float3
        {
            const float phi     =   atan2f(d.z,d.x);                        /// [-PI, PI]
            const float theta   =   acosf(clamp(d.y,-1.0f,1.0f));           /// [0, PI]
            float       uf      =   (phi * (0.5f / float(PI)) + 0.5f) * float(eqW) - 0.5f;
            float       vf      =   theta * (1.0f / float(PI)) * float(eqH) - 0.5f;
            uf  =   uf - floorf(uf / float(eqW)) * float(eqW);              /// 环绕到 [0, w)
            vf  =   clamp(vf,0.0f,float(eqH - 1));                          /// 钳制到图内

            const int   x0  =   int(floorf(uf));
            const int   y0  =   int(floorf(vf));
            const int   x1  =   (x0 + 1) % int(eqW);
            const int   y1  =   y0 + 1 < int(eqH) ? y0 + 1 : int(eqH) - 1;
            const float tx  =   uf - floorf(uf);
            const float ty  =   vf - floorf(vf);

            const float*    row0    =   eqPlane.rowAt<float>(uint32(y0));
            const float*    row1    =   eqPlane.rowAt<float>(uint32(y1));
            const float*    p00     =   row0 + size_t(x0) * channel;
            const float*    p10     =   row0 + size_t(x1) * channel;
            const float*    p01     =   row1 + size_t(x0) * channel;
            const float*    p11     =   row1 + size_t(x1) * channel;
            float3          c;
            for (uint32 i = 0; i < 3; ++i)
            {
                const float top     =   p00[i] * (1.0f - tx) + p10[i] * tx;
                const float bottom  =   p01[i] * (1.0f - tx) + p11[i] * tx;
                c[i]    =   top * (1.0f - ty) + bottom * ty;
            }
            return  c;
        };

        uint    mipCount    =   1;
        for (uint s = size; s > 1; s >>= 1)
            ++mipCount;

        ImagePtr    image   =   new FEImage(ctx);
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(size,size,1);
        ci._format  =   FMT_R32G32B32A32_FLOAT;
        ci._layers  =   6;
        ci._levels  =   uint16(mipCount);
        ci._align   =   4;
        if  (!image->create(ci))
            return  nullptr;

        /// level0:逐面逐像素做 equirect -> cube 重投影
        for (uint face = 0; face < 6; ++face)
        {
            auto    plane   =   image->plane(0,face);
            for (uint y = 0; y < size; ++y)
            {
                for (uint x = 0; x < size; ++x)
                {
                    const float a   =   2.0f * (x + 0.5f) / float(size) - 1.0f;
                    const float b   =   2.0f * (y + 0.5f) / float(size) - 1.0f;
                    const float3    c = sampleEq(normalize(cubeFaceDir(int(face),a,b)));
                    float*  px  =   plane.dataOffset<float>(y,x);
                    px[0]   =   c.x;
                    px[1]   =   c.y;
                    px[2]   =   c.z;
                    px[3]   =   1.0f;
                }
            }
        }

        /// mip 链:2x2 盒式滤波(逐面独立,float 域平均保留 HDR 范围)
        for (uint m = 1; m < mipCount; ++m)
        {
            const uint  dstDim  =   (std::max<uint32>)(size >> m,1);
            for (uint face = 0; face < 6; ++face)
            {
                const auto  src =   image->plane(m - 1,face);
                auto        dst =   image->plane(m,face);
                for (uint y = 0; y < dstDim; ++y)
                {
                    for (uint x = 0; x < dstDim; ++x)
                    {
                        float   sum[3]  =   { 0.0f, 0.0f, 0.0f };
                        for (int dy = 0; dy < 2; ++dy)
                        {
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                const float*    px  =   src.dataOffset<float>(y * 2 + dy,x * 2 + dx);
                                sum[0] += px[0]; sum[1] += px[1]; sum[2] += px[2];
                            }
                        }
                        float*  px  =   dst.dataOffset<float>(y,x);
                        px[0]   =   sum[0] * 0.25f;
                        px[1]   =   sum[1] * 0.25f;
                        px[2]   =   sum[2] * 0.25f;
                        px[3]   =   1.0f;
                    }
                }
            }
        }
        return  image;
    }

    TexPtr  FETextureHelper::createCube(FEContext& ctx,const FEImage& equirect,uint size,
                                        FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        ImagePtr    cube    =   cubeImageFromEquirect(ctx,equirect,size);
        if (cube == nullptr)
            return  nullptr;
        return  create(ctx,*cube,magFilter,minFilter,addressMode);
    }
}

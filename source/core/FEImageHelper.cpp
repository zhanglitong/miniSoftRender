#include    <cctype>
#include    <cstring>
#include    "FEImageHelper.hpp"
#include    "../inc/FEContext.hpp"

/// stb_image_write 的实现体:全工程仅此一处
#define     STB_IMAGE_WRITE_IMPLEMENTATION
#include    "../inc/fileFormat/gltf/stb_image_write.h"
/// stb_image 仅取声明:实现体在 FEContext.cpp(包含 glTF 解析器时定义了 STB_IMAGE_IMPLEMENTATION)
#include    "../inc/fileFormat/gltf/stb_image.h"

namespace   FE
{
    namespace
    {
        /// 显式目标格式 -> 解码所需通道数;0 表示不支持
        /// 支持三类逐通道格式(1/2/3/4 通道):
        ///   8bit  UNORM : R8  / R8G8  / R8G8B8  / R8G8B8A8
        ///   16bit half  : R16 / R16G16 / R16G16B16 / R16G16B16A16_FLOAT
        ///   32bit float : R32 / R32G32 / R32G32B32 / R32G32B32A32_FLOAT
        inline  int     loadChannelCount(FEFormat fmt)
        {
            switch (fmt)
            {
            case FMT_R8_UNORM:
            case FMT_R16_FLOAT:
            case FMT_R32_FLOAT:                 return  1;
            case FMT_R8G8_UNORM:
            case FMT_R16G16_FLOAT:
            case FMT_R32G32_FLOAT:              return  2;
            case FMT_R8G8B8_UNORM:
            case FMT_R16G16B16_FLOAT:
            case FMT_R32G32B32_FLOAT:           return  3;
            case FMT_R8G8B8A8_UNORM:
            case FMT_R16G16B16A16_FLOAT:
            case FMT_R32G32B32A32_FLOAT:        return  4;
            default:                            return  0;
            }
        }
        /// 是否每通道4字节单精度浮点格式
        inline  bool    isFloatFormat(FEFormat fmt)
        {
            switch (fmt)
            {
            case FMT_R32_FLOAT:
            case FMT_R32G32_FLOAT:
            case FMT_R32G32B32_FLOAT:
            case FMT_R32G32B32A32_FLOAT:        return  true;
            default:                            return  false;
            }
        }
        /// 是否每通道2字节半精度浮点格式(stb 无 half 输出,由 float 转换写入)
        inline  bool    isHalfFormat(FEFormat fmt)
        {
            switch (fmt)
            {
            case FMT_R16_FLOAT:
            case FMT_R16G16_FLOAT:
            case FMT_R16G16B16_FLOAT:
            case FMT_R16G16B16A16_FLOAT:        return  true;
            default:                            return  false;
            }
        }
        /// 是否浮点系(半点 / 单精度),决定走 stbi_loadf 还是 stbi_load
        inline  bool    isFloatFamily(FEFormat fmt)
        {
            return  isFloatFormat(fmt) || isHalfFormat(fmt);
        }
    }

    bool    FEImageHelper::saveBmp(FEImage& image,const char* fileName)
    {
        return  FEBmpWriter::save(fileName,image);
    }

    ImagePtr    FEImageHelper::load(FEContext& ctx,const char* fileName,FEFormat fmt)
    {
        if  (fileName == nullptr)
            return  nullptr;
        /// 未指定格式 -> 按文件类型自动:HDR 用 RGBA float,其余用 RGBA8
        if  (fmt == FMT_NONE)
            fmt =   (stbi_is_hdr(fileName) != 0) ? FMT_R32G32B32A32_FLOAT : FMT_R8G8B8A8_UNORM;

        const int   channel =   loadChannelCount(fmt);
        if  (channel == 0)
            return  nullptr;

        int     width   =   0;
        int     height  =   0;
        /// 浮点系(半点/单精度)走 stbi_loadf,8bit 走 stbi_load;缺省通道由 stb 补齐
        if  (isFloatFamily(fmt))
        {
            float*  pixels  =   stbi_loadf(fileName,&width,&height,nullptr,channel);
            if  (pixels == nullptr || width <= 0 || height <= 0)
                return  nullptr;
            ImagePtr    image   =   createImage(ctx,uint32(width),uint32(height),fmt);
            if  (image != nullptr)
            {
                const bool      bHalf   =   isHalfFormat(fmt);
                auto            plane   =   image->plane(0,0);
                const size_t    count   =   size_t(width) * size_t(channel);
                for (uint32 y = 0; y < uint32(height); ++y)
                {
                    const float*    src =   pixels + size_t(y) * count;
                    uint8*          dst =   plane.dataOffset<uint8>(y,0);
                    if  (bHalf)
                    {
                        /// stb 无 half 输出,统一从 float 转半精度(保持 IEEE-754 语义)
                        uint16*     h   =   reinterpret_cast<uint16*>(dst);
                        for (size_t i = 0; i < count; ++i)
                            h[i]    =   float16::float2Half(src[i]);
                    }
                    else
                        std::memcpy(dst,src,count * sizeof(float));
                }
            }
            stbi_image_free(pixels);
            return  image;
        }

        uint8*  pixels  =   stbi_load(fileName,&width,&height,nullptr,channel);
        if  (pixels == nullptr || width <= 0 || height <= 0)
            return  nullptr;
        ImagePtr    image   =   createImage(ctx,uint32(width),uint32(height),fmt);
        if  (image != nullptr)
        {
            auto            plane   =   image->plane(0,0);
            const uint32    rowByte =   uint32(width) * uint32(channel);
            for (uint32 y = 0; y < uint32(height); ++y)
                std::memcpy(plane.dataOffset<uint8>(y,0),pixels + size_t(y) * rowByte,rowByte);
        }
        stbi_image_free(pixels);
        return  image;
    }

    ImagePtr    FEImageHelper::loadHdr(FEContext& ctx,const char* fileName,FEFormat fmt)
    {
        /// 只接受浮点系格式(半点 / 单精度);未指定时默认 RGBA float
        if  (fmt != FMT_NONE && !isFloatFamily(fmt))
            return  nullptr;
        if  (fmt == FMT_NONE)
            fmt =   FMT_R32G32B32A32_FLOAT;
        return  load(ctx,fileName,fmt);
    }

    bool    FEImageHelper::save(const FEImage& image,const char* fileName,int quality)
    {
        const auto&     cInfo       =   image.cInfo();
        const uint32    channel     =   FEFormatHelper::channelOf(cInfo._format);
        const uint32    pixelByte   =   FEFormatHelper::sizeOf(cInfo._format);
        /// 只支持每通道 8bit 的数据
        if (channel == 0 || pixelByte != channel)
            return  false;
        const uint32    width   =   cInfo._extent.x;
        const uint32    height  =   cInfo._extent.y;
        if (width == 0 || height == 0)
            return  false;

        const auto  plane   =   image.plane(0,0);
        if (plane._buffer == nullptr)
            return  false;

        /// 重排为紧凑行(源数据行按 _align 对齐)
        const size_t        rowByte =   size_t(width) * pixelByte;
        std::vector<uint8>  tight(rowByte * height);
        for (uint32 y = 0; y < height; ++y)
            std::memcpy(tight.data() + size_t(y) * rowByte,plane.dataOffset<uint8>(y,0),rowByte);

        const int       w       =   int(width);
        const int       h       =   int(height);
        const int       c       =   int(channel);
        const String    ext     =   extensionOf(fileName);
        int             result  =   0;
        if  (ext == "png")
            result  =   stbi_write_png(fileName,w,h,c,tight.data(),int(rowByte));
        else if (ext == "bmp")
            result  =   stbi_write_bmp(fileName,w,h,c,tight.data());
        else if (ext == "tga")
            result  =   stbi_write_tga(fileName,w,h,c,tight.data());
        else if (ext == "jpg" || ext == "jpeg")
            result  =   stbi_write_jpg(fileName,w,h,c,tight.data(),quality);
        return  result != 0;
    }

    ImagePtr    FEImageHelper::createImage(FEContext& ctx,uint32 width,uint32 height,FEFormat fmt)
    {
        ImagePtr    image   =   new FEImage(ctx,width,height,fmt);
        if (image->size() == 0)
            return  nullptr;
        return  image;
    }

    String  FEImageHelper::extensionOf(const char* fileName)
    {
        if (fileName == nullptr)
            return  String();
        const String    name(fileName);
        const size_t    pos =   name.find_last_of('.');
        if (pos == String::npos)
            return  String();
        String  ext =   name.substr(pos + 1);
        for (auto& ch : ext)
            ch  =   (char)tolower(uint8(ch));
        return  ext;
    }
}

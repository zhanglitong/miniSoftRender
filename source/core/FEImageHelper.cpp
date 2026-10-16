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
    bool    FEImageHelper::saveBmp(FEImage& image,const char* fileName)
    {
        return  FEBmpWriter::save(fileName,image);
    }

    ImagePtr    FEImageHelper::load(FEContext& ctx,const char* fileName)
    {
        if (fileName == nullptr)
            return  nullptr;
        int     width   =   0;
        int     height  =   0;
        /// 第4个参数传4:强制解码为 RGBA,缺失通道由 stb 补齐(alpha=255)
        uint8*  pixels  =   stbi_load(fileName,&width,&height,nullptr,4);
        if (pixels == nullptr || width <= 0 || height <= 0)
            return  nullptr;

        ImagePtr    image   =   createImage(ctx,uint32(width),uint32(height),FMT_R8G8B8A8_UNORM);
        if (image != nullptr)
        {
            auto            plane   =   image->plane(0,0);
            const uint32    rowByte =   uint32(width) * 4;
            for (uint32 y = 0; y < uint32(height); ++y)
                std::memcpy(plane.dataOffset<uint8>(y,0),pixels + size_t(y) * rowByte,rowByte);
        }
        stbi_image_free(pixels);
        return  image;
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

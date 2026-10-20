#include    <cctype>
#include    <cstring>
#include    "FEImageHelper.hpp"
#include    "../inc/FEContext.hpp"
/// libktx(external/ktx,KTX-Software v4.4.0,静态库链接进 core)
#include    <ktx.h>

/// stb_image_write 的实现体:全工程仅此一处
#define     STB_IMAGE_WRITE_IMPLEMENTATION
#include    "../inc/fileFormat/gltf/stb_image_write.h"
/// stb_image 仅取声明:实现体在 FEContext.cpp(包含 glTF 解析器时定义了 STB_IMAGE_IMPLEMENTATION)
#include    "../inc/fileFormat/gltf/stb_image.h"

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 显式目标格式 -> 解码所需通道数;0 表示不支持
        /// 支持三类逐通道格式(1/2/3/4 通道):
        ///   8bit  UNORM : R8  / R8G8  / R8G8B8  / R8G8B8A8
        ///   16bit half  : R16 / R16G16 / R16G16B16 / R16G16B16A16_FLOAT
        ///   32bit float : R32 / R32G32 / R32G32B32 / R32G32B32A32_FLOAT
        /// </summary>
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
        /// <summary>
        /// 是否每通道4字节单精度浮点格式
        /// </summary>
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
        /// <summary>
        /// 是否每通道2字节半精度浮点格式(stb 无 half 输出,由 float 转换写入)
        /// </summary>
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
        /// <summary>
        /// 是否浮点系(半点 / 单精度),决定走 stbi_loadf 还是 stbi_load
        /// </summary>
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
        /// KTX/KTX2 走 libktx 专用路径(stb 不支持容器格式)
        {
            const String    ext =   extensionOf(fileName);
            if  (ext == "ktx" || ext == "ktx2")
                return  loadKtx(ctx,fileName,fmt);
        }
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

    ImagePtr    FEImageHelper::loadKtx(FEContext& ctx,const char* fileName,FEFormat fmt)
    {
        if  (fileName == nullptr)
            return  nullptr;
        /// 目标格式固定 RGBA8(压缩纹理统一 transcode 到 RGBA32)
        if  (fmt != FMT_NONE && fmt != FMT_R8G8B8A8_UNORM)
            return  nullptr;

        ktxTexture* ktex    =   nullptr;
        if  (ktxTexture_CreateFromNamedFile(fileName,KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,&ktex) != KTX_SUCCESS
             || ktex == nullptr)
            return  nullptr;

        ImagePtr    result  =   nullptr;
        do
        {
            /// BasisU/ETC1S/UASTC → transcode 为 RGBA32(仅 ktx2;ktx1 压缩格式
            /// 无通用解码路径,落入下方格式校验返回失败)
            if  (ktxTexture_NeedsTranscoding(ktex))
            {
                if  (ktex->classId != ktxTexture2_c
                     || ktxTexture2_TranscodeBasis(reinterpret_cast<ktxTexture2*>(ktex),KTX_TTF_RGBA32,0) != KTX_SUCCESS)
                    break;
            }
            /// 仅接受 RGBA8 未压缩(或 transcode 后的 RGBA32)
            /// VK_FORMAT_R8G8B8A8_UNORM=37 / SRGB=43;GL_RGBA8=0x8058 / GL_SRGB8_ALPHA8=0x8C43
            bool    isRgba8 =   false;
            if  (ktex->classId == ktxTexture2_c)
            {
                const uint32    vf  =   reinterpret_cast<ktxTexture2*>(ktex)->vkFormat;
                isRgba8 =   (vf == 37u || vf == 43u);
            }
            else
            {
                const uint32    gl  =   reinterpret_cast<ktxTexture1*>(ktex)->glInternalformat;
                isRgba8 =   (gl == 0x8058u || gl == 0x8C43u);
            }
            if  (!isRgba8 || ktex->baseWidth == 0 || ktex->baseHeight == 0)
                break;

            /// 支持 2D(单层) 与 cubemap(6 面,KTX/KTX2 均由 isCubemap 标记);
            /// 每面独立按 level 读取,面序与 Vulkan/GL 规范一致(+X,-X,+Y,-Y,+Z,-Z)
            const uint32    faces   =   (ktex->isCubemap != 0) ? 6u : 1u;

            /// 创建带完整 mip 链的目标图像
            FEImage::CreateInfo ci;
            ci._extent  =   uint3(ktex->baseWidth,ktex->baseHeight,1);
            ci._format  =   FMT_R8G8B8A8_UNORM;
            ci._layers  =   uint16(faces);
            ci._levels  =   uint16(ktex->numLevels > 0 ? ktex->numLevels : 1);
            ImagePtr    image   =   new FEImage(ctx);
            if  (!image->create(ci) || image->size() == 0)
                break;

            /// 行序:libktx 的 ktxTexture_SetImageFromMemory 原样存储调用方字节,
            /// orientation 元数据仅作提示——写入端给什么顺序,读出来就是什么顺序。
            /// 我们的测试写入用顶行在前(引擎约定),这里也原样拷贝不翻转。
            bool    ok  =   true;
            for (uint32 face = 0; face < faces && ok; ++face)
            {
                for (uint32 level = 0; level < ci._levels; ++level)
                {
                    ktx_size_t  offset  =   0;
                    /// 参数序:(level, layer, faceSlice);cubemap 时 numLayers=1,numFaces=6
                    if  (ktxTexture_GetImageOffset(ktex,level,0,face,&offset) != KTX_SUCCESS)
                    {
                        ok  =   false;
                        break;
                    }
                    const uint32    w       =   (std::max<uint32>)(ktex->baseWidth >> level,1);
                    const uint32    h       =   (std::max<uint32>)(ktex->baseHeight >> level,1);
                    const uint8*    srcBase =   ktex->pData + offset;
                    const size_t    srcPitch=   ktxTexture_GetRowPitch(ktex,level);
                    auto            plane   =   image->plane(level,face);
                    if  (srcPitch < size_t(w) * 4 || plane._buffer == nullptr)
                    {
                        ok  =   false;
                        break;
                    }
                    for (uint32 y = 0; y < h; ++y)
                        std::memcpy(plane.dataOffset<uint8>(y,0),srcBase + size_t(y) * srcPitch,size_t(w) * 4);
                }
            }
            if  (!ok)
                break;
            result  =   image;
        } while (false);

        ktxTexture_Destroy(ktex);
        return  result;
    }

    /// <summary>
    /// =====================================================================
    /// saveKtxInternal:FEImage → libktx → ktx 或 ktx2 文件
    /// 支持 1/2/3/4 通道 8bit 格式 + mip 链逐层写入;
    /// ktx2 + quality>0 时经 ktxTexture2_CompressBasis 转 ETC1S
    /// =====================================================================
    /// </summary>
    static  bool    saveKtxInternal(const FEImage& image,const char* fileName,bool isKtx2,int quality)
    {
        const auto&     cInfo   =   image.cInfo();
        const uint32    channel =   FEFormatHelper::channelOf(cInfo._format);
        const uint32    levels  =   cInfo._levels > 0 ? cInfo._levels : 1;
        const uint32    W       =   cInfo._extent.x;
        const uint32    H       =   cInfo._extent.y;

        /// 通道 → GL internalformat / baseInternalformat / format / type
        /// 和 Vulkan VkFormat(ktx2 用)
        uint32  glInternal  =   0;
        uint32  glBase      =   0;
        uint32  glFmt       =   0;
        uint32  vkFmt       =   0;
        switch (channel)
        {
        case 1:  glInternal = 0x8229; glBase = 0x1903; glFmt = 0x1903; vkFmt = 9;  break;  /// R8 / GL_R8 / GL_RED
        case 2:  glInternal = 0x822B; glBase = 0x8227; glFmt = 0x8227; vkFmt = 16; break;  /// RG8 / GL_RG8 / GL_RG
        case 3:  glInternal = 0x8051; glBase = 0x1907; glFmt = 0x1907; vkFmt = 23; break;  /// RGB8 / GL_RGB8 / GL_RGB
        case 4:  glInternal = 0x8058; glBase = 0x1908; glFmt = 0x1908; vkFmt = 37; break;  /// RGBA8 / GL_RGBA8 / GL_RGBA
        default:  return  false;
        }
        const uint32    glType =   0x1401;  /// GL_UNSIGNED_BYTE

        ktxTextureCreateInfo    ci  =   {};
        if  (isKtx2)
        {
            ci.vkFormat =   vkFmt;
            ci.glInternalformat =   0;
        }
        else
        {
            ci.glInternalformat =   glInternal;
            ci.vkFormat =   0;
        }
        ci.baseWidth        =   W;
        ci.baseHeight       =   H;
        ci.baseDepth        =   1;
        ci.numDimensions    =   2;
        ci.numLevels        =   levels;
        ci.numLayers        =   1;
        ci.numFaces         =   1;
        ci.isArray          =   KTX_FALSE;
        ci.generateMipmaps  =   KTX_FALSE;

        ktxTexture*     ktex   =   nullptr;
        if  (isKtx2)
        {
            ktxTexture2*    k2  =   nullptr;
            if  (ktxTexture2_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&k2) != KTX_SUCCESS)
                return  false;
            ktxTexture1*    as1 =   reinterpret_cast<ktxTexture1*>(k2);
            /// ktxTexture2 构造后 glType/glFormat 字段未初始化,
            /// SetImageFromMemory 内部会依赖 ktxTexture_vtbl::SetImageFromMemory,
            /// 对 ktxTexture2 只关心 vkFormat,无需设置 GL 字段
            ktex =   ktxTexture(k2);
        }
        else
        {
            ktxTexture1*    k1  =   nullptr;
            if  (ktxTexture1_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&k1) != KTX_SUCCESS)
                return  false;
            k1->glFormat            =   glFmt;
            k1->glBaseInternalformat=   glBase;
            k1->glType              =   glType;
            ktex =   ktxTexture(k1);
        }

        bool    ok  =   true;
        for (uint32 l = 0; ok && l < levels; ++l)
        {
            auto        plane   =   image.plane(l,0);
            const uint32    w   =   (std::max<uint32>)(W >> l,1);
            const uint32    h   =   (std::max<uint32>)(H >> l,1);
            if  (plane._buffer == nullptr || plane.width() != w || plane.height() != h)
            {
                ok  =   false;
                break;
            }
            const size_t    rowByte =   size_t(w) * channel;
            std::vector<uint8>  tight(rowByte * h);
            for (uint32 y = 0; y < h; ++y)
                std::memcpy(tight.data() + size_t(y) * rowByte,plane.dataOffset<uint8>(y,0),rowByte);
            ok  =   ktxTexture_SetImageFromMemory(ktex,l,0,0,tight.data(),tight.size()) == KTX_SUCCESS;
        }

        /// ktx2 + quality>0:尝试 ETC1S 压缩(quality 0~255,128=中等);
        /// 失败(如格式不支持 Basis)静默降级为 RGBA32 容器
        if  (ok && isKtx2 && quality > 0)
        {
            auto*   k2  =   reinterpret_cast<ktxTexture2*>(ktex);
            if  (ktxTexture2_CompressBasis(k2,ktx_uint32_t(quality)) != KTX_SUCCESS)
            {
                /// 压缩失败,放弃——直接写未压缩的 KTX2
            }
        }

        if  (ok)
            ok  =   ktxTexture_WriteToNamedFile(ktex,fileName) == KTX_SUCCESS;

        ktxTexture_Destroy(ktex);
        return  ok;
    }

    bool    FEImageHelper::save(const FEImage& image,const char* fileName,int quality)
    {
        if  (fileName == nullptr)
            return  false;
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

        const String    ext     =   extensionOf(fileName);

        /// KTX/KTX2:经 libktx 写容器
        if  (ext == "ktx" || ext == "ktx2")
            return  (ext == "ktx2") ? saveKtxInternal(image,fileName,true,quality)
                                    : saveKtxInternal(image,fileName,false,quality);

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

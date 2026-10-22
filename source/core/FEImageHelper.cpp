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
/// 块压缩解码器(BC/ETC2/EAC/ASTC,实现宏仅在本 TU 展开)
#include    "image/FEBlockDecoder.hpp"
/// 块压缩编码器(stb_dxt + 自研 BC2 alpha,实现宏仅在本 TU 展开)
#include    "image/FEBlockEncoder.hpp"

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

        /// <summary>
        /// KTX2 vkFormat 数值 → FEFormat;不认识返回 FMT_NONE
        /// 数值取自 libktx 自带 vkformat_enum.h(Vulkan 规范);
        /// sRGB 变体统一映射到宿主 UNORM 格式(引擎无逐通道 sRGB 未压缩格式)
        /// </summary>
        inline  FEFormat    formatOfVkFormat(uint32 vk)
        {
            switch (vk)
            {
            /// 未压缩 8bit UNORM(含 sRGB 变体)
            case 9u:    return  FMT_R8_UNORM;               /// VK_FORMAT_R8_UNORM
            case 15u:   return  FMT_R8_UNORM;               /// VK_FORMAT_R8_SRGB
            case 16u:   return  FMT_R8G8_UNORM;             /// VK_FORMAT_R8G8_UNORM
            case 22u:   return  FMT_R8G8_UNORM;             /// VK_FORMAT_R8G8_SRGB
            case 23u:   return  FMT_R8G8B8_UNORM;           /// VK_FORMAT_R8G8B8_UNORM
            case 29u:   return  FMT_R8G8B8_UNORM;           /// VK_FORMAT_R8G8B8_SRGB
            case 37u:   return  FMT_R8G8B8A8_UNORM;         /// VK_FORMAT_R8G8B8A8_UNORM
            case 43u:   return  FMT_R8G8B8A8_UNORM;         /// VK_FORMAT_R8G8B8A8_SRGB
            /// 未压缩 16bit UNORM
            case 70u:   return  FMT_R16_UNORM;              /// VK_FORMAT_R16_UNORM
            case 77u:   return  FMT_R16G16_UNORM;           /// VK_FORMAT_R16G16_UNORM
            case 84u:   return  FMT_R16G16B16_UNORM;        /// VK_FORMAT_R16G16B16_UNORM
            case 91u:   return  FMT_R16G16B16A16_UNORM;     /// VK_FORMAT_R16G16B16A16_UNORM
            /// 块压缩 BC
            case 131u:  return  FMT_BC1_RGB_UNORM;          /// VK_FORMAT_BC1_RGB_UNORM_BLOCK
            case 132u:  return  FMT_BC1_RGB_SRGB;           /// VK_FORMAT_BC1_RGB_SRGB_BLOCK
            case 133u:  return  FMT_BC1_RGBA_UNORM;         /// VK_FORMAT_BC1_RGBA_UNORM_BLOCK
            case 134u:  return  FMT_BC1_RGBA_SRGB;          /// VK_FORMAT_BC1_RGBA_SRGB_BLOCK
            case 135u:  return  FMT_BC2_UNORM;              /// VK_FORMAT_BC2_UNORM_BLOCK
            case 136u:  return  FMT_BC2_SRGB;               /// VK_FORMAT_BC2_SRGB_BLOCK
            case 137u:  return  FMT_BC3_UNORM;              /// VK_FORMAT_BC3_UNORM_BLOCK
            case 138u:  return  FMT_BC3_SRGB;               /// VK_FORMAT_BC3_SRGB_BLOCK
            case 139u:  return  FMT_BC4_UNORM;              /// VK_FORMAT_BC4_UNORM_BLOCK
            case 140u:  return  FMT_BC4_SNORM;              /// VK_FORMAT_BC4_SNORM_BLOCK
            case 141u:  return  FMT_BC5_UNORM;              /// VK_FORMAT_BC5_UNORM_BLOCK
            case 142u:  return  FMT_BC5_SNORM;              /// VK_FORMAT_BC5_SNORM_BLOCK
            case 143u:  return  FMT_BC6H_UFLOAT;            /// VK_FORMAT_BC6H_UFLOAT_BLOCK
            case 144u:  return  FMT_BC6H_SFLOAT;            /// VK_FORMAT_BC6H_SFLOAT_BLOCK
            case 145u:  return  FMT_BC7_UNORM;              /// VK_FORMAT_BC7_UNORM_BLOCK
            case 146u:  return  FMT_BC7_SRGB;               /// VK_FORMAT_BC7_SRGB_BLOCK
            /// 块压缩 ETC2/EAC
            case 147u:  return  FMT_ETC2_R8G8B8_UNORM;      /// VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK
            case 148u:  return  FMT_ETC2_R8G8B8_SRGB;       /// VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
            case 151u:  return  FMT_ETC2_R8G8B8A8_UNORM;    /// VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
            case 152u:  return  FMT_ETC2_R8G8B8A8_SRGB;     /// VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
            case 153u:  return  FMT_EAC_R11_UNORM;          /// VK_FORMAT_EAC_R11_UNORM_BLOCK
            case 154u:  return  FMT_EAC_R11_SNORM;          /// VK_FORMAT_EAC_R11_SNORM_BLOCK
            case 155u:  return  FMT_EAC_R11G11_UNORM;       /// VK_FORMAT_EAC_R11G11_UNORM_BLOCK
            case 156u:  return  FMT_EAC_R11G11_SNORM;       /// VK_FORMAT_EAC_R11G11_SNORM_BLOCK
            /// 块压缩 ASTC
            case 157u:  return  FMT_ASTC_4x4_UNORM;         /// VK_FORMAT_ASTC_4x4_UNORM_BLOCK
            case 158u:  return  FMT_ASTC_4x4_SRGB;          /// VK_FORMAT_ASTC_4x4_SRGB_BLOCK
            case 165u:  return  FMT_ASTC_6x6_UNORM;         /// VK_FORMAT_ASTC_6x6_UNORM_BLOCK
            case 166u:  return  FMT_ASTC_6x6_SRGB;          /// VK_FORMAT_ASTC_6x6_SRGB_BLOCK
            case 171u:  return  FMT_ASTC_8x8_UNORM;         /// VK_FORMAT_ASTC_8x8_UNORM_BLOCK
            case 172u:  return  FMT_ASTC_8x8_SRGB;          /// VK_FORMAT_ASTC_8x8_SRGB_BLOCK
            default:    return  FMT_NONE;
            }
        }

        /// <summary>
        /// KTX1 glInternalformat → FEFormat;不认识返回 FMT_NONE
        /// GL 常量数值与 libktx 自带 gl_format.h 一致;
        /// sRGB 变体统一映射到宿主 UNORM 格式
        /// </summary>
        inline  FEFormat    formatOfGlInternal(uint32 glInternal)
        {
            switch (glInternal)
            {
            /// 未压缩 8bit UNORM(含 sRGB 变体)
            case 0x8229u:   return  FMT_R8_UNORM;               /// GL_R8
            case 0x822Bu:   return  FMT_R8G8_UNORM;             /// GL_RG8
            case 0x8051u:   return  FMT_R8G8B8_UNORM;           /// GL_RGB8
            case 0x8058u:   return  FMT_R8G8B8A8_UNORM;         /// GL_RGBA8
            case 0x8C43u:   return  FMT_R8G8B8A8_UNORM;         /// GL_SRGB8_ALPHA8
            /// 未压缩 16bit UNORM
            case 0x822Au:   return  FMT_R16_UNORM;              /// GL_R16
            case 0x822Cu:   return  FMT_R16G16_UNORM;           /// GL_RG16
            case 0x8054u:   return  FMT_R16G16B16_UNORM;        /// GL_RGB16
            case 0x805Bu:   return  FMT_R16G16B16A16_UNORM;     /// GL_RGBA16
            /// 块压缩 BC
            case 0x83F0u:   return  FMT_BC1_RGB_UNORM;          /// GL_COMPRESSED_RGB_S3TC_DXT1_EXT
            case 0x83F1u:   return  FMT_BC1_RGBA_UNORM;         /// GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
            case 0x83F2u:   return  FMT_BC2_UNORM;              /// GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
            case 0x83F3u:   return  FMT_BC3_UNORM;              /// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
            case 0x8DBBu:   return  FMT_BC4_UNORM;              /// GL_COMPRESSED_RED_RGTC1
            case 0x8DBCu:   return  FMT_BC4_SNORM;              /// GL_COMPRESSED_SIGNED_RED_RGTC1
            case 0x8DBDu:   return  FMT_BC5_UNORM;              /// GL_COMPRESSED_RG_RGTC2
            case 0x8DBEu:   return  FMT_BC5_SNORM;              /// GL_COMPRESSED_SIGNED_RG_RGTC2
            case 0x8E8Eu:   return  FMT_BC6H_SFLOAT;            /// GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
            case 0x8E8Fu:   return  FMT_BC6H_UFLOAT;            /// GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
            case 0x8E8Cu:   return  FMT_BC7_UNORM;              /// GL_COMPRESSED_RGBA_BPTC_UNORM
            case 0x8E8Du:   return  FMT_BC7_SRGB;               /// GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
            /// 块压缩 ETC2/EAC
            case 0x9274u:   return  FMT_ETC2_R8G8B8_UNORM;      /// GL_COMPRESSED_RGB8_ETC2
            case 0x9275u:   return  FMT_ETC2_R8G8B8_SRGB;       /// GL_COMPRESSED_SRGB8_ETC2
            case 0x9278u:   return  FMT_ETC2_R8G8B8A8_UNORM;    /// GL_COMPRESSED_RGBA8_ETC2_EAC
            case 0x9279u:   return  FMT_ETC2_R8G8B8A8_SRGB;     /// GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
            case 0x9270u:   return  FMT_EAC_R11_UNORM;          /// GL_COMPRESSED_R11_EAC
            case 0x9271u:   return  FMT_EAC_R11_SNORM;          /// GL_COMPRESSED_SIGNED_R11_EAC
            case 0x9272u:   return  FMT_EAC_R11G11_UNORM;       /// GL_COMPRESSED_RG11_EAC
            case 0x9273u:   return  FMT_EAC_R11G11_SNORM;       /// GL_COMPRESSED_SIGNED_RG11_EAC
            /// 块压缩 ASTC
            case 0x93B0u:   return  FMT_ASTC_4x4_UNORM;         /// GL_COMPRESSED_RGBA_ASTC_4x4_KHR
            case 0x93D0u:   return  FMT_ASTC_4x4_SRGB;          /// GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
            case 0x93B4u:   return  FMT_ASTC_6x6_UNORM;         /// GL_COMPRESSED_RGBA_ASTC_6x6_KHR
            case 0x93D4u:   return  FMT_ASTC_6x6_SRGB;          /// GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
            case 0x93B8u:   return  FMT_ASTC_8x8_UNORM;         /// GL_COMPRESSED_RGBA_ASTC_8x8_KHR
            case 0x93D8u:   return  FMT_ASTC_8x8_SRGB;          /// GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
            default:        return  FMT_NONE;
            }
        }

        /// <summary>
        /// FEFormat → KTX2 vkFormat 数值(8bit/16bit UNORM 未压缩 + 全部块压缩;不支持返回 0)
        /// </summary>
        inline  uint32  vkFormatOf(FEFormat fmt)
        {
            switch (fmt)
            {
            case FMT_R8_UNORM:              return  9u;
            case FMT_R8G8_UNORM:            return  16u;
            case FMT_R8G8B8_UNORM:          return  23u;
            case FMT_R8G8B8A8_UNORM:        return  37u;
            case FMT_R16_UNORM:             return  70u;
            case FMT_R16G16_UNORM:          return  77u;
            case FMT_R16G16B16_UNORM:       return  84u;
            case FMT_R16G16B16A16_UNORM:    return  91u;

            case FMT_BC1_RGB_UNORM:         return  131u;
            case FMT_BC1_RGB_SRGB:          return  132u;
            case FMT_BC1_RGBA_UNORM:        return  133u;
            case FMT_BC1_RGBA_SRGB:         return  134u;
            case FMT_BC2_UNORM:             return  135u;
            case FMT_BC2_SRGB:              return  136u;
            case FMT_BC3_UNORM:             return  137u;
            case FMT_BC3_SRGB:              return  138u;
            case FMT_BC4_UNORM:             return  139u;
            case FMT_BC4_SNORM:             return  140u;
            case FMT_BC5_UNORM:             return  141u;
            case FMT_BC5_SNORM:             return  142u;
            case FMT_BC6H_UFLOAT:           return  143u;
            case FMT_BC6H_SFLOAT:           return  144u;
            case FMT_BC7_UNORM:             return  145u;
            case FMT_BC7_SRGB:              return  146u;
            case FMT_ETC2_R8G8B8_UNORM:     return  147u;
            case FMT_ETC2_R8G8B8_SRGB:      return  148u;
            case FMT_ETC2_R8G8B8A8_UNORM:   return  151u;
            case FMT_ETC2_R8G8B8A8_SRGB:    return  152u;
            case FMT_EAC_R11_UNORM:         return  153u;
            case FMT_EAC_R11_SNORM:         return  154u;
            case FMT_EAC_R11G11_UNORM:      return  155u;
            case FMT_EAC_R11G11_SNORM:      return  156u;
            case FMT_ASTC_4x4_UNORM:        return  157u;
            case FMT_ASTC_4x4_SRGB:         return  158u;
            case FMT_ASTC_6x6_UNORM:        return  165u;
            case FMT_ASTC_6x6_SRGB:         return  166u;
            case FMT_ASTC_8x8_UNORM:        return  171u;
            case FMT_ASTC_8x8_SRGB:         return  172u;
            default:                        return  0u;
            }
        }

        /// <summary>
        /// FEFormat → BasisU 转码目标(ktx2 专属);不支持返回 false
        /// FMT_NONE 按默认 RGBA32(与历史行为一致);
        /// ETC2 RGB 转码目标用 ETC1_RGB(8字节块,ETC2 兼容)
        /// </summary>
        inline  bool    transcodeTargetOf(FEFormat fmt,ktx_transcode_fmt_e& ttf)
        {
            switch (fmt)
            {
            case FMT_NONE:                  ttf = KTX_TTF_RGBA32;           return  true;
            case FMT_R8G8B8A8_UNORM:        ttf = KTX_TTF_RGBA32;           return  true;
            case FMT_BC1_RGB_UNORM:         ttf = KTX_TTF_BC1_RGB;          return  true;
            case FMT_BC3_UNORM:             ttf = KTX_TTF_BC3_RGBA;         return  true;
            case FMT_BC4_UNORM:             ttf = KTX_TTF_BC4_R;            return  true;
            case FMT_BC5_UNORM:             ttf = KTX_TTF_BC5_RG;           return  true;
            case FMT_BC7_UNORM:             ttf = KTX_TTF_BC7_RGBA;         return  true;
            case FMT_ETC2_R8G8B8_UNORM:     ttf = KTX_TTF_ETC1_RGB;         return  true;
            case FMT_ETC2_R8G8B8A8_UNORM:   ttf = KTX_TTF_ETC2_RGBA;        return  true;
            case FMT_EAC_R11_UNORM:         ttf = KTX_TTF_ETC2_EAC_R11;     return  true;
            case FMT_EAC_R11G11_UNORM:      ttf = KTX_TTF_ETC2_EAC_RG11;    return  true;
            case FMT_ASTC_4x4_UNORM:        ttf = KTX_TTF_ASTC_4x4_RGBA;    return  true;
            default:                                                        return  false;
            }
        }

        /// <summary>
        /// 编码方向(compress)的 Basis 间接路线目标:sRGB 变体归一为 UNORM 后
        /// 复用 transcodeTargetOf(sRGB 块字节与 UNORM 相同,编码结果直接复用)
        /// </summary>
        inline  bool    compressBasisTargetOf(FEFormat fmt,ktx_transcode_fmt_e& ttf)
        {
            switch (fmt)
            {
            case FMT_BC7_SRGB:              fmt = FMT_BC7_UNORM;            break;
            case FMT_ETC2_R8G8B8_SRGB:      fmt = FMT_ETC2_R8G8B8_UNORM;    break;
            case FMT_ETC2_R8G8B8A8_SRGB:    fmt = FMT_ETC2_R8G8B8A8_UNORM;  break;
            case FMT_ASTC_4x4_SRGB:         fmt = FMT_ASTC_4x4_UNORM;       break;
            default:                                                        break;
            }
            return  transcodeTargetOf(fmt,ttf);
        }

        /// <summary>
        /// 块格式 → 默认解码目标(transcode 的 fmt == FMT_NONE):
        /// BC4/BC5/EAC 单/双通道按字节直出目标选择,BC6H 为 HDR,其余为 RGBA8
        /// </summary>
        inline  FEFormat    decodeDefaultTarget(FEFormat src)
        {
            switch (src)
            {
            case FMT_BC4_UNORM:
            case FMT_EAC_R11_UNORM:             return  FMT_R8_UNORM;
            case FMT_BC4_SNORM:
            case FMT_EAC_R11_SNORM:             return  FMT_R8_SNORM;
            case FMT_BC5_UNORM:
            case FMT_EAC_R11G11_UNORM:          return  FMT_R8G8_UNORM;
            case FMT_BC5_SNORM:
            case FMT_EAC_R11G11_SNORM:          return  FMT_R8G8_SNORM;
            case FMT_BC6H_UFLOAT:
            case FMT_BC6H_SFLOAT:               return  FMT_R32G32B32A32_FLOAT;
            default:                            return  FMT_R8G8B8A8_UNORM;
            }
        }
        /// <summary>
        /// 显式解码目标白名单:8bit UNORM + R8/RG8 SNORM + 16bit half + 32bit float
        /// </summary>
        inline  bool    isDecodeTarget(FEFormat fmt)
        {
            if  (FEFormatHelper::isBlockFormat(fmt))
                return  false;
            switch (fmt)
            {
            case FMT_R8_UNORM:
            case FMT_R8G8_UNORM:
            case FMT_R8G8B8_UNORM:
            case FMT_R8G8B8A8_UNORM:
            case FMT_R8_SNORM:
            case FMT_R8G8_SNORM:
            case FMT_R16_FLOAT:
            case FMT_R16G16_FLOAT:
            case FMT_R16G16B16_FLOAT:
            case FMT_R16G16B16A16_FLOAT:
            case FMT_R32_FLOAT:
            case FMT_R32G32_FLOAT:
            case FMT_R32G32B32_FLOAT:
            case FMT_R32G32B32A32_FLOAT:
                return  true;
            default:
                return  false;
            }
        }
        /// <summary>
        /// 单块解码 → float 缓冲(raster 顺序,每 texel 连续 comps 个 float);
        /// 返回分量数(1/2/4);覆盖全部 30 种块格式
        /// </summary>
        inline  int     decodeBlockToFloats(FEFormat srcFmt,const uint8_t* blk,float* f)
        {
            using   namespace   BlockDecoderImpl;
            const float     kInv255 =   1.0f / 255.0f;
            switch (srcFmt)
            {
            case FMT_BC1_RGB_UNORM:
            case FMT_BC1_RGB_SRGB:
            case FMT_BC1_RGBA_UNORM:
            case FMT_BC1_RGBA_SRGB:
            case FMT_BC2_UNORM:
            case FMT_BC2_SRGB:
            case FMT_BC3_UNORM:
            case FMT_BC3_SRGB:
            case FMT_BC7_UNORM:
            case FMT_BC7_SRGB:
                {
                    uint8_t     rgba[16 * 4];
                    decodeBC(srcFmt,blk,rgba,4 * 4);
                    for (int i = 0; i < 16 * 4; ++i)
                        f[i]    =   rgba[i] * kInv255;
                }
                return  4;
            case FMT_BC4_UNORM:
            case FMT_BC4_SNORM:
                {
                    uint8_t     v[16];
                    const bool  bSigned    =   (srcFmt == FMT_BC4_SNORM);
                    decodeBC(srcFmt,blk,v,4);
                    for (int i = 0; i < 16; ++i)
                        f[i]    =   bSigned ? (float(int8_t(v[i])) / 127.0f)
                                            : (v[i] * kInv255);
                }
                return  1;
            case FMT_BC5_UNORM:
            case FMT_BC5_SNORM:
                {
                    uint8_t     v[16 * 2];
                    const bool  bSigned    =   (srcFmt == FMT_BC5_SNORM);
                    decodeBC(srcFmt,blk,v,8);
                    for (int i = 0; i < 16; ++i)
                    {
                        f[i*2+0]    =   bSigned ? (float(int8_t(v[i*2+0])) / 127.0f)
                                                : (v[i*2+0] * kInv255);
                        f[i*2+1]    =   bSigned ? (float(int8_t(v[i*2+1])) / 127.0f)
                                                : (v[i*2+1] * kInv255);
                    }
                }
                return  2;
            case FMT_BC6H_UFLOAT:
            case FMT_BC6H_SFLOAT:
                {
                    float   rgb[16 * 3];
                    decodeBC6H(srcFmt,blk,rgb,4);
                    for (int i = 0; i < 16; ++i)
                    {
                        f[i*4+0]    =   rgb[i*3+0];
                        f[i*4+1]    =   rgb[i*3+1];
                        f[i*4+2]    =   rgb[i*3+2];
                        f[i*4+3]    =   1.0f;
                    }
                }
                return  4;
            case FMT_ETC2_R8G8B8_UNORM:
            case FMT_ETC2_R8G8B8_SRGB:
                {
                    uint8_t     rgba[16 * 4];
                    etc2ColorDecode(blk,false,rgba);
                    for (int i = 0; i < 16 * 4; ++i)
                        f[i]    =   rgba[i] * kInv255;
                }
                return  4;
            case FMT_ETC2_R8G8B8A8_UNORM:
            case FMT_ETC2_R8G8B8A8_SRGB:
                {
                    /// ETC2_RGBA8 块布局(GPU 硬件/basisu 一致):[8B EAC alpha][8B ETC2 color];
                    /// alpha 半块即 R11 EAC 语义(11bit 值域,8bit 输出经 [0,1] 缩放)
                    uint8_t     rgba[16 * 4];
                    float       a[16];
                    etc2ColorDecode(blk + 8,false,rgba);
                    eacDecode(blk,false,a);
                    /// 颜色半块 raster(i = y*4+x),alpha 半块列主序(i = x*4+y)
                    for (int i = 0; i < 16; ++i)
                    {
                        const float v   =   a[(i & 3) * 4 + (i >> 2)] * 255.0f;
                        rgba[i*4+3] =   (uint8_t)(std::min)(255.0f,(std::max)(0.0f,v + 0.5f));
                    }
                    for (int i = 0; i < 16 * 4; ++i)
                        f[i]    =   rgba[i] * kInv255;
                }
                return  4;
            case FMT_EAC_R11_UNORM:
            case FMT_EAC_R11_SNORM:
                {
                    float   v[16];
                    eacDecode(blk,srcFmt == FMT_EAC_R11_SNORM,v);
                    /// 列主序 → raster
                    for (int ty = 0; ty < 4; ++ty)
                        for (int tx = 0; tx < 4; ++tx)
                            f[ty*4+tx]  =   v[tx*4+ty];
                }
                return  1;
            case FMT_EAC_R11G11_UNORM:
            case FMT_EAC_R11G11_SNORM:
                {
                    float   r[16],g[16];
                    const bool  bSigned    =   (srcFmt == FMT_EAC_R11G11_SNORM);
                    eacDecode(blk,bSigned,r);
                    eacDecode(blk + 8,bSigned,g);
                    for (int ty = 0; ty < 4; ++ty)
                        for (int tx = 0; tx < 4; ++tx)
                        {
                            f[(ty*4+tx)*2+0]    =   r[tx*4+ty];
                            f[(ty*4+tx)*2+1]    =   g[tx*4+ty];
                        }
                }
                return  2;
            default:    /// ASTC 4x4/6x6/8x8
                {
                    const uint2     be  =   FEFormatHelper::blockExtent(srcFmt);
                    const int       n   =   int(be.x * be.y);
                    uint8_t     rgba[64 * 4];
                    if  (!astcDecodeBlock(blk,int(be.x),int(be.y),rgba))
                    {
                        /// error block:规范要求输出不透明洋红
                        for (int i = 0; i < n; ++i)
                        {
                            rgba[i*4+0] =   255;
                            rgba[i*4+1] =   0;
                            rgba[i*4+2] =   255;
                            rgba[i*4+3] =   255;
                        }
                    }
                    for (int i = 0; i < n * 4; ++i)
                        f[i]    =   rgba[i] * kInv255;
                }
                return  4;
            }
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
        ktxTexture* ktex    =   nullptr;
        if  (ktxTexture_CreateFromNamedFile(fileName,KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,&ktex) != KTX_SUCCESS
             || ktex == nullptr)
            return  nullptr;

        ImagePtr    result  =   nullptr;
        do
        {
            /// 路线一:BasisU/ETC1S/UASTC 压缩块 → 按目标格式转码(仅 ktx2);
            /// 转码成功后 vkFormat 更新为目标块格式,后续走统一透传路径
            if  (ktxTexture_NeedsTranscoding(ktex))
            {
                ktx_transcode_fmt_e ttf =   KTX_TTF_RGBA32;
                if  (ktex->classId != ktxTexture2_c || !transcodeTargetOf(fmt,ttf)
                     || ktxTexture2_TranscodeBasis(reinterpret_cast<ktxTexture2*>(ktex),ttf,0) != KTX_SUCCESS)
                    break;
            }
            /// 路线二:容器格式 → FEFormat(vkFormat / glInternalformat 映射)
            FEFormat    srcFmt  =   FMT_NONE;
            if  (ktex->classId == ktxTexture2_c)
                srcFmt  =   formatOfVkFormat(reinterpret_cast<ktxTexture2*>(ktex)->vkFormat);
            else
                srcFmt  =   formatOfGlInternal(reinterpret_cast<ktxTexture1*>(ktex)->glInternalformat);
            /// 不认识的格式(含 ktx1 压缩格式无通用解码路径)或非法尺寸 → 拒绝
            if  (srcFmt == FMT_NONE || FEFormatHelper::sizeOf(srcFmt) == 0
                 || ktex->baseWidth == 0 || ktex->baseHeight == 0)
                break;
            /// 显式指定目标格式时必须与容器一致(透传不做 CPU 转换);
            /// 例外:容器为块压缩且目标为未压缩 → 装载后经 transcode 做 CPU 块解码
            bool    bDecode =   false;
            if  (fmt != FMT_NONE && fmt != srcFmt)
            {
                if  (!FEFormatHelper::isBlockFormat(srcFmt) || FEFormatHelper::isBlockFormat(fmt))
                    break;
                bDecode =   true;
            }

            /// 支持 2D(单层) 与 cubemap(6 面,KTX/KTX2 均由 isCubemap 标记);
            /// 每面独立按 level 读取,面序与 Vulkan/GL 规范一致(+X,-X,+Y,-Y,+Z,-Z)
            const uint32    faces   =   (ktex->isCubemap != 0) ? 6u : 1u;

            /// 创建带完整 mip 链的目标图像
            FEImage::CreateInfo ci;
            ci._extent  =   uint3(ktex->baseWidth,ktex->baseHeight,1);
            ci._format  =   srcFmt;
            ci._layers  =   uint16(faces);
            ci._levels  =   uint16(ktex->numLevels > 0 ? ktex->numLevels : 1);
            ImagePtr    image   =   new FEImage(ctx);
            if  (!image->create(ci) || image->size() == 0)
                break;

            /// 路线三:原始数据按行拷贝(块压缩与未压缩统一公式):
            /// - 块格式:行数 = ceil(h/块高),行字节 = ceil(w/块宽) * 每块字节;
            /// - 未压缩:块粒度退化为 1x1,即普通像素行
            /// 行序:写入端给什么顺序,读出来就是什么顺序;
            /// 我们的测试写入用顶行在前(引擎约定),这里原样拷贝不翻转
            const uint2     be      =   FEFormatHelper::blockExtent(srcFmt);
            const uint32    eltByte =   FEFormatHelper::sizeOf(srcFmt);
            const uint8*    texData =   ktxTexture_GetData(ktex);
            bool    ok  =   (texData != nullptr);
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
                    const uint32    rows    =   (h + be.y - 1) / be.y;
                    const size_t    rowByte =   size_t((w + be.x - 1) / be.x) * eltByte;
                    const uint8*    srcBase =   texData + offset;
                    /// 块格式行按规范本就紧凑,自算 pitch;libktx 的 GetRowPitch
                    /// 用 floor 整除算块列数,尺寸非块宽整数倍时(如 8px 宽的
                    /// ASTC_6x6 → 8/6=1)偏小;未压缩保留 GetRowPitch(含对齐)
                    const size_t    srcPitch=   FEFormatHelper::isBlockFormat(srcFmt)
                                                ?   rowByte
                                                :   size_t(ktxTexture_GetRowPitch(ktex,level));
                    auto            plane   =   image->plane(level,face);
                    if  (srcPitch < rowByte || plane._buffer == nullptr)
                    {
                        ok  =   false;
                        break;
                    }
                    for (uint32 y = 0; y < rows; ++y)
                        std::memcpy(plane.dataOffset<uint8>(y,0),srcBase + size_t(y) * srcPitch,rowByte);
                }
            }
            if  (!ok)
                break;
            result  =   image;
            /// 块压缩容器 + 显式未压缩目标:CPU 逐块解码为未压缩格式
            if  (bDecode)
                result  =   transcode(*image,fmt);
        } while (false);

        ktxTexture_Destroy(ktex);
        return  result;
    }

    /// <summary>
    /// =====================================================================
    /// transcode:块压缩 → 未压缩的 CPU 转码
    /// 每块经 decodeBlockToFloats 解码为 float,再经 FEFormatHelper::packFloatsCore
    /// 打包为目标格式逐 texel 散写;BC4/BC5 → 同量化语义的 R8/RG8 走无损字节直出;
    /// 块寻址按 blockExtent(ASTC 6x6/8x8 为 6x6/8x8 块),边缘块按图像边界裁剪
    /// =====================================================================
    /// </summary>
    ImagePtr    FEImageHelper::transcode(FEImage& src,FEFormat fmt)
    {
        const auto&     ci      =   src.cInfo();
        const FEFormat  srcFmt  =   ci._format;
        if  (!FEFormatHelper::isBlockFormat(srcFmt))
            return  nullptr;
        if  (fmt == FMT_NONE)
            fmt =   decodeDefaultTarget(srcFmt);
        if  (!isDecodeTarget(fmt))
            return  nullptr;

        const uint32    levels  =   (std::max<uint32>)(1,ci._levels);
        const uint32    layers  =   (std::max<uint32>)(1,ci._layers);
        FEImage::CreateInfo ciDst   =   ci;
        ciDst._format   =   fmt;
        ciDst._buffers.clear();
        ImagePtr    dst =   new FEImage(src.ctx());
        if  (!dst->create(ciDst) || dst->size() == 0)
            return  nullptr;

        const uint2     be      =   FEFormatHelper::blockExtent(srcFmt);
        const uint32    pxByte  =   FEFormatHelper::sizeOf(fmt);
        /// BC4/BC5 → 同量化语义的 R8/RG8(UNORM↔UNORM,SNORM↔SNORM):无损字节直出
        bool    bByte   =   false;
        int     srcCh   =   0;
        switch (srcFmt)
        {
        case FMT_BC4_UNORM:     bByte = (fmt == FMT_R8_UNORM);      srcCh = 1;  break;
        case FMT_BC4_SNORM:     bByte = (fmt == FMT_R8_SNORM);      srcCh = 1;  break;
        case FMT_BC5_UNORM:     bByte = (fmt == FMT_R8G8_UNORM);    srcCh = 2;  break;
        case FMT_BC5_SNORM:     bByte = (fmt == FMT_R8G8_SNORM);    srcCh = 2;  break;
        default:                break;
        }

        bool    ok  =   true;
        for (uint32 layer = 0; ok && layer < layers; ++layer)
        {
            for (uint32 level = 0; ok && level < levels; ++level)
            {
                const auto  sp  =   src.plane(level,layer);
                auto        dp  =   dst->plane(level,layer);
                if  (sp._buffer == nullptr || dp._buffer == nullptr)
                {
                    ok  =   false;
                    break;
                }
                const uint32    w   =   sp.width();
                const uint32    h   =   sp.height();
                const uint32    bw  =   (w + be.x - 1) / be.x;
                const uint32    bh  =   (h + be.y - 1) / be.y;
                for (uint32 by = 0; ok && by < bh; ++by)
                {
                    for (uint32 bx = 0; bx < bw; ++bx)
                    {
                        const uint8_t*  blk =   sp.dataOffset<uint8>(by,bx);
                        const uint32    x0  =   bx * be.x;
                        const uint32    y0  =   by * be.y;
                        const uint32    cw  =   (std::min<uint32>)(be.x,w - x0);
                        const uint32    ch  =   (std::min<uint32>)(be.y,h - y0);
                        if  (bByte)
                        {
                            /// 无损直出:块解码到字节后按行拷贝(字节布局与目标一致)
                            uint8_t     rgba[64 * 2];
                            BlockDecoderImpl::decodeBC(srcFmt,blk,rgba,int(be.x) * srcCh);
                            for (uint32 ty = 0; ty < ch; ++ty)
                                std::memcpy(dp.dataOffset<uint8>(y0 + ty,x0),
                                            rgba + size_t(ty) * be.x * srcCh,
                                            size_t(cw) * srcCh);
                        }
                        else
                        {
                            /// float 管线:块 → float(raster) → 打包 → 按行散写
                            float   blkF[64 * 4];
                            const int   comps   =   decodeBlockToFloats(srcFmt,blk,blkF);
                            uint8s  packed  =   FEFormatHelper::packFloatsCore(blkF,size_t(be.x * be.y),uint(comps),fmt);
                            if  (packed.size() < size_t(be.x * be.y) * pxByte)
                            {
                                ok  =   false;
                                break;
                            }
                            for (uint32 ty = 0; ty < ch; ++ty)
                                std::memcpy(dp.dataOffset<uint8>(y0 + ty,x0),
                                            packed.data() + size_t(ty) * be.x * pxByte,
                                            size_t(cw) * pxByte);
                        }
                    }
                }
            }
        }
        if  (!ok)
            return  nullptr;
        return  dst;
    }

    /// <summary>
    /// =====================================================================
    /// compressViaBasis:Basis 间接编码路线(10 种目标)
    ///   建 ktx2(源 UNORM vkFormat,完整 mip 链)→ 逐 level 拷入源数据 →
    ///   CompressBasisEx(ETC1S / UASTC_LDR_4x4)→ TranscodeBasis(目标块)→
    ///   逐 level 逐块拷出;ktx2 未压缩布局 level 间紧凑,转码后块布局同样
    ///   经 GetImageOffset 定位,不依赖行连续假设
    /// =====================================================================
    /// </summary>
    static  bool    compressViaBasis(FEImage& src,FEFormat dstFmt,FEImage& dst,int quality)
    {
        const auto&     ci      =   src.cInfo();
        const FEFormat  srcFmt  =   ci._format;
        const uint32    levels  =   (std::max<uint32>)(1,ci._levels);
        /// 源未压缩格式 → vkFormat(Basis 编码输入)
        uint32  vkSrc   =   0;
        switch (srcFmt)
        {
        case FMT_R8_UNORM:          vkSrc = 9u;     break;  /// VK_FORMAT_R8_UNORM
        case FMT_R8G8_UNORM:        vkSrc = 16u;    break;  /// VK_FORMAT_R8G8_UNORM
        case FMT_R8G8B8_UNORM:      vkSrc = 23u;    break;  /// VK_FORMAT_R8G8B8_UNORM
        case FMT_R8G8B8A8_UNORM:    vkSrc = 37u;    break;  /// VK_FORMAT_R8G8B8A8_UNORM
        default:                                    break;
        }
        /// 目标 → codec + 转码格式;BC7/ASTC4x4 走 UASTC,ETC2/EAC 走 ETC1S
        ktx_basis_codec         codec   =   KTX_BASIS_CODEC_ETC1S;
        switch (dstFmt)
        {
        case FMT_BC7_UNORM:
        case FMT_BC7_SRGB:
        case FMT_ASTC_4x4_UNORM:
        case FMT_ASTC_4x4_SRGB:
            codec    =   KTX_BASIS_CODEC_UASTC_LDR_4x4;
            break;
        default:
            break;
        }
        ktx_transcode_fmt_e     ttf     =   KTX_TTF_RGBA32;
        if  (vkSrc == 0u || !compressBasisTargetOf(dstFmt,ttf))
            return  false;

        ktxTextureCreateInfo    kci     =   {};
        kci.vkFormat        =   vkSrc;
        kci.baseWidth       =   ci._extent.x;
        kci.baseHeight      =   ci._extent.y;
        kci.baseDepth       =   1;
        kci.numDimensions   =   2;
        kci.numLevels       =   levels;
        kci.numLayers       =   1;
        kci.numFaces        =   1;
        kci.isArray         =   KTX_FALSE;
        kci.generateMipmaps =   KTX_FALSE;
        ktxTexture2*    ktex    =   nullptr;
        KTX_error_code  err     =   ktxTexture2_Create(&kci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&ktex);
        if  (err != KTX_SUCCESS || ktex == nullptr)
            return  false;
        do
        {
            /// 逐 level 拷入源数据(ktx2 未压缩布局:level 紧凑、行紧凑)
            for (uint32 level = 0; level < levels; ++level)
            {
                const auto  sp  =   src.plane(level,0);
                if  (sp._buffer == nullptr)
                {
                    err =   KTX_INVALID_OPERATION;
                    break;
                }
                ktx_size_t  offset  =   0;
                err =   ktxTexture2_GetImageOffset(ktex,level,0,0,&offset);
                if  (err != KTX_SUCCESS)
                    break;
                const uint32    rowBytes    =   sp.width() * FEFormatHelper::sizeOf(srcFmt);
                uint8_t*        dp          =   ktex->pData + offset;
                for (uint32 y = 0; y < sp.height(); ++y)
                    std::memcpy(dp + (size_t)y * rowBytes,
                                sp.dataOffset<uint8>(y,0),rowBytes);
            }
            if  (err != KTX_SUCCESS)
                break;
            /// Basis 编码(ETC1S / UASTC_LDR):threadCount=1 保证输出确定
            ktxBasisParams  params  =   {};
            params.structSize            =   sizeof(params);
            params.codec                 =   codec;
            params.threadCount           =   1;
            params.etc1sCompressionLevel =   KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
            params.qualityLevel          =   (ktx_uint32_t)(std::min<int>)(
                                             255,(std::max<int>)(1,quality));
            err =   ktxTexture2_CompressBasisEx(ktex,&params);
            if  (err != KTX_SUCCESS)
                break;
            /// 转码为目标块格式(块数据留在 ktex->pData)
            err =   ktxTexture2_TranscodeBasis(ktex,ttf,
                                               (quality >= 160) ? KTX_TF_HIGH_QUALITY : 0);
            if  (err != KTX_SUCCESS)
                break;
            /// 逐 level 逐块拷出(与解码循环同构,不依赖行连续)
            const uint2     be          =   FEFormatHelper::blockExtent(dstFmt);
            const uint32    blkBytes    =   FEFormatHelper::sizeOf(dstFmt);
            for (uint32 level = 0; level < levels && err == KTX_SUCCESS; ++level)
            {
                auto        dp  =   dst.plane(level,0);
                if  (dp._buffer == nullptr)
                {
                    err =   KTX_INVALID_OPERATION;
                    break;
                }
                ktx_size_t  offset  =   0;
                err =   ktxTexture2_GetImageOffset(ktex,level,0,0,&offset);
                if  (err != KTX_SUCCESS)
                    break;
                const uint32    w   =   dp.width();
                const uint32    h   =   dp.height();
                const uint32    bw  =   (w + be.x - 1) / be.x;
                const uint32    bh  =   (h + be.y - 1) / be.y;
                const uint8_t*  sl  =   ktex->pData + offset;
                for (uint32 by = 0; by < bh; ++by)
                {
                    for (uint32 bx = 0; bx < bw; ++bx)
                    {
                        std::memcpy(dp.dataOffset<uint8>(by,bx),
                                    sl + (size_t(by) * bw + bx) * blkBytes,blkBytes);
                    }
                }
            }
        } while (false);
        ktxTexture_Destroy(ktxTexture(ktex));
        return  err == KTX_SUCCESS;
    }

    /// <summary>
    /// =====================================================================
    /// compress:未压缩 → 块压缩的 CPU 编码(有损,20 种目标)
    /// 直编 10 种:BC1_RGB/BC1_RGBA/BC2/BC3(含 sRGB 变体)+ BC4/BC5(UNORM):
    ///   逐块 gather 边缘外推为完整 4x4 RGBA8 → stb_dxt / 自研 BC2 显式 alpha
    /// Basis 间接 10 种:BC7/ASTC4x4(UASTC)、ETC2_RGB/ETC2_RGBA(ETC1S)、
    ///   EAC_R11/R11G11(ETC1S 单/双通道);sRGB 变体块字节与 UNORM 相同
    /// 源限 8bit UNORM(R8/RG8/RGB8/RGBA8);缺失通道 i==3 补 255 其余补 0;
    /// EAC 单/双通道目标要求源通道精确匹配;Basis 路线仅支持单 layer
    /// =====================================================================
    /// </summary>
    ImagePtr    FEImageHelper::compress(FEImage& src,FEFormat dstFmt,int quality)
    {
        const auto&     ci      =   src.cInfo();
        const FEFormat  srcFmt  =   ci._format;
        /// 源限 8bit UNORM 4 种(sRGB 输入已在装载侧归一为 UNORM)
        if  (srcFmt != FMT_R8_UNORM && srcFmt != FMT_R8G8_UNORM &&
            srcFmt != FMT_R8G8B8_UNORM && srcFmt != FMT_R8G8B8A8_UNORM)
            return  nullptr;
        if  (!FEFormatHelper::isBlockFormat(dstFmt))
            return  nullptr;
        /// EAC 单/双通道目标要求源通道精确匹配(Basis 单/双通道编码路径)
        const uint32    srcCh   =   FEFormatHelper::channelOf(srcFmt);
        if  (dstFmt == FMT_EAC_R11_UNORM && srcCh != 1)
            return  nullptr;
        if  (dstFmt == FMT_EAC_R11G11_UNORM && srcCh != 2)
            return  nullptr;
        /// 直编 or Basis 间接;两者都不支持(BC6H/SNORM/ASTC 6x6/8x8)即拒绝
        const bool  bDirect =   BlockEncoderImpl::isEncodeSupported(dstFmt);
        if  (!bDirect)
        {
            ktx_transcode_fmt_e ttf =   KTX_TTF_RGBA32;
            if  (!compressBasisTargetOf(dstFmt,ttf))
                return  nullptr;
            /// Basis 间接路线单 layer(ktx2 容器 numLayers=1)
            if  ((std::max<uint32>)(1,ci._layers) > 1)
                return  nullptr;
        }

        const uint32    levels  =   (std::max<uint32>)(1,ci._levels);
        const uint32    layers  =   (std::max<uint32>)(1,ci._layers);
        FEImage::CreateInfo ciDst   =   ci;
        ciDst._format   =   dstFmt;
        ciDst._buffers.clear();
        ImagePtr    dst =   new FEImage(src.ctx());
        if  (!dst->create(ciDst) || dst->size() == 0)
            return  nullptr;

        bool    ok  =   true;
        if  (bDirect)
        {
            const uint2     be      =   FEFormatHelper::blockExtent(dstFmt);
            const uint32    srcBpp  =   FEFormatHelper::sizeOf(srcFmt);
            for (uint32 layer = 0; ok && layer < layers; ++layer)
            {
                for (uint32 level = 0; ok && level < levels; ++level)
                {
                    const auto  sp  =   src.plane(level,layer);
                    auto        dp  =   dst->plane(level,layer);
                    if  (sp._buffer == nullptr || dp._buffer == nullptr)
                    {
                        ok  =   false;
                        break;
                    }
                    const uint32    w   =   sp.width();
                    const uint32    h   =   sp.height();
                    const uint32    bw  =   (w + be.x - 1) / be.x;
                    const uint32    bh  =   (h + be.y - 1) / be.y;
                    for (uint32 by = 0; ok && by < bh; ++by)
                    {
                        for (uint32 bx = 0; bx < bw; ++bx)
                        {
                            const uint32    x0  =   bx * be.x;
                            const uint32    y0  =   by * be.y;
                            const uint32    cw  =   (std::min<uint32>)(be.x,w - x0);
                            const uint32    ch  =   (std::min<uint32>)(be.y,h - y0);
                            /// gather 完整 4x4 RGBA8 块:缺失通道规整(默认 RGBA=0,0,0,255)
                            uint8_t rgba[64];
                            for (int i = 0; i < 16; ++i)
                            {
                                rgba[i * 4 + 0] =   0;
                                rgba[i * 4 + 1] =   0;
                                rgba[i * 4 + 2] =   0;
                                rgba[i * 4 + 3] =   255;
                            }
                            for (uint32 y = 0; y < ch; ++y)
                            {
                                const uint8_t*  srow    =   sp.dataOffset<uint8>(y0 + y,x0);
                                for (uint32 x = 0; x < cw; ++x)
                                {
                                    uint8_t*    d   =   rgba + (y * 4 + x) * 4;
                                    for (uint32 c = 0; c < srcCh; ++c)
                                        d[c]    =   srow[x * srcBpp + c];
                                }
                            }
                            /// 边缘外推:行内向右复制最后有效像素,再纵向复制最后有效行
                            if  (cw < be.x)
                            {
                                for (uint32 y = 0; y < ch; ++y)
                                {
                                    for (uint32 x = cw; x < be.x; ++x)
                                    {
                                        std::memcpy(rgba + (y * 4 + x) * 4,
                                                    rgba + (y * 4 + cw - 1) * 4,4);
                                    }
                                }
                            }
                            if  (ch < be.y)
                            {
                                for (uint32 y = ch; y < be.y; ++y)
                                    std::memcpy(rgba + y * 16,rgba + (ch - 1) * 16,16);
                            }
                            BlockEncoderImpl::encodeBlock(dstFmt,rgba,
                                                          dp.dataOffset<uint8>(by,bx),quality);
                        }
                    }
                }
            }
        }
        else
        {
            ok  =   compressViaBasis(src,dstFmt,*dst,quality);
        }
        if  (!ok)
        {
            delete  dst;
            return  nullptr;
        }
        return  dst;
    }

    /// <summary>
    /// =====================================================================
    /// saveKtxInternal:FEImage → libktx → ktx 或 ktx2 文件
    /// 未压缩:1/2/3/4 通道 8bit(ktx1/ktx2)+ 16bit UNORM(仅 ktx2);
    /// 块压缩:全部块格式原样直写(仅 ktx2,不做 CPU 编码/解码);
    /// mip 链逐层写入;ktx2 + quality>0 且未压缩时经 CompressBasis 转 ETC1S
    /// =====================================================================
    /// </summary>
    static  bool    saveKtxInternal(const FEImage& image,const char* fileName,bool isKtx2,int quality)
    {
        const auto&     cInfo   =   image.cInfo();
        const uint32    channel =   FEFormatHelper::channelOf(cInfo._format);
        const uint32    levels  =   cInfo._levels > 0 ? cInfo._levels : 1;
        const uint32    W       =   cInfo._extent.x;
        const uint32    H       =   cInfo._extent.y;
        const bool      bBlock  =   FEFormatHelper::isBlockFormat(cInfo._format);
        /// 块压缩只走 ktx2 透传(ktx1 需逐格式 GL 压缩枚举,超出本实现范围)
        if  (bBlock && !isKtx2)
            return  false;

        /// 通道 → GL internalformat / baseInternalformat / format(ktx1 未压缩用)
        uint32  glInternal  =   0;
        uint32  glBase      =   0;
        uint32  glFmt       =   0;
        if  (!bBlock && !isKtx2)
        {
            /// ktx1 只支持每通道 8bit 的未压缩格式(GL_UNSIGNED_BYTE)
            if  (channel == 0 || FEFormatHelper::sizeOf(cInfo._format) != channel)
                return  false;
            switch (channel)
            {
            case 1:  glInternal = 0x8229; glBase = 0x1903; glFmt = 0x1903; break;  /// R8 / GL_R8 / GL_RED
            case 2:  glInternal = 0x822B; glBase = 0x8227; glFmt = 0x8227; break;  /// RG8 / GL_RG8 / GL_RG
            case 3:  glInternal = 0x8051; glBase = 0x1907; glFmt = 0x1907; break;  /// RGB8 / GL_RGB8 / GL_RGB
            case 4:  glInternal = 0x8058; glBase = 0x1908; glFmt = 0x1908; break;  /// RGBA8 / GL_RGBA8 / GL_RGBA
            default:  return  false;
            }
        }
        /// ktx2:格式 → vkFormat(未压缩 8/16bit UNORM + 全部块压缩);不支持即拒绝
        const uint32    vkFmt   =   isKtx2 ? vkFormatOf(cInfo._format) : 0u;
        if  (isKtx2 && vkFmt == 0u)
            return  false;
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

        /// 块感知的紧凑行重排参数(未压缩时块粒度退化为 1x1)
        const uint2     be      =   FEFormatHelper::blockExtent(cInfo._format);
        const uint32    eltByte =   FEFormatHelper::sizeOf(cInfo._format);
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
            /// 块格式:行数 = ceil(h/块高),行字节 = ceil(w/块宽) * 每块字节
            const uint32    rows    =   (h + be.y - 1) / be.y;
            const size_t    rowByte =   size_t((w + be.x - 1) / be.x) * eltByte;
            std::vector<uint8>  tight(rowByte * rows);
            for (uint32 y = 0; y < rows; ++y)
                std::memcpy(tight.data() + size_t(y) * rowByte,plane.dataOffset<uint8>(y,0),rowByte);
            ok  =   ktxTexture_SetImageFromMemory(ktex,l,0,0,tight.data(),tight.size()) == KTX_SUCCESS;
        }

        /// ktx2 + quality>0 且未压缩:尝试 ETC1S 压缩(quality 0~255,128=中等);
        /// 失败(如格式不支持 Basis)静默降级为原格式容器;块压缩数据不再压缩
        if  (ok && isKtx2 && quality > 0 && !bBlock)
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
        const uint32    width   =   cInfo._extent.x;
        const uint32    height  =   cInfo._extent.y;
        if (width == 0 || height == 0)
            return  false;

        const String    ext     =   extensionOf(fileName);

        /// KTX/KTX2:经 libktx 写容器(须在 8bit 校验之前路由:
        /// 块压缩格式 channelOf 为语义通道而 sizeOf 为块字节,过不了该校验)
        if  (ext == "ktx" || ext == "ktx2")
            return  (ext == "ktx2") ? saveKtxInternal(image,fileName,true,quality)
                                    : saveKtxInternal(image,fileName,false,quality);

        /// png/jpg/bmp/tga:只支持每通道 8bit 的数据
        if (channel == 0 || pixelByte != channel)
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

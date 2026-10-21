#pragma     once
// ============================================================================
// FEBlockEncoder - 未压缩 8bit 像素 → 块压缩格式的 CPU 编码器
//
// 仅由 FEImageHelper.cpp 包含(实现宏在此展开;依赖包含者先行提供 FEFormat)。
// 直编目标(stb_dxt + 自研 BC2 alpha),共 10 种:
// - BC1_RGB / BC1_RGBA / BC2 / BC3 : stb_dxt 颜色块;
//   BC2 的显式 4bit alpha 为自研打包(stb 的 alpha 路径是 BC3 式端点+索引,
//   与 BC2 的显式 nibble 布局不同,不可复用)
// - BC4 / BC5 : stb_compress_bc4/bc5_block(端点+3bit 索引,raster 行序)
// - 对应 sRGB 变体:块字节与 UNORM 完全相同,编码复用 UNORM 分支
//
// stb_dxt 输出保证 c0 >= c1(stb__CompressColorBlock 内部 swap):
// c0 == c1 的单色块索引仅取 {0,1},BC2/BC3 解码无三色模式歧义,无需修正。
// quality >= 160 → STB_DXT_HIGHQUAL(双轮细化,约慢 30~40%)。
// ============================================================================

#define     STB_DXT_IMPLEMENTATION
#include    "../../external/stb/stb_dxt.h"

namespace   FE
{
    namespace   BlockEncoderImpl
    {
        // ------------------------------------------------------------------
        // 直编支持表
        // ------------------------------------------------------------------
        /// <summary>
        /// 是否支持直编(非 Basis 间接路线);sRGB 变体与 UNORM 同字节
        /// </summary>
        inline  bool    isEncodeSupported(FEFormat fmt)
        {
            switch (fmt)
            {
            case FMT_BC1_RGB_UNORM:
            case FMT_BC1_RGB_SRGB:
            case FMT_BC1_RGBA_UNORM:
            case FMT_BC1_RGBA_SRGB:
            case FMT_BC2_UNORM:
            case FMT_BC2_SRGB:
            case FMT_BC3_UNORM:
            case FMT_BC3_SRGB:
            case FMT_BC4_UNORM:
            case FMT_BC5_UNORM:
                return  true;
            default:
                return  false;
            }
        }

        // ------------------------------------------------------------------
        // BC2 显式 4bit alpha(自研):
        // 16 字节 = 16 像素 × 4bit,行主序 nibble 流(i = y*4 + x),
        // 偶数 nibble 在字节低位(LE);解码侧 bcdec__sharp_alpha_block 按行
        // 以 u16(LE) 读出,两侧一致。量化 (a+8)/17 四舍五入(解码端 ×17 还原)
        // ------------------------------------------------------------------
        inline  void    encodeBC2Alpha(const uint8_t* a,uint8_t* out)
        {
            for (int i = 0; i < 16; ++i)
            {
                /// a 指向块首像素的 alpha 字节(src+3),按像素步进 i*4
                const uint8_t   v   =   (uint8_t)((a[i * 4] + 8) / 17) & 0xF;
                const int       bi  =   i >> 1;
                /// 顺序写:i 递增,偶数 nibble 整字节重写(清高位),
                /// 奇数 nibble 合并到已清零字节;不能对调用方缓冲做保留式读改写
                if ((i & 1) == 0)   out[bi] =   v;
                else                out[bi] =   (uint8_t)(out[bi] | (v << 4));
            }
        }

        // ------------------------------------------------------------------
        // 单块直编:src 为 gather 好的 4x4 完整 RGBA8 像素块(raster 行序,64B);
        // BC4/BC5 在此内部抽取 R / RG 平面(stb 期望 1B/px 与 2B/px 输入)
        // out:调用方提供 sizeOf(dstFmt) 字节的块缓冲
        // ------------------------------------------------------------------
        inline  void    encodeBlock(FEFormat dstFmt,const uint8_t* src,uint8_t* out,int quality)
        {
            const int   mode    =   (quality >= 160) ? STB_DXT_HIGHQUAL : STB_DXT_NORMAL;
            switch (dstFmt)
            {
            case FMT_BC1_RGB_UNORM:
            case FMT_BC1_RGB_SRGB:
            case FMT_BC1_RGBA_UNORM:
            case FMT_BC1_RGBA_SRGB:
                /// RGBA 变体仅是解码端语义差异,颜色块编码与 RGB 相同
                stb_compress_dxt_block(out,src,0,mode);
                break;
            case FMT_BC2_UNORM:
            case FMT_BC2_SRGB:
                /// 布局:[8B 显式 alpha][8B 颜色];颜色按无 alpha 压缩
                stb_compress_dxt_block(out + 8,src,0,mode);
                encodeBC2Alpha(src + 3,out);
                break;
            case FMT_BC3_UNORM:
            case FMT_BC3_SRGB:
                /// 布局:[8B 端点+索引 alpha][8B 颜色];stb alpha 路径即 BC3 式
                stb_compress_dxt_block(out,src,1,mode);
                break;
            case FMT_BC4_UNORM:
                {
                    uint8_t r[16];
                    for (int i = 0; i < 16; ++i)    r[i]    =   src[i * 4 + 0];
                    stb_compress_bc4_block(out,r);
                }
                break;
            case FMT_BC5_UNORM:
                {
                    uint8_t rg[32];
                    for (int i = 0; i < 16; ++i)
                    {
                        rg[i * 2 + 0]   =   src[i * 4 + 0];
                        rg[i * 2 + 1]   =   src[i * 4 + 1];
                    }
                    stb_compress_bc5_block(out,rg);
                }
                break;
            default:
                break;
            }
        }
    }
}

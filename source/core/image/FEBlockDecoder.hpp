#pragma     once
// ============================================================================
// FEBlockDecoder - 块压缩格式 → 未压缩格式的 CPU 解码器
//
// 仅由 FEImageHelper.cpp 包含(实现宏在此展开)。
// 支持全部 30 种块格式:
// - BC1~BC7   : bcdec.h (https://github.com/iOrange/bcdec,公有领域/MIT)
// - ETC2/EAC  : 自研(按 Khronos ETC2 规范)
// - ASTC 4x4/6x6/8x8 LDR : 自研(按 ASTC 规范,关键位排布以 astcenc
//   权威实现交叉验证,本地参考: external/ktx/external/astc-encoder)
//
// 解码管线:每个块解码到 float4(stride 4),经 FEFormatHelper::packFloatsCore
// 打包为宿主格式逐 texel 散写;BC4/BC5 为无损字节路径直接输出。
// HDR 块(BC6H 除外)在 LDR 目标下按规范解为 error color(不透明洋红)。
// ============================================================================

#include    <cstring>
#include    <cstdint>

#define     BCDEC_IMPLEMENTATION
#define     BCDEC_BC4BC5_PRECISE
#include    "../../external/bcdec/bcdec.h"

namespace   FE
{
    namespace   BlockDecoderImpl
    {
        // ------------------------------------------------------------------
        // 小工具
        // ------------------------------------------------------------------
        inline  float   halfToFloat(uint16_t h)
        {
            uint32_t    sign    =   (h >> 15) & 0x1;
            uint32_t    exp     =   (h >> 10) & 0x1F;
            uint32_t    man     =   h & 0x3FF;
            uint32_t    f;
            if (exp == 0)
            {
                if (man == 0)   { f = sign << 31; }
                else
                {
                    // 非规格化
                    exp = 127 - 15 + 1;
                    while (!(man & 0x400)) { man <<= 1; exp--; }
                    man &= 0x3FF;
                    f = (sign << 31) | (exp << 23) | (man << 13);
                }
            }
            else if (exp == 0x1F)
            {
                f = (sign << 31) | 0x7F800000u | (man << 13);
            }
            else
            {
                f = (sign << 31) | ((exp - 15 + 127) << 23) | (man << 13);
            }
            float   out;
            std::memcpy(&out, &f, 4);
            return  out;
        }

        inline  int     clampi(int v,int lo,int hi)
        {
            return  v < lo ? lo : (v > hi ? hi : v);
        }

        /// 64bit 大端块读取(q0 为最高有效字节,ETC2 家族)
        inline  uint64_t    etc2Block64(const uint8_t* p)
        {
            uint64_t    v   =   0;
            for (int i = 0; i < 8; ++i) { v = (v << 8) | p[i]; }
            return  v;
        }

        // ------------------------------------------------------------------
        // BC 家族(bcdec):每块解码为原始字节输出
        //   BC1/BC2/BC3/BC7 -> RGBA8 4B/px (byte0=R)
        //   BC4 -> 1B/px    BC5 -> 2B/px
        //   BC6H -> 3x float/px
        // out:调用方提供 4x4 块的输出缓冲(行距 = outPitch 字节)
        // ------------------------------------------------------------------
        inline  void    decodeBC(FEFormat fmt,const uint8_t* block,uint8_t* out,int outPitch)
        {
            switch(fmt)
            {
            case FMT_BC1_RGB_UNORM: case FMT_BC1_RGB_SRGB:
            case FMT_BC1_RGBA_UNORM: case FMT_BC1_RGBA_SRGB:
                bcdec_bc1(block, out, outPitch);
                break;
            case FMT_BC2_UNORM: case FMT_BC2_SRGB:
                bcdec_bc2(block, out, outPitch);
                break;
            case FMT_BC3_UNORM: case FMT_BC3_SRGB:
                bcdec_bc3(block, out, outPitch);
                break;
            case FMT_BC4_UNORM:
                bcdec_bc4(block, out, outPitch, 0);
                break;
            case FMT_BC4_SNORM:
                bcdec_bc4(block, out, outPitch, 1);
                break;
            case FMT_BC5_UNORM:
                bcdec_bc5(block, out, outPitch, 0);
                break;
            case FMT_BC5_SNORM:
                bcdec_bc5(block, out, outPitch, 1);
                break;
            case FMT_BC7_UNORM: case FMT_BC7_SRGB:
                bcdec_bc7(block, out, outPitch);
                break;
            default:
                break;
            }
        }

        inline  void    decodeBC6H(FEFormat fmt,const uint8_t* block,float* out,int rowPx)
        {
            // destinationPitch 单位为 float(每像素 3 个 float)
            bcdec_bc6h_float(block, out, rowPx * 3, fmt == FMT_BC6H_SFLOAT);
        }

        // ------------------------------------------------------------------
        // ETC2 / EAC 家族(自研)
        // ------------------------------------------------------------------

        /// 4bit -> 8bit 扩展
        inline  int     etc2Extend4(int v)  { return  (v << 4) | v; }
        /// 5bit -> 8bit
        inline  int     etc2Extend5(int v)  { return  (v << 3) | (v >> 2); }
        /// 6bit -> 8bit
        inline  int     etc2Extend6(int v)  { return  (v << 2) | (v >> 4); }
        /// 7bit -> 8bit
        inline  int     etc2Extend7(int v)  { return  (v << 1) | (v >> 6); }

        /// diff 模式的 3bit 有符号偏移
        inline  int     etc2Extend3Delta(int v)
        {
            v   &=  7;
            return  (v & 4) ? (v - 8) : v;
        }

        /// ETC2 intensity modifier 表(-b,-a,+a,+b)
        inline  void    etc2Modifiers(int table,int& a,int& b)
        {
            static const int    tableA[8]   =   { 2, 5, 9,13,18,24,33,47 };
            static const int    tableB[8]   =   { 8,17,29,42,60,80,106,183 };
            a   =   tableA[table & 7];
            b   =   tableB[table & 7];
        }

        /// EAC 16 项 modifier 表
        inline  int     eacModifier(int table,int idx)
        {
            static const int    mod[16][8]  =
            {
                { -3, -6, -9,-15,  2,  5,  8, 14 },
                { -3, -7,-10,-13,  2,  6,  9, 12 },
                { -2, -5, -8,-13,  1,  4,  7, 12 },
                { -2, -4, -6,-13,  1,  3,  5, 12 },
                { -3, -6, -8,-12,  2,  5,  7, 11 },
                { -3, -7, -9,-11,  2,  6,  8, 10 },
                { -4, -7, -8,-11,  3,  6,  7, 10 },
                { -3, -5, -8,-11,  2,  4,  7, 10 },
                { -2, -6, -8,-10,  1,  5,  7,  9 },
                { -2, -5, -8,-10,  1,  4,  7,  9 },
                { -2, -4, -8,-10,  1,  3,  7,  9 },
                { -2, -5, -7,-10,  1,  4,  6,  9 },
                { -3, -4, -7,-10,  2,  3,  6,  9 },
                { -1, -2, -3,-10,  0,  1,  2,  9 },
                { -4, -6, -8, -9,  3,  5,  7,  8 },
                { -3, -5, -7, -9,  2,  4,  6,  8 },
            };
            return  mod[table & 15][idx & 7];
        }

        /// EAC 单通道块解码:返回 16 个值
        /// signed=false: [0,1]; signed=true: [-1,1]
        /// 纹理顺序:out[i] 为列主序(texel x = i/4, y = i%4),与选择器位流顺序一致
        inline  void    eacDecode(const uint8_t* block,bool signedOut,float* out)
        {
            int     base    =   (int8_t)block[0];
            if (!signedOut) { base = block[0]; }
            int     mul     =   (block[1] >> 4) & 0xF;
            int     table   =   block[1] & 0xF;
            uint64_t    idxBits =   0;
            for (int i = 1; i < 8; ++i) { idxBits = (idxBits << 8) | block[i]; }

            for (int i = 0; i < 16; ++i)
            {
                int     idx =   (int)((idxBits >> (45 - 3 * i)) & 7);
                int     mod =   eacModifier(table, idx);
                int     v;
                if (mul == 0)
                {
                    v   =   signedOut ? clampi(base * 8 + mod, -1023, 1023)
                                      : clampi(base * 8 + 4 + mod, 0, 2047);
                }
                else
                {
                    v   =   signedOut ? clampi(base * 8 + mod * mul * 8, -1023, 1023)
                                      : clampi(base * 8 + 4 + mod * mul * 8, 0, 2047);
                }
                out[i]  =   signedOut ? (v / 1023.0f) : (v / 2047.0f);
            }
        }

        /// ETC2 color 64bit 块(含 punchthrough)解码,输出 16 组 RGBA8
        /// punchthrough:bit33 为 Op 位,alpha 语义由调用方模式决定
        /// 返回 false 表示非法块(punchthrough planar 等),调用方应输出 error color
        inline  bool    etc2ColorDecode(const uint8_t* block,bool punchthrough,uint8_t* rgba16)
        {
            uint64_t    q   =   etc2Block64(block);
            bool        opaque  =   !punchthrough || ((q >> 33) & 1) != 0;

            uint8_t (*px)[4]    =   (uint8_t(*)[4])rgba16;

            // 差分模式检测位:punchthrough 无 individual 模式,恒按 differential 家族解析
            bool    diffBit =   punchthrough || ((q >> 33) & 1) != 0;

            // individual / differential 检测:R+Rd / G+Gd / B+Bd 越界则 T/H/planar
            int     R   =   (int)((q >> 59) & 0x1F);
            int     Rd  =   (int)((q >> 56) & 0x07);
            int     G   =   (int)((q >> 51) & 0x1F);
            int     Gd  =   (int)((q >> 48) & 0x07);
            int     B   =   (int)((q >> 43) & 0x1F);
            int     Bd  =   (int)((q >> 40) & 0x07);

            // T/H/planar 只可能出现在 differential 编码分支(非 individual)
            if (diffBit && (R + Rd < 0 || R + Rd > 31))
            {
                // ---- T 模式 ----
                int r1  =   (int)((((q >> 59) & 0x03) << 2) | ((q >> 56) & 0x03));
                int g1  =   (int)((q >> 52) & 0x0F);
                int b1  =   (int)((q >> 48) & 0x0F);
                int r2  =   (int)((q >> 44) & 0x0F);
                int g2  =   (int)((q >> 40) & 0x0F);
                int b2  =   (int)((q >> 36) & 0x0F);
                int da  =   (int)((q >> 34) & 0x03);
                int db  =   (int)((q >> 32) & 0x01);
                int distIdx =   (da << 1) | db;
                static const int    dist[8] =   { 3, 6, 11, 16, 23, 32, 41, 64 };
                int     d   =   dist[distIdx];
                // pc0=base1, pc1=base2+d, pc2=base2, pc3=base2-d;2bit 索引恒等映射
                int     c[4][4] =
                {
                    { etc2Extend4(r1), etc2Extend4(g1), etc2Extend4(b1), 255 },
                    { clampi(etc2Extend4(r2) + d, 0, 255), clampi(etc2Extend4(g2) + d, 0, 255), clampi(etc2Extend4(b2) + d, 0, 255), 255 },
                    { etc2Extend4(r2), etc2Extend4(g2), etc2Extend4(b2), 255 },
                    { clampi(etc2Extend4(r2) - d, 0, 255), clampi(etc2Extend4(g2) - d, 0, 255), clampi(etc2Extend4(b2) - d, 0, 255), 255 },
                };
                for (int i = 0; i < 16; ++i)
                {
                    int msb =   (int)((q >> (16 + i)) & 1);
                    int lsb =   (int)((q >> i) & 1);
                    int s   =   (msb << 1) | lsb;
                    if (!opaque && msb == 1 && lsb == 0)
                    {
                        px[i][0] = px[i][1] = px[i][2] = px[i][3] = 0;
                        continue;
                    }
                    std::memcpy(px[i], c[s], 3);
                    px[i][3]    =   255;
                }
                return  true;
            }
            if (diffBit && (G + Gd < 0 || G + Gd > 31))
            {
                // ---- H 模式 ----
                int r1  =   (int)((q >> 59) & 0x0F);
                int g1  =   (int)((((q >> 56) & 0x07) << 1) | ((q >> 52) & 0x01));
                int b1  =   (int)((((q >> 51) & 0x01) << 3) | ((q >> 47) & 0x07));
                int r2  =   (int)((q >> 43) & 0x0F);
                int g2  =   (int)((q >> 39) & 0x0F);
                int b2  =   (int)((q >> 35) & 0x0F);
                int da  =   (int)((q >> 34) & 0x01);
                int db  =   (int)((q >> 32) & 0x01);
                int val1    =   (r1 << 8) | (g1 << 4) | b1;
                int val2    =   (r2 << 8) | (g2 << 4) | b2;
                int distIdx =   (da << 2) | (db << 1) | (val1 >= val2 ? 1 : 0);
                static const int    dist[8] =   { 3, 6, 11, 16, 23, 32, 41, 64 };
                int     d   =   dist[distIdx];
                // pc0=base1+d, pc1=base1-d, pc2=base2+d, pc3=base2-d;2bit 索引恒等映射
                int     c[4][4] =
                {
                    { clampi(etc2Extend4(r1) + d, 0, 255), clampi(etc2Extend4(g1) + d, 0, 255), clampi(etc2Extend4(b1) + d, 0, 255), 255 },
                    { clampi(etc2Extend4(r1) - d, 0, 255), clampi(etc2Extend4(g1) - d, 0, 255), clampi(etc2Extend4(b1) - d, 0, 255), 255 },
                    { clampi(etc2Extend4(r2) + d, 0, 255), clampi(etc2Extend4(g2) + d, 0, 255), clampi(etc2Extend4(b2) + d, 0, 255), 255 },
                    { clampi(etc2Extend4(r2) - d, 0, 255), clampi(etc2Extend4(g2) - d, 0, 255), clampi(etc2Extend4(b2) - d, 0, 255), 255 },
                };
                for (int i = 0; i < 16; ++i)
                {
                    int msb =   (int)((q >> (16 + i)) & 1);
                    int lsb =   (int)((q >> i) & 1);
                    int s   =   (msb << 1) | lsb;
                    if (!opaque && msb == 1 && lsb == 0)
                    {
                        px[i][0] = px[i][1] = px[i][2] = px[i][3] = 0;
                        continue;
                    }
                    std::memcpy(px[i], c[s], 3);
                    px[i][3]    =   255;
                }
                return  true;
            }
            if (diffBit && (B + Bd < 0 || B + Bd > 31))
            {
                // ---- planar 模式(punchthrough 下 Op=0 为非法块) ----
                if (punchthrough && !opaque)
                {
                    return  false;
                }
                int ro  =   (int)((q >> 57) & 0x3F);
                int go  =   (int)((((q >> 56) & 0x01) << 6) | ((q >> 49) & 0x3F));
                int bo  =   (int)((((q >> 48) & 0x01) << 5) | (((q >> 43) & 0x03) << 3) | ((q >> 39) & 0x07));
                int rh  =   (int)((((q >> 34) & 0x1F) << 1) | ((q >> 32) & 0x01));
                int gh  =   (int)((q >> 25) & 0x7F);
                int bh  =   (int)((q >> 19) & 0x3F);
                int rv  =   (int)((q >> 13) & 0x3F);
                int gv  =   (int)((q >> 6) & 0x7F);
                int bv  =   (int)(q & 0x3F);

                int C[3]    =   { etc2Extend6(ro), etc2Extend7(go), etc2Extend6(bo) };
                int Ch[3]   =   { etc2Extend6(rh), etc2Extend7(gh), etc2Extend6(bh) };
                int Cv[3]   =   { etc2Extend6(rv), etc2Extend7(gv), etc2Extend6(bv) };

                for (int y = 0; y < 4; ++y)
                {
                    for (int x = 0; x < 4; ++x)
                    {
                        uint8_t*    p   =   px[y * 4 + x];
                        for (int ch = 0; ch < 3; ++ch)
                        {
                            int v   =   (x * (Ch[ch] - C[ch]) + y * (Cv[ch] - C[ch]) + 4 * C[ch] + 2) >> 2;
                            p[ch]   =   (uint8_t)clampi(v, 0, 255);
                        }
                        p[3]    =   255;
                    }
                }
                return  true;
            }

            // ---- individual / differential 模式 ----
            int     base[2][3];
            int     tableIdx[2];
            int     flipBit =   (int)((q >> 32) & 1);

            if (!diffBit)
            {
                // individual:两半 4bit
                int r1  =   (int)((q >> 60) & 0x0F);
                int g1  =   (int)((q >> 52) & 0x0F);
                int b1  =   (int)((q >> 44) & 0x0F);
                base[0][0]  =   etc2Extend4(r1);
                base[0][1]  =   etc2Extend4(g1);
                base[0][2]  =   etc2Extend4(b1);
                base[1][0]  =   etc2Extend4((int)((q >> 56) & 0x0F));
                base[1][1]  =   etc2Extend4((int)((q >> 48) & 0x0F));
                base[1][2]  =   etc2Extend4((int)((q >> 40) & 0x0F));
                tableIdx[0] =   (int)((q >> 37) & 0x07);
                tableIdx[1] =   (int)((q >> 34) & 0x07);
            }
            else
            {
                // differential:基准 5bit + 3bit delta
                base[0][0]  =   etc2Extend5(R);
                base[0][1]  =   etc2Extend5(G);
                base[0][2]  =   etc2Extend5(B);
                base[1][0]  =   clampi(etc2Extend5(R) + etc2Extend3Delta(Rd) * 8, 0, 255);
                base[1][1]  =   clampi(etc2Extend5(G) + etc2Extend3Delta(Gd) * 8, 0, 255);
                base[1][2]  =   clampi(etc2Extend5(B) + etc2Extend3Delta(Bd) * 8, 0, 255);
                tableIdx[0] =   (int)((q >> 37) & 0x07);
                tableIdx[1] =   (int)((q >> 34) & 0x07);
            }

            int     a1, b1, a2, b2;
            etc2Modifiers(tableIdx[0], a1, b1);
            etc2Modifiers(tableIdx[1], a2, b2);
            int     modTab[2][4]    =   { { -b1, -a1, a1, b1 }, { -b2, -a2, a2, b2 } };
            if (opaque)
            {
                modTab[0][0] = -b1; modTab[0][1] = -a1; modTab[0][2] = a1; modTab[0][3] = b1;
                modTab[1][0] = -b2; modTab[1][1] = -a2; modTab[1][2] = a2; modTab[1][3] = b2;
            }
            else
            {
                // punchthrough 非透明:修饰符表为 {-b, 0, 0, +b}
                modTab[0][0] = -b1; modTab[0][1] = 0; modTab[0][2] = 0; modTab[0][3] = b1;
                modTab[1][0] = -b2; modTab[1][1] = 0; modTab[1][2] = 0; modTab[1][3] = b2;
            }

            for (int i = 0; i < 16; ++i)
            {
                int x   =   i & 3;
                int y   =   i >> 2;
                int sub =   flipBit ? (y < 2 ? 0 : 1) : (x < 2 ? 0 : 1);
                int msb =   (int)((q >> (16 + i)) & 1);
                int lsb =   (int)((q >> i) & 1);
                int s   =   (msb << 1) | lsb;

                if (!opaque && msb == 1 && lsb == 0)
                {
                    px[i][0] = px[i][1] = px[i][2] = px[i][3] = 0;
                    continue;
                }
                int     mod =   modTab[sub][s];
                px[i][0]    =   (uint8_t)clampi(base[sub][0] + mod, 0, 255);
                px[i][1]    =   (uint8_t)clampi(base[sub][1] + mod, 0, 255);
                px[i][2]    =   (uint8_t)clampi(base[sub][2] + mod, 0, 255);
                px[i][3]    =   255;
            }
            return  true;
        }

        // ==================================================================
        // ASTC LDR 家族(自研,位排布经 astcenc 权威实现交叉验证)
        // ==================================================================

        /// ISE: quint 三元组查表
        inline  const uint8_t*  astcQuintsOfPacked(int packed)
        {
            static const uint8_t    TBL[128][3] =
            {
                {0,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0},{0,4,0},{4,4,0},{4,4,4},
                {0,1,0},{1,1,0},{2,1,0},{3,1,0},{4,1,0},{1,4,0},{4,4,1},{4,4,4},
                {0,2,0},{1,2,0},{2,2,0},{3,2,0},{4,2,0},{2,4,0},{4,4,2},{4,4,4},
                {0,3,0},{1,3,0},{2,3,0},{3,3,0},{4,3,0},{3,4,0},{4,4,3},{4,4,4},
                {0,0,1},{1,0,1},{2,0,1},{3,0,1},{4,0,1},{0,4,1},{4,0,4},{0,4,4},
                {0,1,1},{1,1,1},{2,1,1},{3,1,1},{4,1,1},{1,4,1},{4,1,4},{1,4,4},
                {0,2,1},{1,2,1},{2,2,1},{3,2,1},{4,2,1},{2,4,1},{4,2,4},{2,4,4},
                {0,3,1},{1,3,1},{2,3,1},{3,3,1},{4,3,1},{3,4,1},{4,3,4},{3,4,4},
                {0,0,2},{1,0,2},{2,0,2},{3,0,2},{4,0,2},{0,4,2},{2,0,4},{3,0,4},
                {0,1,2},{1,1,2},{2,1,2},{3,1,2},{4,1,2},{1,4,2},{2,1,4},{3,1,4},
                {0,2,2},{1,2,2},{2,2,2},{3,2,2},{4,2,2},{2,4,2},{2,2,4},{3,2,4},
                {0,3,2},{1,3,2},{2,3,2},{3,3,2},{4,3,2},{3,4,2},{2,3,4},{3,3,4},
                {0,0,3},{1,0,3},{2,0,3},{3,0,3},{4,0,3},{0,4,3},{0,0,4},{1,0,4},
                {0,1,3},{1,1,3},{2,1,3},{3,1,3},{4,1,3},{1,4,3},{0,1,4},{1,1,4},
                {0,2,3},{1,2,3},{2,2,3},{3,2,3},{4,2,3},{2,4,3},{0,2,4},{1,2,4},
                {0,3,3},{1,3,3},{2,3,3},{3,3,3},{4,3,3},{3,4,3},{0,3,4},{1,3,4}
            };
            return  TBL[packed & 127];
        }

        /// ISE: trit 五元组查表
        inline  const uint8_t*  astcTritsOfPacked(int packed)
        {
            static const uint8_t    TBL[256][5] =
            {
                {0,0,0,0,0},{1,0,0,0,0},{2,0,0,0,0},{0,0,2,0,0},{0,1,0,0,0},{1,1,0,0,0},{2,1,0,0,0},{1,0,2,0,0},
                {0,2,0,0,0},{1,2,0,0,0},{2,2,0,0,0},{2,0,2,0,0},{0,2,2,0,0},{1,2,2,0,0},{2,2,2,0,0},{2,0,2,0,0},
                {0,0,1,0,0},{1,0,1,0,0},{2,0,1,0,0},{0,1,2,0,0},{0,1,1,0,0},{1,1,1,0,0},{2,1,1,0,0},{1,1,2,0,0},
                {0,2,1,0,0},{1,2,1,0,0},{2,2,1,0,0},{2,1,2,0,0},{0,0,0,2,2},{1,0,0,2,2},{2,0,0,2,2},{0,0,2,2,2},
                {0,0,0,1,0},{1,0,0,1,0},{2,0,0,1,0},{0,0,2,1,0},{0,1,0,1,0},{1,1,0,1,0},{2,1,0,1,0},{1,0,2,1,0},
                {0,2,0,1,0},{1,2,0,1,0},{2,2,0,1,0},{2,0,2,1,0},{0,2,2,1,0},{1,2,2,1,0},{2,2,2,1,0},{2,0,2,1,0},
                {0,0,1,1,0},{1,0,1,1,0},{2,0,1,1,0},{0,1,2,1,0},{0,1,1,1,0},{1,1,1,1,0},{2,1,1,1,0},{1,1,2,1,0},
                {0,2,1,1,0},{1,2,1,1,0},{2,2,1,1,0},{2,1,2,1,0},{0,1,0,2,2},{1,1,0,2,2},{2,1,0,2,2},{1,0,2,2,2},
                {0,0,0,2,0},{1,0,0,2,0},{2,0,0,2,0},{0,0,2,2,0},{0,1,0,2,0},{1,1,0,2,0},{2,1,0,2,0},{1,0,2,2,0},
                {0,2,0,2,0},{1,2,0,2,0},{2,2,0,2,0},{2,0,2,2,0},{0,2,2,2,0},{1,2,2,2,0},{2,2,2,2,0},{2,0,2,2,0},
                {0,0,1,2,0},{1,0,1,2,0},{2,0,1,2,0},{0,1,2,2,0},{0,1,1,2,0},{1,1,1,2,0},{2,1,1,2,0},{1,1,2,2,0},
                {0,2,1,2,0},{1,2,1,2,0},{2,2,1,2,0},{2,1,2,2,0},{0,2,0,2,2},{1,2,0,2,2},{2,2,0,2,2},{2,0,2,2,2},
                {0,0,0,0,2},{1,0,0,0,2},{2,0,0,0,2},{0,0,2,0,2},{0,1,0,0,2},{1,1,0,0,2},{2,1,0,0,2},{1,0,2,0,2},
                {0,2,0,0,2},{1,2,0,0,2},{2,2,0,0,2},{2,0,2,0,2},{0,2,2,0,2},{1,2,2,0,2},{2,2,2,0,2},{2,0,2,0,2},
                {0,0,1,0,2},{1,0,1,0,2},{2,0,1,0,2},{0,1,2,0,2},{0,1,1,0,2},{1,1,1,0,2},{2,1,1,0,2},{1,1,2,0,2},
                {0,2,1,0,2},{1,2,1,0,2},{2,2,1,0,2},{2,1,2,0,2},{0,2,2,2,2},{1,2,2,2,2},{2,2,2,2,2},{2,0,2,2,2},
                {0,0,0,0,1},{1,0,0,0,1},{2,0,0,0,1},{0,0,2,0,1},{0,1,0,0,1},{1,1,0,0,1},{2,1,0,0,1},{1,0,2,0,1},
                {0,2,0,0,1},{1,2,0,0,1},{2,2,0,0,1},{2,0,2,0,1},{0,2,2,0,1},{1,2,2,0,1},{2,2,2,0,1},{2,0,2,0,1},
                {0,0,1,0,1},{1,0,1,0,1},{2,0,1,0,1},{0,1,2,0,1},{0,1,1,0,1},{1,1,1,0,1},{2,1,1,0,1},{1,1,2,0,1},
                {0,2,1,0,1},{1,2,1,0,1},{2,2,1,0,1},{2,1,2,0,1},{0,0,1,2,2},{1,0,1,2,2},{2,0,1,2,2},{0,1,2,2,2},
                {0,0,0,1,1},{1,0,0,1,1},{2,0,0,1,1},{0,0,2,1,1},{0,1,0,1,1},{1,1,0,1,1},{2,1,0,1,1},{1,0,2,1,1},
                {0,2,0,1,1},{1,2,0,1,1},{2,2,0,1,1},{2,0,2,1,1},{0,2,2,1,1},{1,2,2,1,1},{2,2,2,1,1},{2,0,2,1,1},
                {0,0,1,1,1},{1,0,1,1,1},{2,0,1,1,1},{0,1,2,1,1},{0,1,1,1,1},{1,1,1,1,1},{2,1,1,1,1},{1,1,2,1,1},
                {0,2,1,1,1},{1,2,1,1,1},{2,2,1,1,1},{2,1,2,1,1},{0,1,1,2,2},{1,1,1,2,2},{2,1,1,2,2},{1,1,2,2,2},
                {0,0,0,2,1},{1,0,0,2,1},{2,0,0,2,1},{0,0,2,2,1},{0,1,0,2,1},{1,1,0,2,1},{2,1,0,2,1},{1,0,2,2,1},
                {0,2,0,2,1},{1,2,0,2,1},{2,2,0,2,1},{2,0,2,2,1},{0,2,2,2,1},{1,2,2,2,1},{2,2,2,2,1},{2,0,2,2,1},
                {0,0,1,2,1},{1,0,1,2,1},{2,0,1,2,1},{0,1,2,2,1},{0,1,1,2,1},{1,1,1,2,1},{2,1,1,2,1},{1,1,2,2,1},
                {0,2,1,2,1},{1,2,1,2,1},{2,2,1,2,1},{2,1,2,2,1},{0,2,1,2,2},{1,2,1,2,2},{2,2,1,2,2},{2,1,2,2,2},
                {0,0,0,1,2},{1,0,0,1,2},{2,0,0,1,2},{0,0,2,1,2},{0,1,0,1,2},{1,1,0,1,2},{2,1,0,1,2},{1,0,2,1,2},
                {0,2,0,1,2},{1,2,0,1,2},{2,2,0,1,2},{2,0,2,1,2},{0,2,2,1,2},{1,2,2,1,2},{2,2,2,1,2},{2,0,2,1,2},
                {0,0,1,1,2},{1,0,1,1,2},{2,0,1,1,2},{0,1,2,1,2},{0,1,1,1,2},{1,1,1,1,2},{2,1,1,1,2},{1,1,2,1,2},
                {0,2,1,1,2},{1,2,1,1,2},{2,2,1,1,2},{2,1,2,1,2},{0,2,2,2,2},{1,2,2,2,2},{2,2,2,2,2},{2,1,2,2,2}
            };
            return  TBL[packed & 255];
        }

        /// ASTC 量化级别: 0..20 = QUANT_2..QUANT_256
        /// 返回 {每值 bit 数, 含 trit, 含 quint}
        inline  void    astcBtqOfLevel(int level,int& bits,bool& trits,bool& quints)
        {
            static const int    BITS[21]    =   { 1,0,2,0,1,3,1,2,4,2,3,5,3,4,6,4,5,7,5,6,8 };
            bits    =   BITS[level];
            trits   =   (level == 1 || level == 4 || level == 7 || level == 10 || level == 13 || level == 16 || level == 19);
            quints  =   (level == 3 || level == 6 || level == 9 || level == 12 || level == 15 || level == 18);
        }

        /// ISE 序列总位数
        inline  int     astcIseBitCount(int count,int level)
        {
            int     bits; bool trits, quints;
            astcBtqOfLevel(level, bits, trits, quints);
            if (trits)      { return  count * bits + (8 * count + 4) / 5; }
            if (quints)     { return  count * bits + (7 * count + 2) / 3; }
            return  count * bits;
        }

        /// little-endian 位流读取
        inline  int     astcReadBits(const uint8_t* data,int count,int offset)
        {
            int     v   =   0;
            for (int i = 0; i < count; ++i)
            {
                int     bit     =   offset + i;
                v       |=  ((data[bit >> 3] >> (bit & 7)) & 1) << i;
            }
            return  v;
        }

        /// 整数序列解码(ISE):从 bitOffset 起解 count 个值
        inline  void    astcDecodeIse(int level,int count,const uint8_t* data,uint8_t* out,int bitOffset)
        {
            int     bits; bool trits, quints;
            astcBtqOfLevel(level, bits, trits, quints);

            uint8_t     tqBlocks[22]    =   { 0 };
            uint8_t     results[68]     =   { 0 };
            int     lcounter    =   0;
            int     hcounter    =   0;

            for (int i = 0; i < count; ++i)
            {
                results[i]  =   (uint8_t)astcReadBits(data, bits, bitOffset);
                bitOffset   +=  bits;
                if (trits)
                {
                    static const uint8_t    toRead[5]   =   { 2, 2, 1, 2, 1 };
                    static const uint8_t    shift[5]    =   { 0, 2, 4, 5, 7 };
                    static const uint8_t    nextL[5]    =   { 1, 2, 3, 4, 0 };
                    static const uint8_t    hIncr[5]    =   { 0, 0, 0, 0, 1 };
                    int     t   =   astcReadBits(data, toRead[lcounter], bitOffset);
                    bitOffset   +=  toRead[lcounter];
                    tqBlocks[hcounter]  |=  (uint8_t)(t << shift[lcounter]);
                    hcounter    +=  hIncr[lcounter];
                    lcounter    =   nextL[lcounter];
                }
                if (quints)
                {
                    static const uint8_t    toRead[3]   =   { 3, 2, 2 };
                    static const uint8_t    shift[3]    =   { 0, 3, 5 };
                    static const uint8_t    nextL[3]    =   { 1, 2, 0 };
                    static const uint8_t    hIncr[3]    =   { 0, 0, 1 };
                    int     t   =   astcReadBits(data, toRead[lcounter], bitOffset);
                    bitOffset   +=  toRead[lcounter];
                    tqBlocks[hcounter]  |=  (uint8_t)(t << shift[lcounter]);
                    hcounter    +=  hIncr[lcounter];
                    lcounter    =   nextL[lcounter];
                }
            }
            if (trits)
            {
                int     blocks  =   (count + 4) / 5;
                for (int i = 0; i < blocks; ++i)
                {
                    const uint8_t*  tr  =   astcTritsOfPacked(tqBlocks[i]);
                    for (int j = 0; j < 5; ++j) { results[5 * i + j] |= (uint8_t)(tr[j] << bits); }
                }
            }
            if (quints)
            {
                int     blocks  =   (count + 2) / 3;
                for (int i = 0; i < blocks; ++i)
                {
                    const uint8_t*  qu  =   astcQuintsOfPacked(tqBlocks[i]);
                    for (int j = 0; j < 3; ++j) { results[3 * i + j] |= (uint8_t)(qu[j] << bits); }
                }
            }
            for (int i = 0; i < count; ++i) { out[i] = results[i]; }
        }

        /// 字节内位反转(astcenc bitrev8)
        inline  uint8_t     astcBitRev8(uint8_t v)
        {
            v   =   (uint8_t)(((v & 0xF0) >> 4) | ((v & 0x0F) << 4));
            v   =   (uint8_t)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
            v   =   (uint8_t)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
            return  v;
        }

        /// 2D 块模式解码(astcenc decode_block_mode_2d)
        /// 返回 false 表示 void-extent/常色/非法
        inline  bool    astcBlockMode2d(int blockMode,int& xw,int& yw,bool& dual,int& quantLevel,int& weightBits)
        {
            if ((blockMode & 0x1FF) == 0x1FC) { return  false; }

            int     baseQuant   =   (blockMode >> 4) & 1;
            int     H   =   (blockMode >> 9) & 1;
            int     D   =   (blockMode >> 10) & 1;
            int     A   =   (blockMode >> 5) & 0x3;

            xw = yw = 0;
            if ((blockMode & 3) != 0)
            {
                baseQuant   |=  (blockMode & 3) << 1;
                int     B   =   (blockMode >> 7) & 3;
                switch ((blockMode >> 2) & 3)
                {
                case 0: xw = B + 4; yw = A + 2; break;
                case 1: xw = B + 8; yw = A + 2; break;
                case 2: xw = A + 2; yw = B + 8; break;
                case 3:
                    B   &=  1;
                    if (blockMode & 0x100)  { xw = B + 2; yw = A + 2; }
                    else                    { xw = A + 2; yw = B + 6; }
                    break;
                }
            }
            else
            {
                baseQuant   |=  ((blockMode >> 2) & 3) << 1;
                if (((blockMode >> 2) & 3) == 0) { return  false; }
                int     B   =   (blockMode >> 9) & 3;
                switch ((blockMode >> 7) & 3)
                {
                case 0: xw = 12; yw = A + 2; break;
                case 1: xw = A + 2; yw = 12; break;
                case 2: xw = A + 6; yw = B + 6; D = 0; H = 0; break;
                case 3:
                    switch ((blockMode >> 5) & 3)
                    {
                    case 0: xw = 6; yw = 10; break;
                    case 1: xw = 10; yw = 6; break;
                    default: return  false;
                    }
                    break;
                }
            }

            int     weightCount =   xw * yw * (D + 1);
            quantLevel    =   (baseQuant - 2) + 6 * H;
            dual    =   D != 0;
            weightBits = astcIseBitCount(weightCount, quantLevel);
            return  weightCount <= 64 && weightBits >= 24 && weightBits <= 96;
        }

        /// ASTC 分区 hash(astcenc hash52)
        inline  uint32_t    astcHash52(uint32_t inp)
        {
            inp ^=  inp >> 15;
            inp *=  0xEEDE0891u;
            inp ^=  inp >> 5;
            inp +=  inp << 16;
            inp ^=  inp >> 7;
            inp ^=  inp >> 3;
            inp ^=  inp << 6;
            inp ^=  inp >> 17;
            return  inp;
        }

        /// 分区选择(astcenc select_partition)
        inline  int     astcSelectPartition(int seed,int x,int y,int partitionCount,bool smallBlock)
        {
            if (smallBlock) { x <<= 1; y <<= 1; }
            seed    +=  (partitionCount - 1) * 1024;
            uint32_t    rnum    =   astcHash52((uint32_t)seed);

            uint32_t    s[13];
            s[1]    =   rnum & 0xF;         s[2]    =   (rnum >> 4) & 0xF;
            s[3]    =   (rnum >> 8) & 0xF;  s[4]    =   (rnum >> 12) & 0xF;
            s[5]    =   (rnum >> 16) & 0xF; s[6]    =   (rnum >> 20) & 0xF;
            s[7]    =   (rnum >> 24) & 0xF; s[8]    =   (rnum >> 28) & 0xF;
            s[9]    =   (rnum >> 18) & 0xF; s[10]   =   (rnum >> 22) & 0xF;
            s[11]   =   (rnum >> 26) & 0xF; s[12]   =   ((rnum >> 30) | (rnum << 2)) & 0xF;
            for (int i = 1; i <= 12; ++i) { s[i] *= s[i]; }

            int     sh1, sh2;
            if (seed & 1)
            {
                sh1 =   (seed & 2) ? 4 : 5;
                sh2 =   (partitionCount == 3) ? 6 : 5;
            }
            else
            {
                sh1 =   (partitionCount == 3) ? 6 : 5;
                sh2 =   (seed & 2) ? 4 : 5;
            }
            int     sh3 =   (seed & 0x10) ? sh1 : sh2;

            s[1] >>= sh1; s[2] >>= sh2; s[3] >>= sh1; s[4] >>= sh2;
            s[5] >>= sh1; s[6] >>= sh2; s[7] >>= sh1; s[8] >>= sh2;
            s[9] >>= sh3; s[10] >>= sh3; s[11] >>= sh3; s[12] >>= sh3;

            int     a   =   (int)(s[1] * x + s[2] * y) + (int)(rnum >> 14);
            int     b   =   (int)(s[3] * x + s[4] * y) + (int)(rnum >> 10);
            int     c   =   (int)(s[5] * x + s[6] * y) + (int)(rnum >> 6);
            int     d   =   (int)(s[7] * x + s[8] * y) + (int)(rnum >> 2);

            a &= 0x3F; b &= 0x3F; c &= 0x3F; d &= 0x3F;
            if (partitionCount <= 3) { d = 0; }
            if (partitionCount <= 2) { c = 0; }
            if (partitionCount <= 1) { b = 0; }

            if (a >= b && a >= c && a >= d) { return  0; }
            if (b >= c && b >= d)           { return  1; }
            if (c >= d)                     { return  2; }
            return  3;
        }

        /// 有符号位传递:hi 为 delta 高位,lo 为 base 低位
        inline  void    astcBitTransferSigned(int& hi,int& lo)
        {
            lo  >>=  1;
            lo  |=  hi & 0x80;
            hi  >>=  1;
            hi  &=  0x3F;
            if (hi & 0x20) { hi -= 0x40; }
        }

        /// 反蓝收缩(8bit 域,astcenc uncontract_color)
        inline  void    astcUncontract(int& r,int& g,int& b)
        {
            r   =   (r + b) >> 1;
            g   =   (g + b) >> 1;
        }

        /// LDR 端点解包(astcenc unpack_color_endpoints LDR 部分)
        /// vals: 反量化后的 0..255 端点整数;输出 ep0/ep1 RGBA8
        /// 返回 false 表示 HDR CEM(LDR 不支持)
        inline  bool    astcUnpackEndpointsLDR(int cem,const uint8_t* vals,int ep0[4],int ep1[4])
        {
            switch(cem)
            {
            case 0: // LDR luminance direct
                ep0[0]=ep0[1]=ep0[2]=vals[0]; ep0[3]=255;
                ep1[0]=ep1[1]=ep1[2]=vals[1]; ep1[3]=255;
                return  true;
            case 1: // LDR luminance base+scale
                {
                    int v0 = vals[0], v1 = vals[1];
                    int l0  =   (v0 >> 2) | (v1 & 0xC0);
                    int l1  =   l0 + (v1 & 0x3F);
                    l1  =   l1 > 255 ? 255 : l1;
                    ep0[0]=ep0[1]=ep0[2]=l0; ep0[3]=255;
                    ep1[0]=ep1[1]=ep1[2]=l1; ep1[3]=255;
                }
                return  true;
            case 4: // LDR luminance-alpha direct
                ep0[0]=ep0[1]=ep0[2]=vals[0]; ep0[3]=vals[2];
                ep1[0]=ep1[1]=ep1[2]=vals[1]; ep1[3]=vals[3];
                return  true;
            case 5: // LDR luminance-alpha base+scale(astcenc 8bit 特化式)
                {
                    int lum0 = vals[0], lum1 = vals[1], alp0 = vals[2], alp1 = vals[3];
                    lum0 |= (lum1 & 0x80) << 1;
                    alp0 |= (alp1 & 0x80) << 1;
                    lum1 &= 0x7F; alp1 &= 0x7F;
                    if (lum1 & 0x40) { lum1 -= 0x80; }
                    if (alp1 & 0x40) { alp1 -= 0x80; }
                    lum0 >>= 1; lum1 >>= 1; alp0 >>= 1; alp1 >>= 1;
                    lum1 += lum0; alp1 += alp0;
                    lum1 = clampi(lum1, 0, 255);
                    alp1 = clampi(alp1, 0, 255);
                    ep0[0]=ep0[1]=ep0[2]=lum0; ep0[3]=alp0;
                    ep1[0]=ep1[1]=ep1[2]=lum1; ep1[3]=alp1;
                }
                return  true;
            case 6: // LDR RGB base+scale
                {
                    int s = vals[3];
                    ep1[0]=vals[0]; ep1[1]=vals[1]; ep1[2]=vals[2]; ep1[3]=255;
                    ep0[0]=(vals[0]*s)>>8; ep0[1]=(vals[1]*s)>>8; ep0[2]=(vals[2]*s)>>8; ep0[3]=255;
                }
                return  true;
            case 8: // LDR RGB direct
                {
                    int r0=vals[0], g0=vals[1], b0=vals[2];
                    int r1=vals[3], g1=vals[4], b1=vals[5];
                    if (r0+g0+b0 > r1+g1+b1)
                    {
                        astcUncontract(r0, g0, b0);
                        astcUncontract(r1, g1, b1);
                        int t;
                        t=r0; r0=r1; r1=t;  t=g0; g0=g1; g1=t;  t=b0; b0=b1; b1=t;
                    }
                    ep0[0]=r0; ep0[1]=g0; ep0[2]=b0; ep0[3]=255;
                    ep1[0]=r1; ep1[1]=g1; ep1[2]=b1; ep1[3]=255;
                }
                return  true;
            case 9: // LDR RGB delta(brtc)
                {
                    int v[6] = { vals[0], vals[1], vals[2], vals[3], vals[4], vals[5] };
                    astcBitTransferSigned(v[1], v[0]);
                    astcBitTransferSigned(v[3], v[2]);
                    astcBitTransferSigned(v[5], v[4]);
                    int rgbSum  =   v[1] + v[3] + v[5];
                    int r0=v[0], g0=v[2], b0=v[4];
                    int r1=r0+v[1], g1=g0+v[3], b1=b0+v[5];
                    if (rgbSum < 0)
                    {
                        astcUncontract(r0, g0, b0);
                        astcUncontract(r1, g1, b1);
                        int t;
                        t=r0; r0=r1; r1=t;  t=g0; g0=g1; g1=t;  t=b0; b0=b1; b1=t;
                    }
                    ep0[0]=clampi(r0,0,255); ep0[1]=clampi(g0,0,255); ep0[2]=clampi(b0,0,255); ep0[3]=255;
                    ep1[0]=clampi(r1,0,255); ep1[1]=clampi(g1,0,255); ep1[2]=clampi(b1,0,255); ep1[3]=255;
                }
                return  true;
            case 10: // LDR RGB base+scale + alpha direct
                {
                    int s = vals[3];
                    ep1[0]=vals[0]; ep1[1]=vals[1]; ep1[2]=vals[2]; ep1[3]=vals[5];
                    ep0[0]=(vals[0]*s)>>8; ep0[1]=(vals[1]*s)>>8; ep0[2]=(vals[2]*s)>>8; ep0[3]=vals[4];
                }
                return  true;
            case 12: // LDR RGBA direct
                {
                    int r0=vals[0], g0=vals[1], b0=vals[2], a0=vals[3];
                    int r1=vals[4], g1=vals[5], b1=vals[6], a1=vals[7];
                    if (r0+g0+b0 > r1+g1+b1)
                    {
                        astcUncontract(r0, g0, b0);
                        astcUncontract(r1, g1, b1);
                        int t;
                        t=r0; r0=r1; r1=t;  t=g0; g0=g1; g1=t;  t=b0; b0=b1; b1=t;
                    }
                    ep0[0]=r0; ep0[1]=g0; ep0[2]=b0; ep0[3]=a0;
                    ep1[0]=r1; ep1[1]=g1; ep1[2]=b1; ep1[3]=a1;
                }
                return  true;
            case 13: // LDR RGBA delta(brtc)
                {
                    int v[8] = { vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7] };
                    astcBitTransferSigned(v[1], v[0]);
                    astcBitTransferSigned(v[3], v[2]);
                    astcBitTransferSigned(v[5], v[4]);
                    astcBitTransferSigned(v[7], v[6]);
                    int rgbSum  =   v[1] + v[3] + v[5];
                    int r0=v[0], g0=v[2], b0=v[4], a0=v[6];
                    int r1=r0+v[1], g1=g0+v[3], b1=b0+v[5], a1=a0+v[7];
                    if (rgbSum < 0)
                    {
                        astcUncontract(r0, g0, b0);
                        astcUncontract(r1, g1, b1);
                        int t;
                        t=r0; r0=r1; r1=t;  t=g0; g0=g1; g1=t;  t=b0; b0=b1; b1=t;
                    }
                    ep0[0]=clampi(r0,0,255); ep0[1]=clampi(g0,0,255); ep0[2]=clampi(b0,0,255); ep0[3]=clampi(a0,0,255);
                    ep1[0]=clampi(r1,0,255); ep1[1]=clampi(g1,0,255); ep1[2]=clampi(b1,0,255); ep1[3]=clampi(a1,0,255);
                }
                return  true;
            default:
                return  false;  // HDR CEM(2,3,7,11)与保留(14,15)
            }
        }

        /// ASTC 单块解码(LDR) → RGBA8 16/36/64 texel
        /// bw/bh: 块尺寸(4x4/6x6/8x8);out: bw*bh*4 字节,行序 raster
        /// 返回 false 表示 error block(调用方应写 error color)
        inline  bool    astcDecodeBlock(const uint8_t* block,int bw,int bh,uint8_t* out)
        {
            uint8_t     errorColor[4]   =   { 255, 0, 255, 255 };
            int     texelCount  =   bw * bh;
            uint8_t (*px)[4]    =   (uint8_t(*)[4])out;
            bool    smallBlock  =   texelCount < 32;

            int     blockMode   =   astcReadBits(block, 11, 0);

            // ---- void-extent / 常色块 ----
            if ((blockMode & 0x1FF) == 0x1FC)
            {
                int     hdr =   (blockMode >> 9) & 1;
                int     rsv =   astcReadBits(block, 2, 10);
                if (rsv != 3) { return  false; }

                int     vxLowS  =   astcReadBits(block, 8, 12) | (astcReadBits(block, 5, 20) << 8);
                int     vxHighS =   astcReadBits(block, 13, 25);
                int     vxLowT  =   astcReadBits(block, 8, 38) | (astcReadBits(block, 5, 46) << 8);
                int     vxHighT =   astcReadBits(block, 13, 51);
                bool    allOnes =   vxLowS == 0x1FFF && vxHighS == 0x1FFF &&
                                    vxLowT == 0x1FFF && vxHighT == 0x1FFF;
                if ((vxLowS >= vxHighS || vxLowT >= vxHighT) && !allOnes) { return  false; }

                if (hdr) { return  false; } // LDR 目标不支持 HDR 常色
                // UNORM16 色:R@64..79 G@80..95 B@96..111 A@112..127(每通道 16bit)
                int     r16 =   block[8] | (block[9] << 8);
                int     g16 =   block[10] | (block[11] << 8);
                int     b16 =   block[12] | (block[13] << 8);
                int     a16 =   block[14] | (block[15] << 8);
                uint8_t     c[4]    =
                {
                    (uint8_t)(r16 >> 8), (uint8_t)(g16 >> 8),
                    (uint8_t)(b16 >> 8), (uint8_t)(a16 >> 8)
                };
                for (int i = 0; i < texelCount; ++i) { std::memcpy(px[i], c, 4); }
                return  true;
            }

            // ---- 块模式 ----
            int     xw, yw, wQuant, wBits;
            bool    dual;
            if (!astcBlockMode2d(blockMode, xw, yw, dual, wQuant, wBits)) { return  false; }

            // ---- 分区 ----
            int     partCount   =   astcReadBits(block, 2, 11) + 1;

            // ---- 权重解码(权重数据从 bit127 向下填充) ----
            int     realWeightCount =   xw * yw * (dual ? 2 : 1);
            uint8_t     bswapped[16];
            for (int i = 0; i < 16; ++i) { bswapped[i] = astcBitRev8(block[15 - i]); }
            uint8_t     wIndices[64];
            astcDecodeIse(wQuant, realWeightCount, bswapped, wIndices, 0);

            static const uint8_t    W_UNQUANT[12][32]   =
            {
                { 0, 64 },
                { 0, 32, 64 },
                { 0, 21, 43, 64 },
                { 0, 16, 32, 48, 64 },
                { 0, 64, 12, 52, 25, 39 },
                { 0, 9, 18, 27, 37, 46, 55, 64 },
                { 0, 64, 7, 57, 14, 50, 21, 43, 28, 36 },
                { 0, 64, 17, 47, 5, 59, 23, 41, 11, 53, 28, 36 },
                { 0, 4, 8, 12, 17, 21, 25, 29, 35, 39, 43, 47, 52, 56, 60, 64 },
                { 0, 64, 16, 48, 3, 61, 19, 45, 6, 58, 23, 41, 9, 55, 26, 38, 13, 51, 29, 35 },
                { 0, 64, 8, 56, 16, 48, 24, 40, 2, 62, 11, 53, 19, 45, 27, 37, 5, 59, 13, 51, 22, 42, 30, 34 },
                { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64 }
            };
            uint8_t     weights[64];
            for (int i = 0; i < realWeightCount; ++i)
            {
                weights[i]  =   W_UNQUANT[wQuant][wIndices[i]];
            }

            if (dual && partCount == 4) { return  false; }

            // ---- CEM 解析(astcenc physical_to_symbolic 权威排布) ----
            int     cem[4]  =   { 0, 0, 0, 0 };
            int     highpartSize    =   0;
            int     startBit    =   17;
            int     belowWeightsPos =   128 - wBits;
            if (partCount == 1)
            {
                cem[0]  =   astcReadBits(block, 4, 13);
            }
            else
            {
                highpartSize    =   3 * partCount - 4;
                belowWeightsPos -=  highpartSize;
                int     encoded =   astcReadBits(block, 6, 13 + 10) |
                                    (astcReadBits(block, highpartSize, belowWeightsPos) << 6);
                int     baseclass   =   encoded & 0x3;
                if (baseclass == 0)
                {
                    int     f   =   (encoded >> 2) & 0xF;
                    for (int i = 0; i < partCount; ++i) { cem[i] = f; }
                    belowWeightsPos +=  highpartSize;
                    highpartSize    =   0;
                }
                else
                {
                    baseclass--;
                    int     bitpos  =   2;
                    for (int i = 0; i < partCount; ++i)
                    {
                        cem[i]  =   (((encoded >> bitpos) & 1) + baseclass) << 2;
                        ++bitpos;
                    }
                    for (int i = 0; i < partCount; ++i)
                    {
                        cem[i]  |=  (encoded >> bitpos) & 3;
                        bitpos  +=  2;
                    }
                }
                startBit    =   19 + 10;
            }

            // ---- 双平面分量 ----
            int     plane2Component =   -1;
            if (dual)
            {
                plane2Component =   astcReadBits(block, 2, belowWeightsPos - 2);
            }

            // ---- 端点量化级别:能塞进剩余位的最大级别 ----
            int     integerCount    =   0;
            for (int i = 0; i < partCount; ++i)
            {
                int     klass   =   cem[i] >> 2;
                integerCount    +=  (klass + 1) * 2;
            }
            if (integerCount > 18) { return  false; }

            int     colorBits   =   (partCount == 1 ? 111 : 99) - wBits - highpartSize - (dual ? 2 : 0);
            if (colorBits < 0) { colorBits = 0; }

            int     eQuant  =   4;  // QUANT_6 起
            bool    quantOk =   false;
            for (int ql = 20; ql >= 4; --ql)
            {
                if (astcIseBitCount(integerCount, ql) <= colorBits)
                {
                    eQuant  =   ql;
                    quantOk =   true;
                    break;
                }
            }
            if (!quantOk) { return  false; }

            // ---- 端点整数解码 + 反量化 ----
            uint8_t     eValues[32];
            astcDecodeIse(eQuant, integerCount, block, eValues, startBit);

            // scrambled → unquant 0..255 表(astcenc color_scrambled_pquant_to_uquant)
            static const uint8_t    EQ6[6]      =   { 0, 255, 51, 204, 102, 153 };
            static const uint8_t    EQ8[8]      =   { 0, 36, 73, 109, 146, 182, 219, 255 };
            static const uint8_t    EQ10[10]    =   { 0, 255, 28, 227, 56, 199, 84, 171, 113, 142 };
            static const uint8_t    EQ12[12]    =   { 0, 255, 69, 186, 23, 232, 92, 163, 46, 209, 116, 139 };
            static const uint8_t    EQ16[16]    =   { 0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255 };
            static const uint8_t    EQ20[20]    =   { 0, 255, 67, 188, 13, 242, 80, 175, 27, 228, 94, 161, 40, 215, 107, 148, 54, 201, 121, 134 };
            static const uint8_t    EQ24[24]    =   { 0, 255, 33, 222, 66, 189, 99, 156, 11, 244, 44, 211, 77, 178, 110, 145, 22, 233, 55, 200, 88, 167, 121, 134 };
            static const uint8_t    EQ32[32]    =   { 0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123, 132, 140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239, 247, 255 };
            static const uint8_t    EQ40[40]    =   { 0, 255, 32, 223, 65, 190, 97, 158, 6, 249, 39, 216, 71, 184, 104, 151, 13, 242, 45, 210, 78, 177, 110, 145, 19, 236, 52, 203, 84, 171, 117, 138, 26, 229, 58, 197, 91, 164, 123, 132 };
            static const uint8_t    EQ48[48]    =   { 0, 255, 16, 239, 32, 223, 48, 207, 65, 190, 81, 174, 97, 158, 113, 142, 5, 250, 21, 234, 38, 217, 54, 201, 70, 185, 86, 169, 103, 152, 119, 136, 11, 244, 27, 228, 43, 212, 59, 196, 76, 179, 92, 163, 108, 147, 124, 131 };
            static const uint8_t    EQ64[64]    =   { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190, 195, 199, 203, 207, 211, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255 };
            static const uint8_t    EQ80[80]    =   { 0, 255, 16, 239, 32, 223, 48, 207, 64, 191, 80, 175, 96, 159, 112, 143, 3, 252, 19, 236, 35, 220, 51, 204, 67, 188, 83, 172, 100, 155, 116, 139, 6, 249, 22, 233, 38, 217, 54, 201, 71, 184, 87, 168, 103, 152, 119, 136, 9, 246, 25, 230, 42, 213, 58, 197, 74, 181, 90, 165, 106, 149, 122, 133, 13, 242, 29, 226, 45, 210, 61, 194, 77, 178, 93, 162, 109, 146, 125, 130 };
            static const uint8_t    EQ96[96]    =   { 0, 255, 8, 247, 16, 239, 24, 231, 32, 223, 40, 215, 48, 207, 56, 199, 64, 191, 72, 183, 80, 175, 88, 167, 96, 159, 104, 151, 112, 143, 120, 135, 2, 253, 10, 245, 18, 237, 26, 229, 35, 220, 43, 212, 51, 204, 59, 196, 67, 188, 75, 180, 83, 172, 91, 164, 99, 156, 107, 148, 115, 140, 123, 132, 5, 250, 13, 242, 21, 234, 29, 226, 37, 218, 45, 210, 53, 202, 61, 194, 70, 185, 78, 177, 86, 169, 94, 161, 102, 153, 110, 145, 118, 137, 126, 129 };
            static const uint8_t    EQ128[128]  =
            {
                0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
                32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
                64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,
                96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126,
                129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159,
                161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191,
                193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223,
                225, 227, 229, 231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255
            };
            static const uint8_t    EQ160[160]  =
            {
                0, 255, 8, 247, 16, 239, 24, 231, 32, 223, 40, 215, 48, 207, 56, 199,
                64, 191, 72, 183, 80, 175, 88, 167, 96, 159, 104, 151, 112, 143, 120, 135,
                1, 254, 9, 246, 17, 238, 25, 230, 33, 222, 41, 214, 49, 206, 57, 198,
                65, 190, 73, 182, 81, 174, 89, 166, 97, 158, 105, 150, 113, 142, 121, 134,
                3, 252, 11, 244, 19, 236, 27, 228, 35, 220, 43, 212, 51, 204, 59, 196,
                67, 188, 75, 180, 83, 172, 91, 164, 99, 156, 107, 148, 115, 140, 123, 132,
                4, 251, 12, 243, 20, 235, 28, 227, 36, 219, 44, 211, 52, 203, 60, 195,
                68, 187, 76, 179, 84, 171, 92, 163, 100, 155, 108, 147, 116, 139, 124, 131,
                6, 249, 14, 241, 22, 233, 30, 225, 38, 217, 46, 209, 54, 201, 62, 193,
                70, 185, 78, 177, 86, 169, 94, 161, 102, 153, 110, 145, 118, 137, 126, 129
            };
            static const uint8_t    EQ192[192]  =
            {
                0, 255, 4, 251, 8, 247, 12, 243, 16, 239, 20, 235, 24, 231, 28, 227,
                32, 223, 36, 219, 40, 215, 44, 211, 48, 207, 52, 203, 56, 199, 60, 195,
                64, 191, 68, 187, 72, 183, 76, 179, 80, 175, 84, 171, 88, 167, 92, 163,
                96, 159, 100, 155, 104, 151, 108, 147, 112, 143, 116, 139, 120, 135, 124, 131,
                1, 254, 5, 250, 9, 246, 13, 242, 17, 238, 21, 234, 25, 230, 29, 226,
                33, 222, 37, 218, 41, 214, 45, 210, 49, 206, 53, 202, 57, 198, 61, 194,
                65, 190, 69, 186, 73, 182, 77, 178, 81, 174, 85, 170, 89, 166, 93, 162,
                97, 158, 101, 154, 105, 150, 109, 146, 113, 142, 117, 138, 121, 134, 125, 130,
                2, 253, 6, 249, 10, 245, 14, 241, 18, 237, 22, 233, 26, 229, 30, 225,
                34, 221, 38, 217, 42, 213, 46, 209, 50, 205, 54, 201, 58, 197, 62, 193,
                66, 189, 70, 185, 74, 181, 78, 177, 82, 173, 86, 169, 90, 165, 94, 161,
                98, 157, 102, 153, 106, 149, 110, 145, 114, 141, 118, 137, 122, 133, 126, 129
            };
            static const uint8_t    EQ256[256]  =
            {
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
                48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
                64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
                80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
                96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
                112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
                128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
                144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
                160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
                176,
                177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
                193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
                209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
                225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
                241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
            };
            static const uint8_t*   EQ_TABLES[17]   =
            {
                EQ6, EQ8, EQ10, EQ12, EQ16, EQ20, EQ24, EQ32, EQ40, EQ48,
                EQ64, EQ80, EQ96, EQ128, EQ160, EQ192, EQ256
            };
            const uint8_t*  epTable =   EQ_TABLES[eQuant - 4];

            // 按分区取出端点整数(端点流顺序:分区 0 的整数在前)
            uint8_t     epVals[4][18];
            {
                int     k   =   0;
                for (int p = 0; p < partCount; ++p)
                {
                    int     klass   =   cem[p] >> 2;
                    int     n       =   (klass + 1) * 2;
                    for (int j = 0; j < n; ++j)
                    {
                        epVals[p][j]    =   epTable[eValues[k++]];
                    }
                }
            }

            // ---- 端点颜色解包 ----
            int     ep0[4][4], ep1[4][4];
            for (int p = 0; p < partCount; ++p)
            {
                if (!astcUnpackEndpointsLDR(cem[p], epVals[p], ep0[p], ep1[p]))
                {
                    for (int i = 0; i < texelCount; ++i) { std::memcpy(px[i], errorColor, 4); }
                    return  true;
                }
            }

            // ---- 分区归属 ----
            int     partitionIndex  =   (partCount > 1) ? astcReadBits(block, 10, 13) : 0;
            uint8_t     partOfTexel[64];
            for (int y = 0; y < bh; ++y)
            {
                for (int x = 0; x < bw; ++x)
                {
                    int     t   =   y * bw + x;
                    partOfTexel[t]  =   (partCount > 1)
                        ?   (uint8_t)astcSelectPartition(partitionIndex, x, y, partCount, smallBlock)
                        :   0;
                }
            }

            // ---- 权重双线性上采样 + lerp(astcenc init_decimation_info_2d / unpack_weights 权威公式) ----
            // 权重坐标 s0.4 定点:int=xwv>>4, frac=xwv&0xF;零权重贡献跳过(与 astcenc 一致)
            int     half    =   realWeightCount / 2;    // 双平面时平面 2 权重起始
            for (int y = 0; y < bh; ++y)
            {
                for (int x = 0; x < bw; ++x)
                {
                    int     t       =   y * bw + x;
                    int     p       =   partOfTexel[t];

                    int     xwv     =   xw > 1 ? ((((1024 + bw / 2) / (bw - 1)) * x * (xw - 1) + 32) >> 6) : 0;
                    int     ywv     =   yw > 1 ? ((((1024 + bh / 2) / (bh - 1)) * y * (yw - 1) + 32) >> 6) : 0;
                    int     xFrac   =   xwv & 0xF;
                    int     yFrac   =   ywv & 0xF;
                    int     xInt    =   xwv >> 4;
                    int     yInt    =   ywv >> 4;

                    int     qweight[4];
                    qweight[0]  =   xInt + yInt * xw;
                    qweight[1]  =   qweight[0] + 1;
                    qweight[2]  =   qweight[0] + xw;
                    qweight[3]  =   qweight[2] + 1;

                    // 截断精度双线性权重(astcenc truncated-precision)
                    int     prod    =   xFrac * yFrac;
                    int     wv[4];
                    wv[3]   =   (prod + 8) >> 4;
                    wv[1]   =   xFrac - wv[3];
                    wv[2]   =   yFrac - wv[3];
                    wv[0]   =   16 - xFrac - yFrac + wv[3];

                    int     sum1    =   8;      // +8 舍入(astcenc summed_value)
                    int     sum2    =   8;
                    for (int k = 0; k < 4; ++k)
                    {
                        if (wv[k] == 0) { continue; }
                        sum1    +=  wv[k] * weights[qweight[k]];
                        if (dual) { sum2 += wv[k] * weights[half + qweight[k]]; }
                    }
                    int     wPlane1 =   sum1 >> 4;
                    int     wPlane2 =   sum2 >> 4;

                    uint8_t*    o   =   px[t];
                    for (int c = 0; c < 4; ++c)
                    {
                        int     w   =   (dual && c == plane2Component) ? wPlane2 : wPlane1;
                        o[c]    =   (uint8_t)((ep0[p][c] * (64 - w) + ep1[p][c] * w + 32) >> 6);
                    }
                }
            }
            return  true;
        }
    }
}

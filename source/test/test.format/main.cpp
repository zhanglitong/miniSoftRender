#include    <iostream>
#include    <cstdint>
#include    <cmath>
#include    "../../inc/FEFormatHelper.hpp"

using   namespace   FE;

static  int     g_failed    =   0;
static  int     g_total     =   0;

/// <summary>
/// 逐字节比较:打印首个差异,失败计数
/// </summary>
static  bool    expectBytes(const char* name,const uint8s& got,const uint8s& want)
{
    g_total++;
    if (got.size() != want.size())
    {
        std::cout << "[FAIL] " << name << ": size " << got.size() << " != " << want.size() << "\n";
        g_failed++;
        return  false;
    }
    for (size_t i = 0; i < want.size(); i++)
    {
        if (got[i] != want[i])
        {
            std::cout << "[FAIL] " << name << ": byte[" << i << "] got " << (int)got[i] << " want " << (int)want[i] << "\n";
            g_failed++;
            return  false;
        }
    }
    return  true;
}
template<typename T>
static  void    putLE(uint8s& out,T val)
{
    for (size_t i = 0; i < sizeof(T); i++)
        out.push_back(uint8_t(val >> (8*i)));
}

int     main(int ,char** )
{
    std::cout << "==== FEFormatHelper pack tests ====\n";

    /// 1. UNORM8:全值域 0..255 往返(v/255 打包后必须还原为 v)
    {
        bool    ok  =   true;
        for (int v = 0; v <= 255; v++)
        {
            auto    got =   FEFormatHelper::packFloats(floats{v/255.0f},FMT_R8_UNORM,1);
            if (!expectBytes("UNORM8 roundtrip",got,uint8s{uint8_t(v)}))
            {
                ok  =   false;
                break;
            }
        }
        if (ok) std::cout << "[PASS] UNORM8 full sweep 0..255\n";
    }
    /// 2. SNORM8:全值域 -127..127 往返
    {
        bool    ok  =   true;
        for (int s = -127; s <= 127; s++)
        {
            auto    got =   FEFormatHelper::packFloats(floats{s/127.0f},FMT_R8_SNORM,1);
            if (!expectBytes("SNORM8 roundtrip",got,uint8s{uint8_t(int8_t(s))}))
            {
                ok  =   false;
                break;
            }
        }
        if (ok) std::cout << "[PASS] SNORM8 full sweep -127..127\n";
    }
    /// 3. UNORM16:全值域 0..65535 往返(小端)
    {
        bool    ok  =   true;
        for (uint32_t v = 0; v <= 65535; v++)
        {
            auto    got =   FEFormatHelper::packFloats(floats{v/65535.0f},FMT_R16_UNORM,1);
            uint8s  want;
            putLE<uint16_t>(want,uint16_t(v));
            if (!expectBytes("UNORM16 roundtrip",got,want))
            {
                ok  =   false;
                std::cout << "       at v=" << v << "\n";
                break;
            }
        }
        if (ok) std::cout << "[PASS] UNORM16 full sweep 0..65535\n";
    }
    /// 4. SNORM16:全值域 -32767..32767 往返(小端)
    {
        bool    ok  =   true;
        for (int s = -32767; s <= 32767; s++)
        {
            auto    got =   FEFormatHelper::packFloats(floats{s/32767.0f},FMT_R16_SNORM,1);
            uint8s  want;
            putLE<int16_t>(want,int16_t(s));
            if (!expectBytes("SNORM16 roundtrip",got,want))
            {
                ok  =   false;
                std::cout << "       at s=" << s << "\n";
                break;
            }
        }
        if (ok) std::cout << "[PASS] SNORM16 full sweep -32767..32767\n";
    }
    /// 5. UINT/USCALED 8bit:整数值 0..255 恒等;带小数四舍五入
    {
        bool    ok  =   true;
        for (int v = 0; v <= 255 && ok; v++)
            ok  =   expectBytes("UINT8 roundtrip",FEFormatHelper::packFloats(floats{float(v)},FMT_R8_UINT,1),uint8s{uint8_t(v)});
        for (int v = 0; v <= 255 && ok; v++)
            ok  =   expectBytes("USCALED8 roundtrip",FEFormatHelper::packFloats(floats{float(v)},FMT_R8_USCALED,1),uint8s{uint8_t(v)});
        if (ok) std::cout << "[PASS] UINT8/USCALED8 full sweep 0..255\n";
        expectBytes("USCALED8 round 100.6",FEFormatHelper::packFloats(floats{100.6f},FMT_R8_USCALED,1),uint8s{101});
        expectBytes("USCALED8 round 100.4",FEFormatHelper::packFloats(floats{100.4f},FMT_R8_USCALED,1),uint8s{100});
    }
    /// 6. SINT/SSCALED 8bit:-128..127 恒等 + 越界 clamp
    {
        bool    ok  =   true;
        for (int v = -128; v <= 127 && ok; v++)
            ok  =   expectBytes("SINT8 roundtrip",FEFormatHelper::packFloats(floats{float(v)},FMT_R8_SINT,1),uint8s{uint8_t(int8_t(v))});
        if (ok) std::cout << "[PASS] SINT8/SSCALED8 full sweep -128..127\n";
        expectBytes("SSCALED8 clamp 200",FEFormatHelper::packFloats(floats{200.0f},FMT_R8_SSCALED,1),uint8s{uint8_t(int8_t(127))});
        expectBytes("SSCALED8 clamp -200",FEFormatHelper::packFloats(floats{-200.0f},FMT_R8_SSCALED,1),uint8s{uint8_t(int8_t(-128))});
    }
    /// 7. UINT/USCALED/SINT/SSCALED 16bit 全值域
    {
        bool    ok  =   true;
        for (uint32_t v = 0; v <= 65535; v++)
        {
            uint8s  want;
            putLE<uint16_t>(want,uint16_t(v));
            ok  =   expectBytes("UINT16 roundtrip",FEFormatHelper::packFloats(floats{float(v)},FMT_R16_UINT,1),want);
            if (!ok)    break;
            ok  =   expectBytes("USCALED16 roundtrip",FEFormatHelper::packFloats(floats{float(v)},FMT_R16_USCALED,1),want);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] UINT16/USCALED16 full sweep 0..65535\n";
        for (int s = -32768; s <= 32767 && ok; s++)
        {
            uint8s  want;
            putLE<int16_t>(want,int16_t(s));
            ok  =   expectBytes("SINT16 roundtrip",FEFormatHelper::packFloats(floats{float(s)},FMT_R16_SINT,1),want);
            if (!ok)    break;
            ok  =   expectBytes("SSCALED16 roundtrip",FEFormatHelper::packFloats(floats{float(s)},FMT_R16_SSCALED,1),want);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] SINT16/SSCALED16 full sweep -32768..32767\n";
    }
    /// 8. FLOAT32 恒等:按位模式抽样(含 0、负数、极小、极大)
    {
        bool    ok  =   true;
        union   Bits    {   float f; uint32_t u;   };
        for (uint32_t i = 0; i < 4096; i++)
        {
            Bits    b;
            b.u     =   (i * 0x00D813B7u)   ;   /// 均匀扫过指数/尾数空间
            if (!std::isfinite(b.f))    continue;
            uint8s  want;
            putLE<uint32_t>(want,b.u);
            ok  =   expectBytes("F32 identity",FEFormatHelper::packFloats(floats{b.f},FMT_R32_FLOAT,1),want);
            if (!ok)
            {
                std::cout << "       at bits=0x" << std::hex << b.u << std::dec << "\n";
                break;
            }
        }
        if (ok) std::cout << "[PASS] FLOAT32 identity sweep 4096 bit patterns\n";
    }
    /// 9. FLOAT16:已知 half 位模式 + 往返精度
    {
        expectBytes("half 1.0",FEFormatHelper::packFloats(floats{1.0f},FMT_R16_FLOAT,1),uint8s{0x00,0x3C});
        expectBytes("half 0.5",FEFormatHelper::packFloats(floats{0.5f},FMT_R16_FLOAT,1),uint8s{0x00,0x38});
        expectBytes("half 2.0",FEFormatHelper::packFloats(floats{2.0f},FMT_R16_FLOAT,1),uint8s{0x00,0x40});
        expectBytes("half -2.0",FEFormatHelper::packFloats(floats{-2.0f},FMT_R16_FLOAT,1),uint8s{0x00,0xC0});
        bool    ok  =   true;
        for (float v : {0.1f,0.33f,-0.77f,123.456f,-9.87f})
        {
            auto    got =   FEFormatHelper::packFloats(floats{v},FMT_R16_FLOAT,1);
            uint16_t    h   =   uint16_t(got[0]) | (uint16_t(got[1]) << 8);
            float   back    =   float16(h);
            g_total++;
            if (std::abs(back - v) > 1e-3f + 1e-3f*std::abs(v))
            {
                std::cout << "[FAIL] half roundtrip: " << v << " -> " << back << "\n";
                g_failed++;
                ok  =   false;
            }
        }
        if (ok) std::cout << "[PASS] half precision roundtrip\n";
    }
    /// 10. 多分量:补 0 与截断
    {
        expectBytes("pad 2->4 UNORM8",
            FEFormatHelper::packFloats(floats{0.25f,0.5f},FMT_R8G8B8A8_UNORM,2),
            uint8s{64,128,0,0});
        expectBytes("trunc 4->2 UNORM8",
            FEFormatHelper::packFloats(floats{0.25f,0.5f,0.75f,1.0f},FMT_R8G8_UNORM,4),
            uint8s{64,128});
        expectBytes("pad 2->4 half",
            FEFormatHelper::packFloats(floats{0.5f,0.25f},FMT_R16G16B16A16_FLOAT,2),
            uint8s{0x00,0x38,0x00,0x34,0x00,0x00,0x00,0x00});
        float3s tri {float3(1,2,3),float3(-1,0.5f,0)};
        expectBytes("float3s overload identity",
            FEFormatHelper::packFloats(tri,FMT_R32G32B32_FLOAT),
            ([](){ uint8s w; float vs[6]={1,2,3,-1,0.5f,0}; auto* p=reinterpret_cast<const uint8_t*>(vs);
                   w.assign(p,p+24); return w; })());
    }
    /// 11. packColors:UNORM 恒等(随机字节) + FLOAT 精确 + SNORM 独立手算表
    {
        Rgba8s  colors(4096);
        auto*   p   =   reinterpret_cast<uint8_t*>(colors.data());
        for (size_t i = 0; i < colors.size()*4; i++)
            p[i]    =   uint8_t((i*37 + 11) & 0xFF);
        uint8s  want(p,p+colors.size()*4);
        expectBytes("colors UNORM identity",FEFormatHelper::packColors(colors,FMT_R8G8B8A8_UNORM),want);

        auto    f32 =   FEFormatHelper::packColors(colors,FMT_R32G32B32A32_FLOAT);
        bool    ok  =   true;
        const   float*  fv  =   reinterpret_cast<const float*>(f32.data());
        for (size_t i = 0; i < colors.size()*4; i++)
        {
            g_total++;
            if (std::abs(fv[i] - p[i]/255.0f) > 1e-6f)
            {
                std::cout << "[FAIL] colors->FLOAT at " << i << ": " << fv[i] << " != " << p[i]/255.0f << "\n";
                g_failed++;
                ok  =   false;
                break;
            }
        }
        if (ok) std::cout << "[PASS] colors->FLOAT32 exact (16384 components)\n";

        /// SNORM8 手算对照:b/255*127+0.5 截断(独立于实现的期望公式,正数路径)
        bool    ok2 =   true;
        for (int b : {0,1,63,64,127,128,191,192,254,255})
        {
            auto    got =   FEFormatHelper::packFloats(floats{b/255.0f,b/255.0f,b/255.0f,b/255.0f},FMT_R8G8B8A8_SNORM,4);
            int8_t  e0  =   int8_t(int(b/255.0f*127.0f + 0.5f));
            if (got.size() != 4 || got[0] != uint8_t(e0))
            {
                std::cout << "[FAIL] SNORM8 hand table at b=" << b << ": got " << (got.empty()? -1 : (int)int8_t(got[0])) << " want " << (int)e0 << "\n";
                g_failed++; g_total++;
                ok2 =   false;
                break;
            }
        }
        if (ok2) { std::cout << "[PASS] SNORM8 hand-computed table (10 values)\n"; g_total++; }
    }
    /// 12. 空输入:返回空字节流,不触发 assert
    {
        expectBytes("empty floats",FEFormatHelper::packFloats(floats{},FMT_R32G32B32_FLOAT,3),uint8s{});
        expectBytes("empty colors",FEFormatHelper::packColors(Rgba8s{},FMT_R8G8B8A8_UNORM),uint8s{});
        std::cout << "[PASS] empty inputs\n";
    }
    /// 13. packed A2B10G10R10:10/10/10/2 单 word(小端),R=bit0-9,G=bit10-19,B=bit20-29,A=bit30-31
    {
        /// UNORM:R/G/B 全值域 0..1023 往返(源 3 分量,A 补 0)
        bool    ok  =   true;
        for (uint32_t v = 0; v <= 1023 && ok; v++)
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,v | (v << 10) | (v << 20));
            ok  =   expectBytes("A2B10G10R10 UNORM sweep",
                    FEFormatHelper::packFloats(floats{v/1023.0f,v/1023.0f,v/1023.0f},FMT_A2B10G10R10_UNORM,3),wantBytes);
            if (!ok)    std::cout << "       at v=" << v << "\n";
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 UNORM full sweep 0..1023\n";
        /// UNORM A 通道:2bit 映射 a/3 → a,位于最高 2 位(源 4 分量,RGB 为 0)
        ok  =   true;
        for (uint32_t a = 0; a <= 3 && ok; a++)
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,a << 30);
            ok  =   expectBytes("A2B10G10R10 UNORM alpha",
                    FEFormatHelper::packFloats(floats{0.0f,0.0f,0.0f,a/3.0f},FMT_A2B10G10R10_UNORM,4),wantBytes);
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 UNORM alpha 2bit\n";
        /// A 通道整数族:UINT/USCALED 原值 clamp[0,3];SINT/SSCALED 补码 2bit
        ok  =   true;
        for (uint32_t a = 0; a <= 3 && ok; a++)
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,a << 30);
            ok  =   expectBytes("A2B10G10R10 UINT alpha",
                    FEFormatHelper::packFloats(floats{0.0f,0.0f,0.0f,float(a)},FMT_A2B10G10R10_UINT,4),wantBytes);
            if (!ok)    break;
            ok  =   expectBytes("A2B10G10R10 USCALED alpha",
                    FEFormatHelper::packFloats(floats{0.0f,0.0f,0.0f,float(a)},FMT_A2B10G10R10_USCALED,4),wantBytes);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 UINT/USCALED alpha 2bit\n";
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,3u << 30);    /// -1 的 2bit 补码
            expectBytes("A2B10G10R10 SINT alpha -1",
                FEFormatHelper::packFloats(floats{0.0f,0.0f,0.0f,-1.0f},FMT_A2B10G10R10_SINT,4),wantBytes);
            expectBytes("A2B10G10R10 SSCALED alpha -1",
                FEFormatHelper::packFloats(floats{0.0f,0.0f,0.0f,-1.0f},FMT_A2B10G10R10_SSCALED,4),wantBytes);
            std::cout << "[PASS] A2B10G10R10 SINT/SSCALED alpha 2bit\n";
        }
        /// SNORM 手算表:独立公式 n=v*511±0.5 截断,补码取低 10 位
        ok  =   true;
        for (float v : {-1.0f,-0.5f,0.0f,0.25f,1.0f})
        {
            int32_t     n   =   int32_t(v*511.0f + (v < 0.0f ? -0.5f : 0.5f));
            uint32_t    e10 =   uint32_t((n % 1024 + 1024) % 1024);
            uint8s      wantBytes;
            putLE<uint32_t>(wantBytes,e10 | (e10 << 10) | (e10 << 20));
            ok  =   expectBytes("A2B10G10R10 SNORM hand table",
                    FEFormatHelper::packFloats(floats{v,v,v},FMT_A2B10G10R10_SNORM,3),wantBytes);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 SNORM hand-computed table\n";
        /// UINT/USCALED:0..1023 恒等全值域
        ok  =   true;
        for (uint32_t v = 0; v <= 1023 && ok; v++)
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,v | (v << 10) | (v << 20));
            ok  =   expectBytes("A2B10G10R10 UINT sweep",
                    FEFormatHelper::packFloats(floats{float(v),float(v),float(v)},FMT_A2B10G10R10_UINT,3),wantBytes);
            if (!ok)    break;
            ok  =   expectBytes("A2B10G10R10 USCALED sweep",
                    FEFormatHelper::packFloats(floats{float(v),float(v),float(v)},FMT_A2B10G10R10_USCALED,3),wantBytes);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 UINT/USCALED full sweep 0..1023\n";
        /// SINT/SSCALED:-512..511 恒等全值域(补码低 10 位)
        ok  =   true;
        for (int s = -512; s <= 511 && ok; s++)
        {
            uint32_t    e10 =   uint32_t((s % 1024 + 1024) % 1024);
            uint8s      wantBytes;
            putLE<uint32_t>(wantBytes,e10 | (e10 << 10) | (e10 << 20));
            ok  =   expectBytes("A2B10G10R10 SINT sweep",
                    FEFormatHelper::packFloats(floats{float(s),float(s),float(s)},FMT_A2B10G10R10_SINT,3),wantBytes);
            if (!ok)    break;
            ok  =   expectBytes("A2B10G10R10 SSCALED sweep",
                    FEFormatHelper::packFloats(floats{float(s),float(s),float(s)},FMT_A2B10G10R10_SSCALED,3),wantBytes);
            if (!ok)    break;
        }
        if (ok) std::cout << "[PASS] A2B10G10R10 SINT/SSCALED full sweep -512..511\n";
        /// 越界 clamp
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,0x3FFu | (0x3FFu << 10) | (0x3FFu << 20));
            expectBytes("A2B10G10R10 UINT clamp 2000",
                FEFormatHelper::packFloats(floats{2000.0f,2000.0f,2000.0f},FMT_A2B10G10R10_UINT,3),wantBytes);
            wantBytes.clear();
            putLE<uint32_t>(wantBytes,512u | (512u << 10) | (512u << 20));
            expectBytes("A2B10G10R10 SINT clamp -1000",
                FEFormatHelper::packFloats(floats{-1000.0f,-1000.0f,-1000.0f},FMT_A2B10G10R10_SINT,3),wantBytes);
        }
        /// 多元素 stride + 补 0 + 截断
        {
            uint8s  wantBytes;
            putLE<uint32_t>(wantBytes,512u | (256u << 10));
            putLE<uint32_t>(wantBytes,256u | (512u << 10));
            expectBytes("A2B10G10R10 2 elements pad",
                FEFormatHelper::packFloats(floats{0.5f,0.25f,0.25f,0.5f},FMT_A2B10G10R10_UNORM,2),wantBytes);
            wantBytes.clear();
            putLE<uint32_t>(wantBytes,1023u | (512u << 10) | (256u << 20) | (3u << 30));
            expectBytes("A2B10G10R10 truncate 5->4",
                FEFormatHelper::packFloats(floats{1.0f,0.5f,0.25f,1.0f,999.0f},FMT_A2B10G10R10_UNORM,5),wantBytes);
            std::cout << "[PASS] A2B10G10R10 clamp/pad/truncate\n";
        }
    }

    /// 14. 块压缩格式(BC/ETC2/EAC/ASTC):sizeOf/isBlockFormat/blockExtent/channelOf/
    ///     名称往返/pitch/bytesOfPlane(含非整块尺寸 5x3)
    {
        const FEFormat  blockFormats[]  =
        {
            FMT_BC1_RGB_UNORM, FMT_BC1_RGB_SRGB, FMT_BC1_RGBA_UNORM, FMT_BC1_RGBA_SRGB,
            FMT_BC2_UNORM, FMT_BC2_SRGB, FMT_BC3_UNORM, FMT_BC3_SRGB,
            FMT_BC4_UNORM, FMT_BC4_SNORM, FMT_BC5_UNORM, FMT_BC5_SNORM,
            FMT_BC6H_UFLOAT, FMT_BC6H_SFLOAT, FMT_BC7_UNORM, FMT_BC7_SRGB,
            FMT_ETC2_R8G8B8_UNORM, FMT_ETC2_R8G8B8_SRGB,
            FMT_ETC2_R8G8B8A8_UNORM, FMT_ETC2_R8G8B8A8_SRGB,
            FMT_EAC_R11_UNORM, FMT_EAC_R11_SNORM, FMT_EAC_R11G11_UNORM, FMT_EAC_R11G11_SNORM,
            FMT_ASTC_4x4_UNORM, FMT_ASTC_4x4_SRGB, FMT_ASTC_6x6_UNORM, FMT_ASTC_6x6_SRGB,
            FMT_ASTC_8x8_UNORM, FMT_ASTC_8x8_SRGB,
        };
        auto    check   =   [](bool cond,const char* what)
        {
            g_total++;
            if (!cond)
            {
                std::cout << "[FAIL] " << what << "\n";
                g_failed++;
                return  false;
            }
            return  true;
        };
        /// 64bit 块(8 字节)与 128bit 块(16 字节)的期望值
        auto    bytesOfBlock    =   [](FEFormat f)
        {
            switch (f)
            {
            case FMT_BC1_RGB_UNORM: case FMT_BC1_RGB_SRGB:
            case FMT_BC1_RGBA_UNORM: case FMT_BC1_RGBA_SRGB:
            case FMT_BC4_UNORM: case FMT_BC4_SNORM:
            case FMT_ETC2_R8G8B8_UNORM: case FMT_ETC2_R8G8B8_SRGB:
            case FMT_EAC_R11_UNORM: case FMT_EAC_R11_SNORM:
                return  8u;
            default:
                return  16u;
            }
        };
        /// 30 个块格式:sizeOf / isBlockFormat / blockExtent / 名称往返
        bool    ok  =   true;
        for (FEFormat f : blockFormats)
        {
            ok  =   check(FEFormatHelper::sizeOf(f) == bytesOfBlock(f),         "block sizeOf 8/16") && ok;
            ok  =   check(FEFormatHelper::isBlockFormat(f),                     "isBlockFormat true") && ok;
            /// ASTC 6x6/8x8 块粒度为其自身尺寸,其余(BC/ETC2/EAC/ASTC4x4)均 4x4
            const uint2 be      =   FEFormatHelper::blockExtent(f);
            const bool  bAstc66 =   (f == FMT_ASTC_6x6_UNORM || f == FMT_ASTC_6x6_SRGB);
            const bool  bAstc88 =   (f == FMT_ASTC_8x8_UNORM || f == FMT_ASTC_8x8_SRGB);
            const uint2 expect  =   bAstc66 ? uint2(6,6) : (bAstc88 ? uint2(8,8) : uint2(4,4));
            ok  =   check(be.x == expect.x && be.y == expect.y,                 "blockExtent per spec") && ok;
            const char* name    =   FEFormatHelper::enumName(f);
            ok  =   check(name != nullptr && FEFormatHelper::formatFromName(name) == f,
                                                                                "enumName/formatFromName roundtrip") && ok;
        }
        if (ok) std::cout << "[PASS] 30 block formats sizeOf/blockExtent/isBlock/name roundtrip\n";
        /// 未压缩格式不受块语义影响
        ok  =   true;
        for (FEFormat f : {FMT_R8_UNORM, FMT_R8G8B8A8_UNORM, FMT_R16G16B16A16_FLOAT, FMT_R16_UNORM})
        {
            ok  =   check(!FEFormatHelper::isBlockFormat(f),                    "uncompressed isBlockFormat false") && ok;
            const uint2 be  =   FEFormatHelper::blockExtent(f);
            ok  =   check(be.x == 1 && be.y == 1,                               "uncompressed blockExtent 1x1") && ok;
        }
        ok  =   check(!FEFormatHelper::isBlockFormat(FMT_NONE),                 "FMT_NONE isBlockFormat false") && ok;
        if (ok) std::cout << "[PASS] uncompressed formats keep 1x1 block extent\n";
        /// 语义通道:BC4/EAC_R11=1,BC5/EAC_R11G11=2,RGB 类=3,RGBA 类=4
        ok  =   true;
        for (FEFormat f : {FMT_BC4_UNORM, FMT_BC4_SNORM, FMT_EAC_R11_UNORM, FMT_EAC_R11_SNORM})
            ok  =   check(FEFormatHelper::channelOf(f) == 1,                    "block channelOf 1") && ok;
        for (FEFormat f : {FMT_BC5_UNORM, FMT_BC5_SNORM, FMT_EAC_R11G11_UNORM, FMT_EAC_R11G11_SNORM})
            ok  =   check(FEFormatHelper::channelOf(f) == 2,                    "block channelOf 2") && ok;
        for (FEFormat f : {FMT_BC1_RGB_UNORM, FMT_BC1_RGB_SRGB, FMT_BC6H_UFLOAT, FMT_BC6H_SFLOAT,
                           FMT_ETC2_R8G8B8_UNORM, FMT_ETC2_R8G8B8_SRGB})
            ok  =   check(FEFormatHelper::channelOf(f) == 3,                    "block channelOf 3") && ok;
        for (FEFormat f : {FMT_BC1_RGBA_UNORM, FMT_BC2_UNORM, FMT_BC3_UNORM, FMT_BC7_UNORM,
                           FMT_ETC2_R8G8B8A8_UNORM, FMT_ASTC_4x4_UNORM, FMT_ASTC_8x8_UNORM})
            ok  =   check(FEFormatHelper::channelOf(f) == 4,                    "block channelOf 4") && ok;
        if (ok) std::cout << "[PASS] block channelOf semantic channels\n";
        /// pitch / bytesOfPlane:非整块尺寸 5x3 与整块 8x8
        /// BC1:5 像素 → 2 块 × 8B = 16B(align 1/4 相同);8 像素 → 2 块 = 16B
        ok  =   check(FEFormatHelper::pitch(5,1,FMT_BC1_RGB_UNORM) == 16,       "BC1 pitch 5px align1 == 16") && ok;
        ok  =   check(FEFormatHelper::pitch(5,4,FMT_BC1_RGB_UNORM) == 16,       "BC1 pitch 5px align4 == 16") && ok;
        ok  =   check(FEFormatHelper::pitch(8,4,FMT_BC1_RGB_UNORM) == 16,       "BC1 pitch 8px align4 == 16") && ok;
        /// BC7:3 像素 → 1 块 × 16B = 16B
        ok  =   check(FEFormatHelper::pitch(3,16,FMT_BC7_UNORM) == 16,          "BC7 pitch 3px align16 == 16") && ok;
        /// bytesOfPlane:块行数 = ceil(h/4);BC1 5x3 → 1 行 × 16B;5x6 → 2 行;8x2 → 1 行
        ok  =   check(FEFormatHelper::bytesOfPlane(5,3,1,1,FMT_BC1_RGB_UNORM) == 16,  "BC1 plane 5x3 == 16") && ok;
        ok  =   check(FEFormatHelper::bytesOfPlane(5,6,1,1,FMT_BC1_RGB_UNORM) == 32,  "BC1 plane 5x6 == 32") && ok;
        ok  =   check(FEFormatHelper::bytesOfPlane(8,2,1,4,FMT_BC1_RGB_UNORM) == 16,  "BC1 plane 8x2 == 16") && ok;
        ok  =   check(FEFormatHelper::bytesOfPlane(8,8,1,4,FMT_BC7_UNORM) == 64,      "BC7 plane 8x8 == 64") && ok;
        /// 未压缩回归:行为不变
        ok  =   check(FEFormatHelper::pitch(5,4,FMT_R8_UNORM) == 8,             "R8 pitch 5px align4 == 8") && ok;
        ok  =   check(FEFormatHelper::bytesOfPlane(5,3,1,1,FMT_R8G8B8A8_UNORM) == 60, "RGBA8 plane 5x3 == 60") && ok;
        if (ok) std::cout << "[PASS] block pitch/bytesOfPlane (5x3, 8x8, non-aligned)\n";
    }

    std::cout << "==== " << (g_failed == 0 ? "ALL PASS" : "HAS FAILURES")
              << " : checks=" << g_total << " failed=" << g_failed << " ====\n";
    return  g_failed == 0 ? 0 : 1;
}


#include    <iostream>
#include    <cstring>
#include    <cmath>
#include    <string>
#include    <vector>
#include    <algorithm>
#include    <assert.h>
#include    "../../inc/FEImage.h"
#include    "../../inc/FEContext.hpp"
#include    "../../core/FEImageHelper.hpp"
#include    "../../core/graphic/FETextureHelper.hpp"
/// libktx(external/ktx,写测试用 KTX 文件;引擎加载路径也用它)
#include    <ktx.h>

using   namespace   FE;

static  int     g_failed    =   0;

#define CHECK(cond,msg)                                             \
    do{                                                             \
        if(cond){                                                   \
            std::cout << "[ OK ] " << msg << "\n";                  \
        }else{                                                      \
            std::cout << "[FAIL] " << msg                           \
                      << "  (line " << __LINE__ << ")\n";           \
            ++g_failed;                                             \
        }                                                           \
    }while(0)

/// 创建与创建信息
static  void    testCreate()
{
    std::cout << "---- testCreate ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    FEImage::CreateInfo cInfo =
    {
        uint3(111,115,1),
        FMT_R8G8B8_UNORM,
        1,
        1,
    };
    CHECK(image.create(cInfo),                "create valid image");
    CHECK(image.cInfo()._extent.x == 111,     "cInfo extent.x");
    CHECK(image.cInfo()._extent.y == 115,     "cInfo extent.y");
    CHECK(image.cInfo()._format  == FMT_R8G8B8_UNORM, "cInfo format");
    CHECK(image.cInfo()._buffers.size() == 1, "one layer one buffer");
    CHECK(image.cInfo()._buffers[0] != nullptr, "buffer allocated");
}

/// 级别大小与偏移(offsetOfLevel 回归)
static  void    testLevelSizeAndOffset()
{
    std::cout << "---- testLevelSizeAndOffset ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    /// 4x4 RGBA8 align=4 : level0=64 level1=16 level2=4
    image.create({uint3(4,4,1),FMT_R8G8B8A8_UNORM,2,3,4});

    CHECK(image.byteOfLevel(0) == 64,         "byteOfLevel(0) == 64");
    CHECK(image.byteOfLevel(1) == 16,         "byteOfLevel(1) == 16");
    CHECK(image.byteOfLevel(2) == 4,          "byteOfLevel(2) == 4");

    CHECK(image.offsetOfLevel(0) == 0,        "offsetOfLevel(0) == 0");
    CHECK(image.offsetOfLevel(1) == 64,       "offsetOfLevel(1) == byteOfLevel(0)");
    CHECK(image.offsetOfLevel(2) == 80,       "offsetOfLevel(2) == b0 + b1");

    CHECK(image.bytesOfLayer()  == 84,        "bytesOfLayer == 64+16+4");
    CHECK(image.size()          == 168,       "size == layers * bytesOfLayer");

    /// 偏移与大小必须恰好铺满一层
    CHECK(image.offsetOfLevel(2) + image.byteOfLevel(2) == image.bytesOfLayer(),
          "last level ends exactly at bytesOfLayer");
}

/// 对齐计算
static  void    testAlign()
{
    std::cout << "---- testAlign ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    /// 3x3 RGBA8 align=8 : pitch = (3*4 + 7)/8*8 = 16, 16*3 = 48
    image.create({uint3(3,3,1),FMT_R8G8B8A8_UNORM,1,1,8});
    CHECK(image.bytesOfLayer() == 48,         "align=8 pitch padding");
    CHECK(image.size()         == 48,         "size matches single layer");
}

/// pixel 指针与内存布局
static  void    testPixelLayout()
{
    std::cout << "---- testPixelLayout ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    image.create({uint3(4,4,1),FMT_R8G8B8A8_UNORM,2,3,4});

    uint8*  p0  =   image.pixel(0,0);
    uint8*  p1  =   image.pixel(0,1);
    uint8*  p2  =   image.pixel(0,2);
    CHECK(p0 != nullptr && p1 != nullptr && p2 != nullptr, "pixel non-null");
    CHECK(size_t(p1 - p0) == 64,              "pixel(0,1) - pixel(0,0) == byteOfLevel(0)");
    CHECK(size_t(p2 - p0) == 80,              "pixel(0,2) - pixel(0,0) == b0 + b1");

    /// const 与非 const 版本返回相同地址
    const FEImage&  cimage  =   image;
    CHECK(cimage.pixel(0,1) == p1,            "const pixel same address");

    /// 每个level写入不同字节,验证三个level在内存中严格连续不重叠
    std::memset(p0,0x11,image.byteOfLevel(0));
    std::memset(p1,0x22,image.byteOfLevel(1));
    std::memset(p2,0x33,image.byteOfLevel(2));

    bool    layout  =   true;
    for (uint32 i = 0;  i < 64; ++i)  layout &= (p0[i]      == 0x11);
    for (uint32 i = 0;  i < 16; ++i)  layout &= (p0[64 + i] == 0x22);
    for (uint32 i = 0;  i < 4;  ++i)  layout &= (p0[80 + i] == 0x33);
    CHECK(layout,                             "levels laid out contiguously, no overlap");

    /// 不同layer是独立buffer,写layer1不影响layer0
    uint8*  pL0 =   image.pixel(0,0);
    uint8*  pL1 =   image.pixel(1,0);
    CHECK(pL1 != nullptr && pL1 != pL0,       "layer buffers independent");
    std::memset(pL1,0xEE,image.bytesOfLayer());
    bool    keep    =   true;
    for (uint32 i = 0; i < 64; ++i)   keep &= (pL0[i] == 0x11);
    CHECK(keep,                               "write layer1 does not touch layer0");

#ifdef NDEBUG
    /// 越界返回nullptr(Debug下触发assert,仅在Release验证)
    CHECK(image.pixel(2,0) == nullptr,        "layer out of range -> nullptr");
    CHECK(image.pixel(0,3) == nullptr,        "level out of range -> nullptr");
#endif
}

/// plane 接口
static  void    testPlane()
{
    std::cout << "---- testPlane ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    image.create({uint3(4,4,1),FMT_R8G8B8A8_UNORM,1,3,4});

    auto    pl0 =   image.plane(0,0);
    CHECK(pl0._buffer == image.pixel(0,0),    "plane(0,0) buffer == pixel(0,0)");
    CHECK(pl0._width == 4 && pl0._height == 4,"plane(0,0) dims 4x4");

    auto    pl1 =   image.plane(1,0);
    CHECK(pl1._buffer == image.pixel(0,1),    "plane(1,0) buffer == pixel(0,1)");
    CHECK(pl1._width == 2 && pl1._height == 2,"plane(1,0) dims 2x2");

    auto    pl2 =   image.plane(2,0);
    CHECK(pl2._width == 1 && pl2._height == 1,"plane(2,0) dims 1x1");

#ifdef NDEBUG
    auto    plBad   =   image.plane(3,0);
    CHECK(plBad._buffer == nullptr,           "plane out of range -> null buffer");
#endif
}

/// fill(color) 填充所有layer与mip
/// 注意:fill<TFrom>会实例化switch中全部60+种目标格式的TConvert特化,
/// 任何TFrom都存在不支持的转换对(static_cast失败),当前无法编译,暂时禁用.
/// 修复方向:castTo增加if constexpr/SFINAE探测,不支持的分支返回false
#define TEST_WITH_FILL_COLOR    1
#if TEST_WITH_FILL_COLOR
static  void    testFillColor()
{
    std::cout << "---- testFillColor ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    image.create({uint3(4,4,1),FMT_R8G8B8A8_UNORM,2,3,4});

    /// float4 -> unorm : (0,1,0,1) 精确映射 (0,255,0,255)
    CHECK(image.fill(float4(0,1,0,1)),        "fill color returns true");

    const uint8     expect[4] =   {0,255,0,255};
    bool    allOk =   true;
    for (uint32 l = 0; l < 2; ++l)
    {
        const uint8*    p   =   image.pixel(l,0);
        for (uint32 i = 0; i < image.bytesOfLayer(); i += 4)
            allOk   &=  (std::memcmp(p + i,expect,4) == 0);
    }
    CHECK(allOk,                              "every pixel of every layer/level filled");
}
#endif /// TEST_WITH_FILL_COLOR

/// fill(color) 取值正确性校验
/// 0/255 是无量化误差的边界值,无法暴露舍入与跨格式路径;这里用中间值按格式读回逐字节比对
#if TEST_WITH_FILL_COLOR
static  void    testFillColorValues()
{
    std::cout << "---- testFillColorValues ----\n";
    FEContext&  ctx =   FEContext::instance();

    /// 1) uint4 -> RGBA8 : 整型值保持(钳制到宿范围)
    {
        FEImage     image(ctx);
        image.create({uint3(4,4,1),FMT_R8G8B8A8_UNORM,1,1,4});
        CHECK(image.fill(uint4(100,100,100,255)),   "fill uint4 -> rgba8");

        auto    pl  =   image.plane(0,0);
        const uint8 expect[4] = {100,100,100,255};
        bool    allOk   =   true;
        for (uint32 r = 0; r < 4; ++r)
            for (uint32 c = 0; c < 4; ++c)
                allOk   &=  (std::memcmp(pl.dataOffset<uint8>(r,c),expect,4) == 0);
        CHECK(allOk,                                "uint4(100,100,100,255) -> rgba8 reads back identical");
    }

    /// 2) float4 -> RGBA8 : 中间值必须四舍五入(截断会得到 99/127/63)
    {
        FEImage     image(ctx);
        image.create({uint3(1,1,1),FMT_R8G8B8A8_UNORM,1,1,4});
        CHECK(image.fill(float4(100.0f/255.0f,0.5f,0.25f,1.0f)), "fill float4 -> rgba8");

        const uint8 expect[4] = {100,128,64,255};
        CHECK(std::memcmp(image.plane(0,0).dataOffset<uint8>(0,0),expect,4) == 0,
              "100/255->100, 0.5->128, 0.25->64, alpha->255 (round not truncate)");
    }

    /// 3) float4 -> RGBA16 : Unorm16 满量程,100/255 应得 100*257
    {
        FEImage     image(ctx);
        image.create({uint3(1,1,1),FMT_R16G16B16A16_UNORM,1,1,8});
        CHECK(image.fill(float4(100.0f/255.0f,100.0f/255.0f,100.0f/255.0f,1.0f)), "fill float4 -> rgba16");

        const uint16    expect[4] = {25700,25700,25700,65535};
        CHECK(std::memcmp(image.plane(0,0).dataOffset<uint8>(0,0),expect,8) == 0,
              "100/255->25700, alpha->65535 on unorm16");
    }

    /// 4) uint4 -> RGBA16 : 两端同为无符号整型,值保持(与旧 static_cast 行为一致)
    {
        FEImage     image(ctx);
        image.create({uint3(1,1,1),FMT_R16G16B16A16_UNORM,1,1,8});
        CHECK(image.fill(uint4(100,100,100,255)),   "fill uint4 -> rgba16");

        const uint16    expect[4] = {100,100,100,255};
        CHECK(std::memcmp(image.plane(0,0).dataOffset<uint8>(0,0),expect,8) == 0,
              "uint4 value preserved on unorm16 target");
    }

    /// 5) float4 -> R8 : 单通道,只取 R,alpha 不影响灰度
    {
        FEImage     image(ctx);
        image.create({uint3(1,1,1),FMT_R8_UNORM,1,1,4});
        CHECK(image.fill(float4(0.5f,0.0f,0.0f,1.0f)), "fill float4 -> r8");

        const uint8 expect  =   128;
        CHECK(*image.plane(0,0).dataOffset<uint8>(0,0) == expect, "0.5 -> unorm8 rounds to 128");
    }
}
#endif /// TEST_WITH_FILL_COLOR

/// fill(pBuffer) 填充每个layer的level 0
/// TPBuffer的5处缺陷已修复(_blocks运算符优先级/瓦片矩形右下角/瓦片类型硬编码float4/
/// rowAt与dataOffset多乘sizeof(T)/FEImage::convert目标行漏加y),本用例启用
#define TEST_WITH_PBUFFER   1
#if TEST_WITH_PBUFFER
static  void    testFillPBuffer()
{
    std::cout << "---- testFillPBuffer ----\n";
    FEContext&  ctx =   FEContext::instance();
    FEImage     image(ctx);
    image.create({uint3(64,64,1),FMT_R8G8B8A8_UNORM,2,1,4});

    TPBuffer<uint8x4>   pbo(ctx);
    pbo.create(64,64);
    const uint8x4   color(1,2,3,4);
    for (auto& tile : pbo.tiles())
        tile._data.fill(color);

    CHECK(image.fill(pbo),                    "fill pBuffer returns true");

    const uint8     expect[4] =   {1,2,3,4};
    bool            allOk     =   true;
    for (uint32 l = 0; l < 2; ++l)
    {
        auto        pl  =   image.plane(0,l);
        const uint8 corner[4][2]  =   {{0,0},{63,0},{0,63},{63,63}};
        for (auto& c : corner)
            allOk   &=  (std::memcmp(pl.dataOffset(c[1],c[0]),expect,4) == 0);
    }
    CHECK(allOk,                              "all layers level0 corners filled");

#ifdef NDEBUG
    /// pBuffer大于图像 -> 拒绝(Release下验证,Debug触发assert)
    TPBuffer<uint8x4>   big(ctx);
    big.create(128,128);
    CHECK(!image.fill(big),                   "oversize pBuffer rejected");
#endif
}
#endif /// TEST_WITH_PBUFFER

/// =====================================================================
/// 全格式转换验证
/// 对 FEImage::fill 支持的每一种格式,以固定 float4 源填充,再按下方"格式规格表"
/// 独立推算期望字节并逐字节比对。规格表(cat+通道数+元素字节)为手工书写,
/// 与内核的 quantCatOfFormat/TypeOfFormat 映射互为独立,可交叉校验。
/// =====================================================================
struct  FmtCase
{
    FEFormat        fmt;
    FEQuantCat      cat;        /// 期望量化类别(Packed = 位打包,暂不支持从float转换)
    bool            fillOk;     /// fill(float4) 是否应当成功
};

/// 源色分量刻意避开 0 / 1 / 0.5 这类边界,使每种量化的期望值唯一且可人工核对
static  const   float   k_SrcR  =   0.25f;
static  const   float   k_SrcG  =   0.75f;
static  const   float   k_SrcB  =   1.00f;
static  const   float   k_SrcA  =   1.00f;

static  const   FmtCase g_fmtCases[]    =
{
    /// --- 8bit 归一化/整型 ---
    { FMT_R8_UNORM              ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R8_SNORM              ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R8_USCALED            ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R8_SSCALED            ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R8_UINT               ,   FEQuantCat::Uint    ,   true    },
    { FMT_R8_SINT               ,   FEQuantCat::Sint    ,   true    },
    { FMT_R8G8_UNORM            ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R8G8_SNORM            ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R8G8_USCALED          ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R8G8_SSCALED          ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R8G8_UINT             ,   FEQuantCat::Uint    ,   true    },
    { FMT_R8G8_SINT             ,   FEQuantCat::Sint    ,   true    },
    { FMT_R8G8B8_UNORM          ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R8G8B8_SNORM          ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R8G8B8_USCALED        ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R8G8B8_SSCALED        ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R8G8B8_UINT           ,   FEQuantCat::Uint    ,   true    },
    { FMT_R8G8B8_SINT           ,   FEQuantCat::Sint    ,   true    },
    { FMT_R8G8B8A8_UNORM        ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R8G8B8A8_SNORM        ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R8G8B8A8_USCALED      ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R8G8B8A8_SSCALED      ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R8G8B8A8_UINT         ,   FEQuantCat::Uint    ,   true    },
    { FMT_R8G8B8A8_SINT         ,   FEQuantCat::Sint    ,   true    },

    /// --- 16bit 归一化/整型 ---
    { FMT_R16_UNORM             ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R16_SNORM             ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R16_USCALED           ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R16_SSCALED           ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R16_UINT              ,   FEQuantCat::Uint    ,   true    },
    { FMT_R16_SINT              ,   FEQuantCat::Sint    ,   true    },
    { FMT_R16G16_UNORM          ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R16G16_SNORM          ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R16G16_USCALED        ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R16G16_SSCALED        ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R16G16_UINT           ,   FEQuantCat::Uint    ,   true    },
    { FMT_R16G16_SINT           ,   FEQuantCat::Sint    ,   true    },
    { FMT_R16G16B16_UNORM       ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R16G16B16_SNORM       ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R16G16B16_USCALED     ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R16G16B16_SSCALED     ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R16G16B16_UINT        ,   FEQuantCat::Uint    ,   true    },
    { FMT_R16G16B16_SINT        ,   FEQuantCat::Sint    ,   true    },
    { FMT_R16G16B16A16_UNORM    ,   FEQuantCat::Unorm   ,   true    },
    { FMT_R16G16B16A16_SNORM    ,   FEQuantCat::Snorm   ,   true    },
    { FMT_R16G16B16A16_USCALED  ,   FEQuantCat::Uscaled ,   true    },
    { FMT_R16G16B16A16_SSCALED  ,   FEQuantCat::Sscaled ,   true    },
    { FMT_R16G16B16A16_UINT     ,   FEQuantCat::Uint    ,   true    },
    { FMT_R16G16B16A16_SINT     ,   FEQuantCat::Sint    ,   true    },

    /// --- 32bit 整型 ---
    { FMT_R32_UINT              ,   FEQuantCat::Uint    ,   true    },
    { FMT_R32_SINT              ,   FEQuantCat::Sint    ,   true    },
    { FMT_R32G32_UINT           ,   FEQuantCat::Uint    ,   true    },
    { FMT_R32G32_SINT           ,   FEQuantCat::Sint    ,   true    },
    { FMT_R32G32B32_UINT        ,   FEQuantCat::Uint    ,   true    },
    { FMT_R32G32B32_SINT        ,   FEQuantCat::Sint    ,   true    },
    { FMT_R32G32B32A32_UINT     ,   FEQuantCat::Uint    ,   true    },
    { FMT_R32G32B32A32_SINT     ,   FEQuantCat::Sint    ,   true    },

    /// --- 浮点/半浮点 ---
    { FMT_R32_FLOAT             ,   FEQuantCat::Float   ,   true    },
    { FMT_R32G32_FLOAT          ,   FEQuantCat::Float   ,   true    },
    { FMT_R32G32B32_FLOAT       ,   FEQuantCat::Float   ,   true    },
    { FMT_R32G32B32A32_FLOAT    ,   FEQuantCat::Float   ,   true    },
    { FMT_R16_FLOAT             ,   FEQuantCat::Half    ,   true    },
    { FMT_R16G16_FLOAT          ,   FEQuantCat::Half    ,   true    },
    { FMT_R16G16B16_FLOAT       ,   FEQuantCat::Half    ,   true    },
    { FMT_R16G16B16A16_FLOAT    ,   FEQuantCat::Half    ,   true    },

    /// --- 深度 ---
    { FMT_D16_UNORM             ,   FEQuantCat::Half    ,   true    },
    { FMT_D32_UNORM             ,   FEQuantCat::Float   ,   true    },

    /// --- A2B10G10R10_SNORM 存储类型为uint32_t且量化类别为Packed,与其余A2B10变体一致被拒绝 ---
    { FMT_A2B10G10R10_SNORM     ,   FEQuantCat::Packed  ,   false   },

    /// --- 位打包格式:未实现位运算,从float4转换必须显式失败 ---
    { FMT_R4G4_UNORM            ,   FEQuantCat::Packed  ,   false   },
    { FMT_R4G4B4A4_UNORM        ,   FEQuantCat::Packed  ,   false   },
    { FMT_B4G4R4A4_UNORM        ,   FEQuantCat::Packed  ,   false   },
    { FMT_R5G6B5_UNORM          ,   FEQuantCat::Packed  ,   false   },
    { FMT_B5G6R5_UNORM          ,   FEQuantCat::Packed  ,   false   },
    { FMT_R5G5B5A1_UNORM        ,   FEQuantCat::Packed  ,   false   },
    { FMT_B5G5R5A1_UNORM        ,   FEQuantCat::Packed  ,   false   },
    { FMT_A1R5G5B5_UNORM        ,   FEQuantCat::Packed  ,   false   },
    { FMT_A2B10G10R10_UNORM     ,   FEQuantCat::Packed  ,   false   },
    { FMT_A2B10G10R10_USCALED   ,   FEQuantCat::Packed  ,   false   },
    { FMT_A2B10G10R10_SSCALED   ,   FEQuantCat::Packed  ,   false   },
    { FMT_A2B10G10R10_UINT      ,   FEQuantCat::Packed  ,   false   },
    { FMT_A2B10G10R10_SINT      ,   FEQuantCat::Packed  ,   false   },
    { FMT_D32_S8_UNORM          ,   FEQuantCat::Packed  ,   false   },
};

/// 独立参考实现:按量化类别与元素字节宽,把浮点分量编码成期望数值
static  double  specQuant(FEQuantCat cat,uint32 elemBytes,float v)
{
    const double    maxU    =   elemBytes == 4 ? 4294967295.0 : (elemBytes == 2 ? 65535.0 : 255.0);
    const double    maxS    =   elemBytes == 4 ? 2147483647.0 : (elemBytes == 2 ? 32767.0 : 127.0);
    const double    minS    =   elemBytes == 4 ? -2147483648.0: (elemBytes == 2 ? -32768.0: -128.0);
    switch (cat)
    {
    case FEQuantCat::Unorm:                     /// [0,1] → [0,max] 四舍五入
        return  std::round((std::min)((std::max)(double(v),0.0),1.0) * maxU);
    case FEQuantCat::Snorm:                     /// [-1,1] → [-max,max] 四舍五入
        return  std::round((std::min)((std::max)(double(v),-1.0),1.0) * maxS);
    case FEQuantCat::Uscaled:
    case FEQuantCat::Uint:                      /// 浮点按原值取整并钳制到 [0,max]
        return  std::round((std::min)((std::max)(double(v),0.0),maxU));
    case FEQuantCat::Sscaled:
    case FEQuantCat::Sint:                      /// 浮点按原值取整并钳制到 [min,max]
        return  std::round((std::min)((std::max)(double(v),minS),maxS));
    default:                                    /// Float / Half 直取
        return  double(v);
    }
}
/// 把期望值写成目标格式的元素字节
static  void    specEncode(uint8* dst,uint32 elemBytes,FEQuantCat cat,float v)
{
    if (cat == FEQuantCat::Float)
    {
        const float f   =   v;
        std::memcpy(dst,&f,4);
        return;
    }
    if (cat == FEQuantCat::Half)
    {
        const float16   h   =   float16(v);
        std::memcpy(dst,&h,2);
        return;
    }
    const double    q   =   specQuant(cat,elemBytes,v);
    if (elemBytes == 1)
    {
        const uint8 x   =   uint8((std::max)(q,0.0));
        dst[0]  =   x;
    }
    else if (elemBytes == 2)
    {
        const uint16    x   =   uint16((std::max)(q,0.0));
        std::memcpy(dst,&x,2);
    }
    else
    {
        const uint32    x   =   uint32((std::max)(q,0.0));
        std::memcpy(dst,&x,4);
    }
}

static  void    testFillAllFormats()
{
    std::cout << "---- testFillAllFormats ----\n";
    FEContext&  ctx =   FEContext::instance();

    const float4            src(k_SrcR,k_SrcG,k_SrcB,k_SrcA);
    const float             comp[4] =   {   k_SrcR,k_SrcG,k_SrcB,k_SrcA  };

    uint32  nChecked    =   0;      /// 成功填充并逐字节核对通过的格式数
    uint32  nRejected   =   0;      /// 按规格应拒绝且确实拒绝的格式数
    uint32  nMismatch   =   0;

    for (auto& tc : g_fmtCases)
    {
        FEImage     image(ctx);
        image.create({uint3(1,1,1),tc.fmt,1,1,4});

        const bool  filled  =   image.fill(src);
        if (!filled)
        {
            if (!tc.fillOk)
            {
                ++nRejected;
                continue;
            }
            std::cout << "[FAIL] fill rejected unexpectedly, fmt=" << uint32(tc.fmt) << "\n";
            ++nMismatch;
            continue;
        }

        const uint32    ch      =   FEFormatHelper::channelOf(tc.fmt);
        const uint32    elem    =   FEFormatHelper::sizeOf(tc.fmt) / ch;
        uint8           expect[16];
        for (uint32 c = 0; c < ch; ++c)
            specEncode(expect + c * elem,elem,tc.cat,comp[c]);

        const uint8*    got     =   image.plane(0,0).dataOffset<uint8>(0,0);
        if (std::memcmp(got,expect,ch * elem) != 0)
        {
            std::cout << "[FAIL] fmt=" << uint32(tc.fmt) << " got=";
            for (uint32 i = 0; i < ch * elem; ++i)
                std::cout << uint32(got[i]) << ",";
            std::cout << " expect=";
            for (uint32 i = 0; i < ch * elem; ++i)
                std::cout << uint32(expect[i]) << ",";
            std::cout << "\n";
            ++nMismatch;
            continue;
        }
        ++nChecked;
    }

    std::cout << "  formats: checked=" << nChecked
              << " rejected=" << nRejected
              << " mismatch=" << nMismatch << "\n";
    /// 期望值:66种非打包格式逐字节一致 + 15种位打包格式显式拒绝
    CHECK(nChecked == 66 && nRejected == 15 && nMismatch == 0,
          "every format converts to spec-derived bytes (checked=66 rejected=15)");

    /// A2B10G10R10_SNORM 手工特化:float4 ↔ 打包结构往返应回到原值(10bit snorm 步长内)
    {
        using   A2b10S  =   A2bgr10Snorm;
        const float             step    =   1.0f / 511.0f;
        const A2b10S            packed  =   castTo<float4,A2b10S>(src);
        const float4            back    =   castTo<A2b10S,float4>(packed);
        CHECK(   std::fabs(back.r - k_SrcR) <= step
              && std::fabs(back.g - k_SrcG) <= step
              && std::fabs(back.b - k_SrcB) <= step,
              "A2B10G10R10_SNORM float4 round trip within 10bit snorm step");
    }
}

/// =====================================================================
/// FEImageHelper:图片加载 / 保存(stb_image 解码,stb_image_write 编码)
/// =====================================================================
/// 构造带已知图案的 RGBA8 图像:像素 = (x*16, y*32, (x+y)*8, 255)
static  ImagePtr    makePattern(FEContext& ctx,uint32 w,uint32 h)
{
    ImagePtr    image   =   new FEImage(ctx,w,h,FMT_R8G8B8A8_UNORM);
    auto        plane   =   image->plane(0,0);
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
        {
            uint8*  px  =   plane.dataOffset<uint8>(y,x);
            px[0]   =   uint8(x * 16);
            px[1]   =   uint8(y * 32);
            px[2]   =   uint8((x + y) * 8);
            px[3]   =   255;
        }
    return  image;
}
/// 逐行比对图像 level0 与紧凑行参考数据
static  bool    sameRows(const FEImage& image,uint32 w,uint32 h,
                         const uint8* expect,size_t rowByte)
{
    auto    plane   =   image.plane(0,0);
    for (uint32 y = 0; y < h; ++y)
        if (std::memcmp(plane.dataOffset<uint8>(y,0),expect + size_t(y) * rowByte,rowByte) != 0)
            return  false;
    return  true;
}

static  void    testHelperSaveLoad()
{
    std::cout << "---- testHelperSaveLoad ----\n";
    FEContext&  ctx =   FEContext::instance();
    const uint32    W   =   16;
    const uint32    H   =   8;

    ImagePtr    src =   makePattern(ctx,W,H);

    /// 参考像素(紧凑行 RGBA8)取自图像本身
    std::vector<uint8>  ref(size_t(W) * H * 4);
    {
        auto    plane   =   src->plane(0,0);
        for (uint32 y = 0; y < H; ++y)
            std::memcpy(ref.data() + size_t(y) * W * 4,plane.dataOffset<uint8>(y,0),size_t(W) * 4);
    }

    /// 1) 无损格式:保存 -> 加载 -> 尺寸/格式/像素完全一致
    const char* lossless[]  =   { "png","bmp","tga" };
    for (auto ext : lossless)
    {
        const std::string   file    =   std::string("./test_image_tmp.") + ext;
        const std::string   tag     =   std::string("lossless ") + ext;
        CHECK(FEImageHelper::save(*src,file.c_str()),           (tag + " save ok"));
        ImagePtr    back    =   FEImageHelper::load(ctx,file.c_str());
        CHECK(back != nullptr,                                  (tag + " load ok"));
        if (back != nullptr)
        {
            const bool  sizeOk  =   back->cInfo()._extent.x == W && back->cInfo()._extent.y == H;
            CHECK(sizeOk,                                       (tag + " size preserved"));
            CHECK(back->cInfo()._format == FMT_R8G8B8A8_UNORM,  (tag + " loaded as RGBA8"));
            CHECK(sameRows(*back,W,H,ref.data(),size_t(W) * 4),(tag + " pixels identical"));
        }
        std::remove(file.c_str());
    }

    /// 2) jpg 有损:尺寸一致,三通道平均绝对误差在容差内(alpha 不参与编码)
    {
        const std::string   file    =   "./test_image_tmp.jpg";
        CHECK(FEImageHelper::save(*src,file.c_str(),95),        "lossy jpg save ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file.c_str());
        CHECK(back != nullptr,                                  "lossy jpg load ok");
        if (back != nullptr)
        {
            const bool  sizeOk  =   back->cInfo()._extent.x == W && back->cInfo()._extent.y == H;
            CHECK(sizeOk,                                       "jpg size preserved");
            double      sum =   0.0;
            auto        plane   =   back->plane(0,0);
            for (uint32 y = 0; y < H; ++y)
                for (uint32 x = 0; x < W; ++x)
                {
                    const uint8*    got =   plane.dataOffset<uint8>(y,x);
                    const uint8*    exp =   ref.data() + (size_t(y) * W + x) * 4;
                    for (int c = 0; c < 3; ++c)
                        sum +=  std::fabs(double(got[c]) - double(exp[c]));
                }
            const double    mae =   sum / (double(W) * H * 3.0);
            std::cout << "  jpg mean abs error = " << mae << "\n";
            CHECK(mae <= 12.0,                                  "jpg error within lossy tolerance");
        }
        std::remove(file.c_str());
    }

    /// 3) RGB8 源(3通道)保存:加载后统一补齐 alpha=255
    {
        FEImage     rgb(ctx,W,H,FMT_R8G8B8_UNORM);
        auto        plane   =   rgb.plane(0,0);
        for (uint32 y = 0; y < H; ++y)
            for (uint32 x = 0; x < W; ++x)
            {
                uint8*  px  =   plane.dataOffset<uint8>(y,x);
                px[0]   =   ref[(size_t(y) * W + x) * 4 + 0];
                px[1]   =   ref[(size_t(y) * W + x) * 4 + 1];
                px[2]   =   ref[(size_t(y) * W + x) * 4 + 2];
            }
        const std::string   file    =   "./test_image_tmp3.png";
        CHECK(FEImageHelper::save(rgb,file.c_str()),            "rgb8 -> png save ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file.c_str());
        CHECK(back != nullptr,                                  "rgb8 png load ok");
        if (back != nullptr)
        {
            bool    ok  =   (back->cInfo()._format == FMT_R8G8B8A8_UNORM);
            auto    bp  =   back->plane(0,0);
            for (uint32 y = 0; ok && y < H; ++y)
                for (uint32 x = 0; x < W; ++x)
                {
                    const uint8*    got =   bp.dataOffset<uint8>(y,x);
                    const uint8*    exp =   ref.data() + (size_t(y) * W + x) * 4;
                    ok  =   ok  &&  got[0] == exp[0] && got[1] == exp[1]
                                &&  got[2] == exp[2] && got[3] == 255;
                }
            CHECK(ok,   "rgb8 png round trip identical, alpha filled 255");
        }
        std::remove(file.c_str());
    }

    /// 4) R8 灰度源:png 往返后 RGB 三通道都等于源灰度
    {
        FEImage     gray(ctx,W,H,FMT_R8_UNORM);
        auto        plane   =   gray.plane(0,0);
        for (uint32 y = 0; y < H; ++y)
            for (uint32 x = 0; x < W; ++x)
                *plane.dataOffset<uint8>(y,x)   =   uint8((x * 16 + y * 4) & 0xFF);
        const std::string   file    =   "./test_image_tmp_gray.png";
        CHECK(FEImageHelper::save(gray,file.c_str()),           "r8 -> png save ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file.c_str());
        bool        ok      =   (back != nullptr);
        if (ok)
        {
            auto    bp  =   back->plane(0,0);
            for (uint32 y = 0; ok && y < H; ++y)
                for (uint32 x = 0; x < W; ++x)
                {
                    const uint8*    got =   bp.dataOffset<uint8>(y,x);
                    const uint8     g   =   uint8((x * 16 + y * 4) & 0xFF);
                    ok  =   ok  &&  got[0] == g && got[1] == g && got[2] == g;
                }
        }
        CHECK(ok,   "r8 gray png round trip");
        std::remove(file.c_str());
    }

    /// 5) 异常路径:非8bit格式 / 不支持的扩展名 / 文件不存在 / 空文件名
    {
        FEImage     rgba16(ctx,2,2,FMT_R16G16B16A16_UNORM);
        CHECK(!FEImageHelper::save(rgba16,"./test_image_tmp16.png"),
              "16bit format save rejected");
        CHECK(!FEImageHelper::save(*src,"./test_image_tmp.gif"),
              "unsupported extension -> false");
        CHECK(FEImageHelper::load(ctx,"./no_such_image_file.png") == nullptr,
              "load missing file -> nullptr");
        CHECK(FEImageHelper::load(ctx,nullptr) == nullptr,
              "load null name -> nullptr");
    }
}

/// =====================================================================
/// FEImageHelper::loadHdr + FETextureHelper::cubeImageFromEquirect
/// (equirect float image -> float cubemap image,纯 CPU 侧)
/// =====================================================================
/// 取路径的目录部分(用 argv[0] 定位可执行文件目录,进而定位 ../data)
static  String  dirOf(const char* path)
{
    const String    s   =   (path != nullptr) ? String(path) : String();
    const size_t    pos =   s.find_last_of("/\\");
    return  (pos == String::npos) ? String(".") : s.substr(0,pos);
}
/// 取 cubemap 指定面 / level 的 RGB 均值
static  float3  faceMean(const FEImage& cube,uint32 level,uint32 face)
{
    auto            plane   =   cube.plane(level,face);
    const uint32    w       =   plane.width();
    const uint32    h       =   plane.height();
    double          sum[3]  =   { 0.0,0.0,0.0 };
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
        {
            const float*    px  =   plane.dataOffset<float>(y,x);
            sum[0] += px[0]; sum[1] += px[1]; sum[2] += px[2];
        }
    const double    n   =   double(w) * h;
    return  float3(float(sum[0] / n),float(sum[1] / n),float(sum[2] / n));
}

static  void    testHdrAndCubeImage(const char* hdrPath)
{
    std::cout << "---- testHdrAndCubeImage ----\n";
    FEContext&  ctx =   FEContext::instance();

    /// 1) 真实 HDR -> float equirect image
    ImagePtr    equirect    =   FEImageHelper::loadHdr(ctx,hdrPath);
    if (equirect == nullptr)
    {
        std::cout << "  (skip) hdr file not found: " << (hdrPath ? hdrPath : "(null)") << "\n";
        return;
    }
    const auto& eqInfo  =   equirect->cInfo();
    std::cout << "  hdr " << eqInfo._extent.x << "x" << eqInfo._extent.y
              << " format=" << FEFormatHelper::enumName(eqInfo._format) << "\n";
    CHECK(eqInfo._format == FMT_R32G32B32A32_FLOAT,     "hdr loaded as RGBA32F");
    /// equirect 约定:宽为高的 2 倍
    CHECK(eqInfo._extent.x == 2 * eqInfo._extent.y,     "hdr is 2:1 equirect");

    /// 动态范围:浮点保留 >1 的高光,alpha 恒为 1
    {
        auto    plane   =   equirect->plane(0,0);
        float   maxC    =   0.0f;
        bool    alpha1  =   true;
        for (uint32 y = 0; y < plane.height(); ++y)
            for (uint32 x = 0; x < plane.width(); ++x)
            {
                const float*    px  =   plane.dataOffset<float>(y,x);
                for (int c = 0; c < 3; ++c)
                    maxC    =   px[c] > maxC ? px[c] : maxC;
                alpha1  =   alpha1 && (px[3] == 1.0f);
            }
        std::cout << "  hdr rgbMax = " << maxC << "\n";
        CHECK(maxC > 1.0f,                              "hdr keeps >1.0 float range");
        CHECK(alpha1,                                   "hdr alpha filled 1.0");
    }

    /// 2) equirect -> cubemap image(6 层 + 完整 mip 链)
    const uint32    cubeSize    =   64;
    ImagePtr        cube        =   FETextureHelper::cubeImageFromEquirect(ctx,*equirect,cubeSize);
    CHECK(cube != nullptr,                              "cube image created");
    if (cube != nullptr)
    {
        const auto& ci  =   cube->cInfo();
        CHECK(ci._layers == 6,                          "cube image has 6 layers");
        CHECK(ci._format == FMT_R32G32B32A32_FLOAT,     "cube image RGBA32F");
        CHECK(ci._levels == 7,                          "64x64 cube has 7 mip levels");
        CHECK(ci._extent.x == cubeSize && ci._extent.y == cubeSize, "cube face is size x size");

        /// 各 level 尺寸逐级减半(最小 1)
        bool    dimsOk  =   true;
        for (uint32 l = 0; l < ci._levels; ++l)
        {
            auto            p   =   cube->plane(l,0);
            const uint32    e   =   (std::max<uint32>)(cubeSize >> l,1);
            dimsOk  =   dimsOk && p.width() == e && p.height() == e;
        }
        CHECK(dimsOk,                                   "cube mip dims halve each level");

        /// 盒式滤波保均值:最低 mip 的单像素 == level0 该面 RGB 均值(逐面,浮点容差内)
        bool    meanOk  =   true;
        for (uint32 f = 0; f < 6; ++f)
        {
            const float3    m0      =   faceMean(*cube,0,f);
            const auto      last    =   cube->plane(ci._levels - 1,f);
            const float*    px      =   last.dataOffset<float>(0,0);
            const float     tol     =   1e-3f;
            meanOk  =   meanOk  &&  std::fabs(px[0] - m0.x) < tol
                                &&  std::fabs(px[1] - m0.y) < tol
                                &&  std::fabs(px[2] - m0.z) < tol
                                &&  px[3] == 1.0f;
        }
        CHECK(meanOk,                                   "cube mip chain preserves face mean");
    }

    /// 3) 合成常色 equirect -> cube 每个像素都等于该常色(确定性校验重采样全链路)
    {
        const uint32    w   =   8;
        const uint32    h   =   4;
        FEImage         eq(ctx,w,h,FMT_R32G32B32A32_FLOAT);
        auto            plane   =   eq.plane(0,0);
        for (uint32 y = 0; y < h; ++y)
            for (uint32 x = 0; x < w; ++x)
            {
                float*  px  =   plane.dataOffset<float>(y,x);
                px[0] = 0.25f; px[1] = 0.5f; px[2] = 0.75f; px[3] = 1.0f;
            }
        ImagePtr    c   =   FETextureHelper::cubeImageFromEquirect(ctx,eq,4);
        bool        ok  =   (c != nullptr);
        if (ok)
        {
            for (uint32 l = 0; ok && l < c->cInfo()._levels; ++l)
                for (uint32 f = 0; ok && f < 6; ++f)
                {
                    auto    p   =   c->plane(l,f);
                    for (uint32 y = 0; ok && y < p.height(); ++y)
                        for (uint32 x = 0; ok && x < p.width(); ++x)
                        {
                            const float*    px  =   p.dataOffset<float>(y,x);
                            ok  =   ok  &&  px[0] == 0.25f && px[1] == 0.5f
                                        &&  px[2] == 0.75f && px[3] == 1.0f;
                        }
                }
        }
        CHECK(ok,   "constant equirect -> constant cube (all faces/levels)");
    }

    /// 4) 错误路径:非浮点输入 / 尺寸为 0
    {
        FEImage     rgba8(ctx,4,4,FMT_R8G8B8A8_UNORM);
        CHECK(FETextureHelper::cubeImageFromEquirect(ctx,rgba8,4) == nullptr,
              "non-float equirect rejected");
        CHECK(FETextureHelper::cubeImageFromEquirect(ctx,*equirect,0) == nullptr,
              "size 0 rejected");
    }
}

/// =====================================================================
/// FEImageHelper::load / loadHdr 显式目标格式(调用方决定格式)
/// =====================================================================
static  void    testExplicitFormat(const char* hdrPath)
{
    std::cout << "---- testExplicitFormat ----\n";
    FEContext&  ctx =   FEContext::instance();

    /// 1) HDR 显式 RGB32F(3通道) vs 默认 RGBA32F(4通道)
    ImagePtr    rgb     =   FEImageHelper::loadHdr(ctx,hdrPath,FMT_R32G32B32_FLOAT);
    ImagePtr    rgba    =   FEImageHelper::loadHdr(ctx,hdrPath,FMT_R32G32B32A32_FLOAT);
    if (rgb == nullptr || rgba == nullptr)
    {
        std::cout << "  (skip) hdr file not found: " << (hdrPath ? hdrPath : "(null)") << "\n";
        return;
    }
    CHECK(rgb->cInfo()._format == FMT_R32G32B32_FLOAT,      "loadHdr explicit RGB32F");
    CHECK(rgb->cInfo()._extent.x == rgba->cInfo()._extent.x
       && rgb->cInfo()._extent.y == rgba->cInfo()._extent.y,"RGB32F extent == RGBA32F extent");
    /// 同一像素 RGB 三通道应逐位相同(3通道仅是少了 alpha)
    {
        auto    pr  =   rgb->plane(0,0);
        auto    pa  =   rgba->plane(0,0);
        bool    same=   true;
        for (uint32 y = 0; same && y < pa.height(); ++y)
            for (uint32 x = 0; same && x < pa.width(); ++x)
            {
                const float*    a   =   pa.dataOffset<float>(y,x);
                const float*    b   =   pr.dataOffset<float>(y,x);
                same    =   same && a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
            }
        CHECK(same,                                         "RGB32F rgb values == RGBA32F rgb values");
    }
    /// 3通道 equirect 转 cube 应与 4通道结果完全一致(验证 cubeImageFromEquirect 支持3通道)
    {
        ImagePtr    c3  =   FETextureHelper::cubeImageFromEquirect(ctx,*rgb,32);
        ImagePtr    c4  =   FETextureHelper::cubeImageFromEquirect(ctx,*rgba,32);
        CHECK(c3 != nullptr && c4 != nullptr,               "cube from 3ch and 4ch equirect");
        bool    same    =   (c3 != nullptr && c4 != nullptr);
        if (same)
        {
            for (uint32 l = 0; same && l < c4->cInfo()._levels; ++l)
                for (uint32 f = 0; same && f < 6; ++f)
                {
                    auto    p3  =   c3->plane(l,f);
                    auto    p4  =   c4->plane(l,f);
                    for (uint32 y = 0; same && y < p4.height(); ++y)
                        for (uint32 x = 0; same && x < p4.width(); ++x)
                        {
                            const float*    a   =   p4.dataOffset<float>(y,x);
                            const float*    b   =   p3.dataOffset<float>(y,x);
                            same    =   same && a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
                        }
                }
        }
        CHECK(same,                                         "3ch equirect -> same cube as 4ch");
    }
    /// HDR 显式单通道 R32F;及 loadHdr 拒绝非浮点系格式
    ImagePtr    r   =   FEImageHelper::loadHdr(ctx,hdrPath,FMT_R32_FLOAT);
    CHECK(r != nullptr && r->cInfo()._format == FMT_R32_FLOAT, "loadHdr explicit R32F");
    CHECK(FEImageHelper::loadHdr(ctx,hdrPath,FMT_R8G8B8A8_UNORM) == nullptr,
          "loadHdr rejects non-float format");

    /// 1c) HDR 显式半浮点(16F):与 32F 逐位对照,验证 float->half 转换链路
    {
        ImagePtr    h4  =   FEImageHelper::loadHdr(ctx,hdrPath,FMT_R16G16B16A16_FLOAT);
        CHECK(h4 != nullptr && h4->cInfo()._format == FMT_R16G16B16A16_FLOAT,
              "loadHdr explicit RGBA16F");
        if (h4 != nullptr)
        {
            auto            pf      =   rgba->plane(0,0);
            auto            ph      =   h4->plane(0,0);
            /// 逐像素逐通道:半点原始位 == float2Half(对应 32F 值) —— 位精确,无需容差
            bool            exact   =   true;
            float           maxRel  =   0.0f;
            for (uint32 y = 0; y < pf.height(); ++y)
            {
                const float*    f   =   pf.rowAt<float>(y);
                const uint16*   h   =   ph.rowAt<uint16>(y);
                for (uint32 i = 0; i < pf.width() * 4; ++i)
                {
                    exact   =   exact && (h[i] == float16::float2Half(f[i]));
                    const float hv  =   float16::half2Float(h[i]);
                    const float rel =   (f[i] != 0.0f) ? std::fabs(hv - f[i]) / std::fabs(f[i])
                                                       : std::fabs(hv);
                    maxRel  =   rel > maxRel ? rel : maxRel;
                }
            }
            std::cout << "  hdr RGBA16F maxRelErr = " << maxRel << "\n";
            CHECK(exact,                                    "RGBA16F bits == float2Half(RGBA32F) bit-exact");
            /// 半点相对精度 2^-11 ≈ 4.9e-4,留一倍余量
            CHECK(maxRel < 1.0e-3f,                          "RGBA16F within half precision of RGBA32F");
        }
    }
    CHECK(FEImageHelper::loadHdr(ctx,hdrPath,FMT_R16_FLOAT)      != nullptr, "loadHdr explicit R16F");
    CHECK(FEImageHelper::loadHdr(ctx,hdrPath,FMT_R16G16B16_FLOAT) != nullptr, "loadHdr explicit RGB16F");
    CHECK(FEImageHelper::loadHdr(ctx,hdrPath,FMT_R16_UNORM)      == nullptr, "loadHdr rejects R16_UNORM (not float family)");
    CHECK(FEImageHelper::loadHdr(ctx,hdrPath,FMT_R16G16B16A16_UNORM) == nullptr, "loadHdr rejects RGBA16_UNORM");

    /// 2) LDR:保存已知图案 PNG,再按不同显式格式加载
    const std::string   file    =   "./test_image_fmt_tmp.png";
    ImagePtr            src     =   makePattern(ctx,8,4);
    CHECK(FEImageHelper::save(*src,file.c_str()),       "pattern saved to png");

    ImagePtr    rgba8   =   FEImageHelper::load(ctx,file.c_str());
    ImagePtr    rgb8    =   FEImageHelper::load(ctx,file.c_str(),FMT_R8G8B8_UNORM);
    ImagePtr    r8      =   FEImageHelper::load(ctx,file.c_str(),FMT_R8_UNORM);
    ImagePtr    f4      =   FEImageHelper::load(ctx,file.c_str(),FMT_R32G32B32A32_FLOAT);
    ImagePtr    h4      =   FEImageHelper::load(ctx,file.c_str(),FMT_R16G16B16A16_FLOAT);
    CHECK(rgba8 != nullptr && rgba8->cInfo()._format == FMT_R8G8B8A8_UNORM, "load default -> RGBA8");
    CHECK(rgb8  != nullptr && rgb8->cInfo()._format  == FMT_R8G8B8_UNORM,   "load explicit RGB8");
    CHECK(r8    != nullptr && r8->cInfo()._format    == FMT_R8_UNORM,       "load explicit R8");
    CHECK(f4    != nullptr && f4->cInfo()._format    == FMT_R32G32B32A32_FLOAT, "load explicit RGBA32F");
    CHECK(h4    != nullptr && h4->cInfo()._format    == FMT_R16G16B16A16_FLOAT, "load explicit RGBA16F");

    /// LDR 的半点路径与单精度路径一致(同样逐位对照,PNG 的 8bit 值转浮点后两次加载可比)
    if (h4 != nullptr && f4 != nullptr)
    {
        auto            pf      =   f4->plane(0,0);
        auto            ph      =   h4->plane(0,0);
        bool            exact   =   true;
        for (uint32 y = 0; exact && y < pf.height(); ++y)
        {
            const float*    f   =   pf.rowAt<float>(y);
            const uint16*   h   =   ph.rowAt<uint16>(y);
            for (uint32 i = 0; i < pf.width() * 4; ++i)
                exact   =   exact && (h[i] == float16::float2Half(f[i]));
        }
        CHECK(exact,                                    "LDR RGBA16F == float2Half(RGBA32F) bit-exact");
    }

    /// RGB8 与默认 RGBA8 的 RGB 通道一致(载荷语义不因通道裁剪改变)
    if (rgb8 != nullptr && rgba8 != nullptr)
    {
        auto    p8  =   rgb8->plane(0,0);
        auto    p4  =   rgba8->plane(0,0);
        bool    same=   true;
        for (uint32 y = 0; same && y < p4.height(); ++y)
            for (uint32 x = 0; same && x < p4.width(); ++x)
            {
                const uint8*    a   =   p4.dataOffset<uint8>(y,x);
                const uint8*    b   =   p8.dataOffset<uint8>(y,x);
                same    =   same && a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
            }
        CHECK(same,                                     "explicit RGB8 == default RGBA8 rgb channels");
    }

    /// 3) 不支持的目标格式(16bit / 位打包)-> nullptr
    CHECK(FEImageHelper::load(ctx,file.c_str(),FMT_R16_UNORM) == nullptr,          "unsupported R16 rejected");
    CHECK(FEImageHelper::load(ctx,file.c_str(),FMT_R16G16B16A16_UNORM) == nullptr, "unsupported RGBA16 rejected");
    CHECK(FEImageHelper::load(ctx,file.c_str(),FMT_A2B10G10R10_UNORM) == nullptr,  "unsupported packed rejected");

    std::remove(file.c_str());
}

/// =====================================================================
/// FEImageHelper::loadKtx(KTX/KTX2,external/ktx libktx 静态库)
/// 用 libktx 写出确定性 KTX1/KTX2(含 mip 链/方向元数据/ETC1S 压缩),
/// 再经 FEImageHelper::load 读回校验:尺寸/格式/mip 链/像素与行序
/// =====================================================================
/// 生成确定性 RGBA8 图案数据(按 level 尺寸):像素 = (x*16, y*32, (x+y)*8, 255)
static  void    fillPattern(std::vector<uint8>& out,uint32 w,uint32 h)
{
    out.resize(size_t(w) * h * 4);
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
        {
            uint8*  px  =   out.data() + (size_t(y) * w + x) * 4;
            px[0]   =   uint8(x * 16);
            px[1]   =   uint8(y * 32);
            px[2]   =   uint8((x + y) * 8);
            px[3]   =   255;
        }
}
/// 写出 KTX2(w×h,levels 级 mip,各级独立图案;orientY='d'顶行在前/'u'底行在前)
static  bool    writeKtx2(const char* file,uint32 w,uint32 h,uint32 levels,char orientY)
{
    ktxTextureCreateInfo    ci  =   {};
    ci.vkFormat         =   37;     /// VK_FORMAT_R8G8B8A8_UNORM
    ci.baseWidth        =   w;
    ci.baseHeight       =   h;
    ci.baseDepth        =   1;
    ci.numDimensions    =   2;
    ci.numLevels        =   levels;
    ci.numLayers        =   1;
    ci.numFaces         =   1;
    ci.isArray          =   KTX_FALSE;
    ci.generateMipmaps  =   KTX_FALSE;
    ktxTexture2*    ktex2   =   nullptr;
    if  (ktxTexture2_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&ktex2) != KTX_SUCCESS)
        return  false;
    ktex2->orientation.y    =   (orientY == 'd') ? KTX_ORIENT_Y_DOWN : KTX_ORIENT_Y_UP;
    bool    ok  =   true;
    for (uint32 l = 0; ok && l < levels; ++l)
    {
        std::vector<uint8>  pixels;
        fillPattern(pixels,(std::max)(w >> l,1u),(std::max)(h >> l,1u));
        ok  =   ktxTexture_SetImageFromMemory(ktxTexture(ktex2),l,0,0,
                                              pixels.data(),pixels.size()) == KTX_SUCCESS;
    }
    if (ok)
        ok  =   ktxTexture_WriteToNamedFile(ktxTexture(ktex2),file) == KTX_SUCCESS;
    ktxTexture_Destroy(ktxTexture(ktex2));
    return  ok;
}
/// 写出 KTX1(RGBA8,levels 级 mip,默认方向元数据)
static  bool    writeKtx1(const char* file,uint32 w,uint32 h,uint32 levels)
{
    ktxTextureCreateInfo    ci  =   {};
    ci.glInternalformat =   0x8058; /// GL_RGBA8
    ci.baseWidth        =   w;
    ci.baseHeight       =   h;
    ci.baseDepth        =   1;
    ci.numDimensions    =   2;
    ci.numLevels        =   levels;
    ci.numLayers        =   1;
    ci.numFaces         =   1;
    ci.isArray          =   KTX_FALSE;
    ci.generateMipmaps  =   KTX_FALSE;
    ktxTexture1*    ktex1   =   nullptr;
    if  (ktxTexture1_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&ktex1) != KTX_SUCCESS)
        return  false;
    bool    ok  =   true;
    for (uint32 l = 0; ok && l < levels; ++l)
    {
        std::vector<uint8>  pixels;
        fillPattern(pixels,(std::max)(w >> l,1u),(std::max)(h >> l,1u));
        ok  =   ktxTexture_SetImageFromMemory(ktxTexture(ktex1),l,0,0,
                                              pixels.data(),pixels.size()) == KTX_SUCCESS;
    }
    if (ok)
        ok  =   ktxTexture_WriteToNamedFile(ktxTexture(ktex1),file) == KTX_SUCCESS;
    ktxTexture_Destroy(ktxTexture(ktex1));
    return  ok;
}
/// 校验图像某 level 与参考图案一致(flipY=true 时按垂直翻转比对)
static  bool    samePattern(const FEImage& image,uint32 level,uint32 w,uint32 h,bool flipY)
{
    std::vector<uint8>  ref;
    fillPattern(ref,w,h);
    auto    plane   =   image.plane(level,0);
    if (plane._buffer == nullptr || plane.width() != w || plane.height() != h)
        return  false;
    for (uint32 y = 0; y < h; ++y)
    {
        const uint32    srcY    =   flipY ? (h - 1 - y) : y;
        if (std::memcmp(plane.dataOffset<uint8>(y,0),ref.data() + size_t(srcY) * w * 4,size_t(w) * 4) != 0)
            return  false;
    }
    return  true;
}

static  void    testKtxLoad()
{
    std::cout << "---- testKtxLoad ----\n";
    FEContext&  ctx =   FEContext::instance();

    /// 1) KTX2 往返:8x8 满 mip 链(4 级),orientation='d'(顶行在前)→ 不翻转
    {
        const char* file    =   "./test_ktx2_d.ktx2";
        CHECK(writeKtx2(file,8,8,4,'d'),                    "ktx2(d) write ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr,                              "ktx2(d) load ok");
        if (back != nullptr)
        {
            CHECK(back->cInfo()._extent.x == 8 && back->cInfo()._extent.y == 8, "ktx2(d) size preserved");
            CHECK(back->cInfo()._format == FMT_R8G8B8A8_UNORM,                  "ktx2(d) loaded as RGBA8");
            CHECK(back->cInfo()._levels == 4,                                   "ktx2(d) mip chain 4 levels");
            bool    ok  =   true;
            for (uint32 l = 0; l < 4; ++l)
                ok  =   ok && samePattern(*back,l,(std::max)(8u >> l,1u),(std::max)(8u >> l,1u),false);
            CHECK(ok,                                                           "ktx2(d) all levels pixel-identical");
        }
        std::remove(file);
    }

    /// 2) KTX2 行序:orientation='u'/'d' 只是元数据提示,libktx 原样存储
    ///    SetImageFromMemory 的字节(顶行在前),读回也不翻转
    {
        const char* file    =   "./test_ktx2_u.ktx2";
        CHECK(writeKtx2(file,8,8,1,'u'),                    "ktx2(u) write ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr && samePattern(*back,0,8,8,false),
                                                            "ktx2(u) read back top-row-first (no flip)");
        std::remove(file);
    }

    /// 3) KTX1 往返:RGBA8 + mip 链;libktx 原样存储顶行在前的字节
    {
        const char* file    =   "./test_ktx1.ktx";
        CHECK(writeKtx1(file,8,8,3),                        "ktx1 write ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr,                              "ktx1 load ok");
        if (back != nullptr)
        {
            CHECK(back->cInfo()._extent.x == 8 && back->cInfo()._extent.y == 8, "ktx1 size preserved");
            CHECK(back->cInfo()._format == FMT_R8G8B8A8_UNORM,                  "ktx1 loaded as RGBA8");
            CHECK(back->cInfo()._levels == 3,                                   "ktx1 mip chain 3 levels");
            bool    ok  =   true;
            for (uint32 l = 0; l < 3; ++l)
                ok  =   ok && samePattern(*back,l,(std::max)(8u >> l,1u),(std::max)(8u >> l,1u),false);
            CHECK(ok,                                                           "ktx1 all levels top-row-first (no flip)");
        }
        std::remove(file);
    }

    /// 4) ETC1S 压缩 KTX2:常色图压缩 → transcode RGBA32 读回(有损,容差 ±4)
    {
        const char*     file    =   "./test_ktx2_etc1s.ktx2";
        const uint32    W = 8, H = 8;
        ktxTextureCreateInfo    ci  =   {};
        ci.vkFormat         =   37;
        ci.baseWidth        =   W;
        ci.baseHeight       =   H;
        ci.baseDepth        =   1;
        ci.numDimensions    =   2;
        ci.numLevels        =   1;
        ci.numLayers        =   1;
        ci.numFaces         =   1;
        ci.isArray          =   KTX_FALSE;
        ci.generateMipmaps  =   KTX_FALSE;
        ktxTexture2*    ktex2   =   nullptr;
        bool    ok  =   ktxTexture2_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&ktex2) == KTX_SUCCESS;
        const uint8     kColor[4]   =   { 200,100,50,255 };
        if (ok)
        {
            std::vector<uint8>  pixels(size_t(W) * H * 4);
            for (size_t i = 0; i < size_t(W) * H; ++i)
                std::memcpy(pixels.data() + i * 4,kColor,4);
            ok  =   ktxTexture_SetImageFromMemory(ktxTexture(ktex2),0,0,0,pixels.data(),pixels.size()) == KTX_SUCCESS;
        }
        if (ok)
        {
            /// 简化压缩接口:quality 取中值 128(ETC1S,常色图任意质量均可)
            ok  =   ktxTexture2_CompressBasis(ktex2,128) == KTX_SUCCESS;
        }
        if (ok)
            ok  =   ktxTexture_WriteToNamedFile(ktxTexture(ktex2),file) == KTX_SUCCESS;
        if (ktex2 != nullptr)
            ktxTexture_Destroy(ktxTexture(ktex2));
        CHECK(ok,                                                           "ktx2 etc1s compress+write ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr,                                              "ktx2 etc1s load(transcode) ok");
        if (back != nullptr)
        {
            CHECK(back->cInfo()._format == FMT_R8G8B8A8_UNORM,              "etc1s transcoded to RGBA8");
            auto    plane   =   back->plane(0,0);
            bool    close   =   true;
            for (uint32 y = 0; y < H; ++y)
                for (uint32 x = 0; x < W; ++x)
                {
                    const uint8*    px  =   plane.dataOffset<uint8>(y,x);
                    for (int c = 0; c < 4; ++c)
                        close   =   close && std::abs(int(px[c]) - int(kColor[c])) <= 4;
                }
            CHECK(close,                                                    "etc1s constant color within +-4");
        }
        std::remove(file);
    }

    /// 5) 异常路径:文件不存在 / 不支持的目标格式 / 空文件名
    {
        CHECK(FEImageHelper::load(ctx,"./no_such_texture.ktx2") == nullptr,
              "load missing ktx2 -> nullptr");
        CHECK(FEImageHelper::load(ctx,"./no_such_texture.ktx") == nullptr,
              "load missing ktx -> nullptr");
        const char* file    =   "./test_ktx2_fmt.ktx2";
        CHECK(writeKtx2(file,4,4,1,'d'),                                    "ktx2 fmt-guard write ok");
        CHECK(FEImageHelper::load(ctx,file,FMT_R16G16B16A16_FLOAT) == nullptr,
              "loadKtx rejects non-RGBA8 target format");
        ImagePtr    ok  =   FEImageHelper::load(ctx,file,FMT_R8G8B8A8_UNORM);
        CHECK(ok != nullptr,                                                "loadKtx accepts explicit RGBA8");
        std::remove(file);
    }

    /// 6) FEImageHelper::save 往返(ktx / ktx2 无压缩 / ktx2 压缩 / R8G8)
    {
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(8,8,1);
        ci._format  =   FMT_R8G8B8A8_UNORM;
        ci._layers  =   1;
        ci._levels  =   4;
        ImagePtr    img =   new FEImage(ctx);
        CHECK(img->create(ci) && img->size() > 0,                          "save-rt create RGBA8 mip image");
        for (uint32 l = 0; l < 4; ++l)
        {
            auto    plane   =   img->plane(l,0);
            std::vector<uint8>  pixels;
            fillPattern(pixels,(std::max)(8u >> l,1u),(std::max)(8u >> l,1u));
            /// plane 是值对象(非指针),dataOffset 返回裸指针
            for (uint32 y = 0; y < plane.height(); ++y)
                std::memcpy(plane.dataOffset<uint8>(y,0),pixels.data() + size_t(y) * plane.width() * 4,
                            size_t(plane.width()) * 4);
        }

        /// ktx 往返(KTX1 + GL_RGBA8 + 完整 mip)
        {
            const char* file    =   "./rt.ktx";
            CHECK(FEImageHelper::save(*img,file),                           "save RGBA8 mip -> ktx ok");
            ImagePtr    back    =   FEImageHelper::load(ctx,file);
            CHECK(back != nullptr && back->cInfo()._levels == 4,            "ktx load 4 levels");
            bool    ok  =   true;
            for (uint32 l = 0; l < 4; ++l)
                ok  =   ok && samePattern(*back,l,(std::max)(8u >> l,1u),(std::max)(8u >> l,1u),false);
            CHECK(ok,                                                        "ktx roundtrip all levels identical");
            std::remove(file);
        }
        /// ktx2 无压缩往返(quality=0 → 跳过 Basis → 像素逐字节一致)
        {
            const char* file    =   "./rt.ktx2";
            CHECK(FEImageHelper::save(*img,file,0),                          "save RGBA8 mip -> ktx2 (no compress)");
            ImagePtr    back    =   FEImageHelper::load(ctx,file);
            CHECK(back != nullptr && back->cInfo()._levels == 4,            "ktx2 load 4 levels");
            bool    ok  =   true;
            for (uint32 l = 0; l < 4; ++l)
                ok  =   ok && samePattern(*back,l,(std::max)(8u >> l,1u),(std::max)(8u >> l,1u),false);
            CHECK(ok,                                                        "ktx2(no compress) roundtrip all levels identical");
            std::remove(file);
        }
        /// ktx2 压缩往返(quality=128 ETC1S → 读回 transcode,常色图容差 ±4)
        /// 常色图 ETC1S 压缩失真可忽略,验证 save→压缩→transcode 全链路
        {
            const uint8     kColor[4]   =   { 120,200,40,255 };
            FEImage::CreateInfo ce;
            ce._extent = uint3(8,8,1); ce._format = FMT_R8G8B8A8_UNORM; ce._levels = 2; ce._layers = 1;
            ImagePtr    ic  =   new FEImage(ctx);
            CHECK(ic->create(ce) && ic->size() > 0,                         "etc1s constant image created");
            for (uint32 l = 0; l < 2; ++l)
            {
                auto    pc  =   ic->plane(l,0);
                const uint32    w = pc.width(), h = pc.height();
                for (uint32 y = 0; y < h; ++y)
                    for (uint32 x = 0; x < w; ++x)
                        std::memcpy(pc.dataOffset<uint8>(y,x),kColor,4);
            }
            const char* file    =   "./rt_etc1s.ktx2";
            CHECK(FEImageHelper::save(*ic,file,128),                        "save RGBA8 mip -> ktx2(ETC1S q=128) ok");
            ImagePtr    back    =   FEImageHelper::load(ctx,file);
            CHECK(back != nullptr && back->cInfo()._levels == 2,            "ktx2(ETC1S) load 2 levels");
            if (back != nullptr)
            {
                bool    close   =   true;
                for (uint32 l = 0; close && l < 2; ++l)
                {
                    auto    pc  =   back->plane(l,0);
                    for (uint32 y = 0; close && y < pc.height(); ++y)
                        for (uint32 x = 0; close && x < pc.width(); ++x)
                        {
                            const uint8*    px  =   pc.dataOffset<uint8>(y,x);
                            for (int c = 0; c < 4 && close; ++c)
                                close = close && std::abs(int(px[c]) - int(kColor[c])) <= 4;
                        }
                }
                CHECK(close,                                                "ktx2(ETC1S) constant color within +-4");
            }
            std::remove(file);
        }

        /// R8G8(2 通道) save ktx1 → loadKtx 扩通道逐像素往返(GL_RG8 通道映射)
        {
            FEImage::CreateInfo c2;
            c2._extent = uint3(6,4,1); c2._format = FMT_R8G8_UNORM; c2._levels = 1; c2._layers = 1;
            ImagePtr    i2  =   new FEImage(ctx);
            CHECK(i2->create(c2) && i2->size() > 0,                         "save-rt create R8G8 image");
            auto    p   =   i2->plane(0,0);
            for (uint32 y = 0; y < 4; ++y)
                for (uint32 x = 0; x < 6; ++x)
                {
                    uint8*  px  =   p.dataOffset<uint8>(y,x);
                    px[0]   =   uint8(x * 40);
                    px[1]   =   uint8(y * 60);
                }
            const char* file    =   "./rt_rg8.ktx";
            CHECK(FEImageHelper::save(*i2,file),                            "save R8G8 -> ktx ok (channel mapping works)");
            ImagePtr    back    =   FEImageHelper::load(ctx,file);
            CHECK(back != nullptr && back->cInfo()._format == FMT_R8G8_UNORM,
                                                                            "loadKtx R8G8 ktx -> R8G8 (expanded channels)");
            if (back != nullptr)
            {
                auto    pb  =   back->plane(0,0);
                bool    same=   pb.width() == 6 && pb.height() == 4;
                for (uint32 y = 0; same && y < 4; ++y)
                    for (uint32 x = 0; same && x < 6; ++x)
                    {
                        const uint8*    a   =   p.dataOffset<uint8>(y,x);
                        const uint8*    b   =   pb.dataOffset<uint8>(y,x);
                        same    =   same && a[0] == b[0] && a[1] == b[1];
                    }
                CHECK(same,                                                 "R8G8 ktx roundtrip pixel-identical");
            }
            std::remove(file);
        }
    }
}

/// =====================================================================
/// KTX 扩展覆盖:未压缩扩通道(R8/RGB8/RGBA16 ktx2)、未知 vkFormat 拒绝、
/// 全部 30 个块压缩格式 ktx2 逐字节往返、BasisU 转码目标(BC7/BC1)
/// =====================================================================
static  void    testKtxExpanded()
{
    std::cout << "---- testKtxExpanded ----\n";
    FEContext&  ctx =   FEContext::instance();

    /// 1) 未压缩扩通道 ktx2 往返:R8(1ch,2级 mip)
    {
        FEImage::CreateInfo c1;
        c1._extent = uint3(8,8,1); c1._format = FMT_R8_UNORM; c1._levels = 2; c1._layers = 1;
        ImagePtr    i1  =   new FEImage(ctx);
        CHECK(i1->create(c1) && i1->size() > 0,                             "save-rt create R8 mip image");
        for (uint32 l = 0; l < 2; ++l)
        {
            auto    pl  =   i1->plane(l,0);
            for (uint32 y = 0; y < pl.height(); ++y)
                for (uint32 x = 0; x < pl.width(); ++x)
                    *pl.dataOffset<uint8>(y,x)  =   uint8(x * 16 + y * 8 + l * 64);
        }
        const char* file    =   "./rt_r8.ktx2";
        CHECK(FEImageHelper::save(*i1,file,0),                              "save R8 mip -> ktx2 ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr && back->cInfo()._format == FMT_R8_UNORM
              && back->cInfo()._levels == 2,                                "load R8 ktx2 -> R8 with 2 levels");
        bool    ok  =   back != nullptr;
        for (uint32 l = 0; ok && l < 2; ++l)
        {
            auto    pa  =   i1->plane(l,0);
            auto    pb  =   back->plane(l,0);
            for (uint32 y = 0; ok && y < pa.height(); ++y)
                for (uint32 x = 0; ok && x < pa.width(); ++x)
                    ok  =   *pa.dataOffset<uint8>(y,x) == *pb.dataOffset<uint8>(y,x);
        }
        CHECK(ok,                                                           "R8 ktx2 roundtrip pixel-identical");
        std::remove(file);
    }

    /// 2) 未压缩扩通道 ktx2 往返:RGB8(3ch) + RGBA16(16bit UNORM)
    {
        /// RGB8:逐像素 3 通道比对(vkFormat=23)
        FEImage::CreateInfo c3;
        c3._extent = uint3(8,8,1); c3._format = FMT_R8G8B8_UNORM; c3._levels = 1; c3._layers = 1;
        ImagePtr    i3  =   new FEImage(ctx);
        CHECK(i3->create(c3) && i3->size() > 0,                             "save-rt create RGB8 image");
        for (uint32 y = 0; y < 8; ++y)
            for (uint32 x = 0; x < 8; ++x)
            {
                uint8*  px  =   i3->plane(0,0).dataOffset<uint8>(y,x);
                px[0]   =   uint8(x * 31); px[1] = uint8(y * 17); px[2] = uint8((x + y) * 9);
            }
        const char* file    =   "./rt_rgb8.ktx2";
        CHECK(FEImageHelper::save(*i3,file,0),                              "save RGB8 -> ktx2 ok");
        ImagePtr    back    =   FEImageHelper::load(ctx,file);
        CHECK(back != nullptr && back->cInfo()._format == FMT_R8G8B8_UNORM, "load RGB8 ktx2 -> RGB8");
        bool    ok  =   back != nullptr;
        if (ok)
        {
            auto    pa  =   i3->plane(0,0);
            auto    pb  =   back->plane(0,0);
            for (uint32 y = 0; ok && y < 8; ++y)
                for (uint32 x = 0; ok && x < 8; ++x)
                {
                    const uint8*    a   =   pa.dataOffset<uint8>(y,x);
                    const uint8*    b   =   pb.dataOffset<uint8>(y,x);
                    ok  =   a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
                }
        }
        CHECK(ok,                                                           "RGB8 ktx2 roundtrip pixel-identical");
        std::remove(file);

        /// RGBA16:16bit UNORM 逐像素比对(vkFormat=91,每像素 8 字节)
        FEImage::CreateInfo c4;
        c4._extent = uint3(8,8,1); c4._format = FMT_R16G16B16A16_UNORM; c4._levels = 1; c4._layers = 1;
        ImagePtr    i4  =   new FEImage(ctx);
        CHECK(i4->create(c4) && i4->size() > 0,                             "save-rt create RGBA16 image");
        for (uint32 y = 0; y < 8; ++y)
            for (uint32 x = 0; x < 8; ++x)
            {
                uint16* px  =   i4->plane(0,0).dataOffset<uint16>(y,x);
                px[0]   =   uint16(x * 8000 + 1); px[1] = uint16(y * 500 + 2);
                px[2]   =   uint16((x + y) * 300 + 3); px[3] = 0xFFFF;
            }
        const char* file16  =   "./rt_rgba16.ktx2";
        CHECK(FEImageHelper::save(*i4,file16,0),                            "save RGBA16 -> ktx2 ok");
        ImagePtr    back16  =   FEImageHelper::load(ctx,file16);
        CHECK(back16 != nullptr && back16->cInfo()._format == FMT_R16G16B16A16_UNORM,
                                                                            "load RGBA16 ktx2 -> RGBA16");
        ok  =   back16 != nullptr;
        if (ok)
        {
            auto    pa  =   i4->plane(0,0);
            auto    pb  =   back16->plane(0,0);
            for (uint32 y = 0; ok && y < 8; ++y)
                for (uint32 x = 0; ok && x < 8; ++x)
                {
                    const uint16*   a   =   pa.dataOffset<uint16>(y,x);
                    const uint16*   b   =   pb.dataOffset<uint16>(y,x);
                    ok  =   a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
                }
        }
        CHECK(ok,                                                           "RGBA16 ktx2 roundtrip pixel-identical");
        std::remove(file16);
    }

    /// 3) 未知 vkFormat 拒绝:VK_FORMAT_R4G4B4A4_UNORM_PACK16(5)未映射 → nullptr
    {
        const char* file    =   "./rt_unknown_vkfmt.ktx2";
        ktxTextureCreateInfo    ci  =   {};
        ci.vkFormat         =   5;      /// 未映射的打包 16bit 格式
        ci.baseWidth        =   8;
        ci.baseHeight       =   8;
        ci.baseDepth        =   1;
        ci.numDimensions    =   2;
        ci.numLevels        =   1;
        ci.numLayers        =   1;
        ci.numFaces         =   1;
        ci.isArray          =   KTX_FALSE;
        ci.generateMipmaps  =   KTX_FALSE;
        ktxTexture2*    ktex2   =   nullptr;
        bool    ok  =   ktxTexture2_Create(&ci,KTX_TEXTURE_CREATE_ALLOC_STORAGE,&ktex2) == KTX_SUCCESS;
        if (ok)
        {
            std::vector<uint8>  pixels(size_t(8) * 8 * 2,0xAB);
            ok  =   ktxTexture_SetImageFromMemory(ktxTexture(ktex2),0,0,0,pixels.data(),pixels.size()) == KTX_SUCCESS;
        }
        if (ok)
            ok  =   ktxTexture_WriteToNamedFile(ktxTexture(ktex2),file) == KTX_SUCCESS;
        if (ktex2 != nullptr)
            ktxTexture_Destroy(ktxTexture(ktex2));
        CHECK(ok,                                                           "unknown vkFormat ktx2 write ok");
        CHECK(FEImageHelper::load(ctx,file) == nullptr,                     "loadKtx rejects unknown vkFormat(5)");
        std::remove(file);
    }

    /// 4) 块压缩透传:全部 30 个块格式 ktx2 save→load 逐字节往返
    ///    (只验证容器映射与块拷贝,不做 CPU 编解码;尺寸取 4 的倍数规避 floor/ceil 差异)
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
        const char* file    =   "./rt_block.ktx2";
        uint32  passCount   =   0;
        for (FEFormat fmt : blockFormats)
        {
            FEImage::CreateInfo cb;
            cb._extent = uint3(8,8,1); cb._format = fmt; cb._levels = 2; cb._layers = 1;
            ImagePtr    ib  =   new FEImage(ctx);
            bool    ok  =   ib->create(cb) && ib->size() > 0;
            if (ok)
            {
                /// 确定性伪随机字节填满各级块数据(块数据无像素语义)
                for (uint32 l = 0; l < 2; ++l)
                {
                    auto        pl      =   ib->plane(l,0);
                    const auto  be      =   FEFormatHelper::blockExtent(fmt);
                    const uint32    rows=   (pl.height() + be.y - 1) / be.y;
                    uint8*      base    =   pl.rowAt<uint8>(0);
                    const size_t    n   =   size_t(pl.pitch()) * rows;
                    for (size_t i = 0; i < n; ++i)
                        base[i] =   uint8(i * 31 + 7 + l * 101);
                }
                ok  =   FEImageHelper::save(*ib,file,0);
                ImagePtr    back    =   ok ? FEImageHelper::load(ctx,file) : nullptr;
                ok  =   back != nullptr && back->cInfo()._format == fmt && back->cInfo()._levels == 2;
                if (ok)
                {
                    for (uint32 l = 0; ok && l < 2; ++l)
                    {
                        auto        pa      =   ib->plane(l,0);
                        auto        pb      =   back->plane(l,0);
                        /// 块格式按块行遍历(ceil(h/块高)),未压缩退化为像素行
                        const auto  beB     =   FEFormatHelper::blockExtent(fmt);
                        const uint32    rows=   (pa.height() + beB.y - 1) / beB.y;
                        const uint64    rowByte =   pa.bytesOfRow();
                        ok  =   pa.width() == pb.width() && pa.height() == pb.height();
                        for (uint32 r = 0; ok && r < rows; ++r)
                            ok  =   std::memcmp(pa.rowAt<uint8>(r),pb.rowAt<uint8>(r),size_t(rowByte)) == 0;
                    }
                }
            }
            if (ok)
                ++passCount;
            else
                std::cout << "       block roundtrip FAILED at format #" << int(fmt) << "\n";
        }
        std::remove(file);
        CHECK(passCount == 30,                                              "all 30 block formats ktx2 byte-identical roundtrip");
    }

    /// 5) BasisU 转码目标:ETC1S ktx2 → 指定 BC7 / BC1 目标转码
    ///    (常色图,验证格式映射与块字节量;不解码块数据)
    {
        const uint8     kColor[4]   =   { 200,100,50,255 };
        const char*     file    =   "./rt_transcode_bc.ktx2";
        FEImage::CreateInfo ce;
        ce._extent = uint3(8,8,1); ce._format = FMT_R8G8B8A8_UNORM; ce._levels = 2; ce._layers = 1;
        ImagePtr    ic  =   new FEImage(ctx);
        bool    ok  =   ic->create(ce) && ic->size() > 0;
        if (ok)
            for (uint32 l = 0; l < 2; ++l)
            {
                auto    pc  =   ic->plane(l,0);
                for (uint32 y = 0; y < pc.height(); ++y)
                    for (uint32 x = 0; x < pc.width(); ++x)
                        std::memcpy(pc.dataOffset<uint8>(y,x),kColor,4);
            }
        ok  =   ok && FEImageHelper::save(*ic,file,128);
        CHECK(ok,                                                           "ETC1S source ktx2 saved(q=128)");

        /// BC7 目标:8x8 → 2x2 块 × 16B = 64B;4x4 → 16B
        ImagePtr    bc7 =   ok ? FEImageHelper::load(ctx,file,FMT_BC7_UNORM) : nullptr;
        CHECK(bc7 != nullptr && bc7->cInfo()._format == FMT_BC7_UNORM && bc7->cInfo()._levels == 2,
                                                                            "ETC1S transcode to BC7_UNORM");
        CHECK(ok && bc7 != nullptr && bc7->byteOfLevel(0) == 64,            "BC7 8x8 level0 bytes == 64");
        /// BC1 目标:8x8 → 2x2 块 × 8B = 32B;4x4 → 8B
        ImagePtr    bc1 =   ok ? FEImageHelper::load(ctx,file,FMT_BC1_RGB_UNORM) : nullptr;
        CHECK(bc1 != nullptr && bc1->cInfo()._format == FMT_BC1_RGB_UNORM && bc1->cInfo()._levels == 2,
                                                                            "ETC1S transcode to BC1_RGB_UNORM");
        CHECK(ok && bc1 != nullptr && bc1->byteOfLevel(0) == 32,            "BC1 8x8 level0 bytes == 32");
        std::remove(file);
    }
}

/// =====================================================================
/// compress / transcode(块编码 ↔ 解码)全矩阵测试
/// 直编 10 种 + RGB8 3 通道源(常色往返精度,两级 mip 各验)+
/// Basis 间接 10 种(常色往返精度,两级 mip 各验)+
/// quality 分支(直编 HIGHQUAL / Basis qualityLevel 1~255 边界)+
/// 显式单通道解码目标(BC4 → R8_UNORM)+
/// sRGB 变体块字节一致性 + 边界块外推(含 mip1)+ 不支持目标锁定 +
/// 多 layer(直编逐 layer 编解码 / Basis 拒绝)+
/// loadKtx 块容器自动解码集成
/// =====================================================================
/// 常色图(w×h,levels 级 mip,每 level 全部像素同一颜色)
static  ImagePtr    makeConstantImage(FEContext& ctx,uint32 w,uint32 h,FEFormat fmt,
                                      const uint8_t c[4],uint32 levels = 1)
{
    ImagePtr    img =   new FEImage(ctx);
    if  (!img->create({uint3(w,h,1),fmt,uint16_t(1),uint16_t(levels),uint16_t(4)}))
        return  nullptr;
    const uint32    ch  =   FEFormatHelper::channelOf(fmt);
    for (uint32 l = 0; l < levels; ++l)
    {
        auto    sp  =   img->plane(l,0);
        for (uint32 y = 0; y < sp.height(); ++y)
        {
            for (uint32 x = 0; x < sp.width(); ++x)
            {
                uint8_t*    p   =   sp.dataOffset<uint8>(y,x);
                for (uint32 k = 0; k < ch; ++k)
                    p[k]    =   c[k < 4 ? k : 3];
            }
        }
    }
    return  img;
}
/// 校验图像指定 level 的 layer 全部像素为常色(按格式通道数比对,容差 tol)
static  bool    isConstantLevel(const FEImage& img,const uint8_t c[4],int tol,uint32 level = 0,uint32 layer = 0)
{
    const uint32    ch  =   FEFormatHelper::channelOf(img.cInfo()._format);
    auto            sp  =   img.plane(level,layer);
    if  (sp._buffer == nullptr)
        return  false;
    for (uint32 y = 0; y < sp.height(); ++y)
    {
        for (uint32 x = 0; x < sp.width(); ++x)
        {
            const uint8_t*  p   =   sp.dataOffset<uint8>(y,x);
            for (uint32 k = 0; k < ch; ++k)
            {
                const uint8_t   ref =   c[k < 4 ? k : 3];
                const int       err =   std::abs(int(p[k]) - int(ref));
                if (err > tol)
                    return  false;
            }
        }
    }
    return  true;
}
/// 两图 level0 块数据逐字节一致(用于 sRGB 变体 == UNORM 验证)
static  bool    sameBlockBytes(const FEImage& a,const FEImage& b)
{
    if  (a.cInfo()._format == b.cInfo()._format)
        return  false;      /// 应为不同格式变体
    if  (a.byteOfLevel(0) != b.byteOfLevel(0))
        return  false;
    auto    pa  =   a.plane(0,0);
    auto    pb  =   b.plane(0,0);
    return  std::memcmp(pa.dataOffset<uint8>(0,0),
                        pb.dataOffset<uint8>(0,0),a.byteOfLevel(0)) == 0;
}

static  void    testBlockCompressTranscode()
{
    std::cout << "---- testBlockCompressTranscode ----\n";
    FEContext&  ctx =   FEContext::instance();
    const uint8_t   CLR[4]  =   {200,60,130,255};
    const uint8_t   RED[4]  =   {180,30,40,200};

    /// ===== 1) 直编 10 种:常色 8x8(2 级 mip)编码 → 解码回 RGBA8 往返 =====
    {
        ImagePtr    rgba    =   makeConstantImage(ctx,8,8,FMT_R8G8B8A8_UNORM,CLR,2);
        ImagePtr    r8      =   makeConstantImage(ctx,8,8,FMT_R8_UNORM,CLR,2);
        ImagePtr    rg8     =   makeConstantImage(ctx,8,8,FMT_R8G8_UNORM,CLR,2);
        ImagePtr    rgb8    =   makeConstantImage(ctx,8,8,FMT_R8G8B8_UNORM,CLR,2);
        CHECK(rgba != nullptr && r8 != nullptr && rg8 != nullptr && rgb8 != nullptr,
                                                            "compress sources created");

        struct  DirectCase  {   FEFormat    dst;    FEImage*    src;    int tol;    bool rtDefault; const char* name;   };
        /// rtDefault=true:解码用 FMT_NONE(BC4→R8 / BC5→RG8 默认目标,
        /// 单/双通道解码回 RGBA8 时缺失通道为 0,不能按 4 通道常色比对)
        const DirectCase    cases[] =
        {
            { FMT_BC1_RGB_UNORM,    rgba.get(), 9,  false,  "BC1_RGB_UNORM"    },
            { FMT_BC1_RGBA_UNORM,   rgba.get(), 9,  false,  "BC1_RGBA_UNORM"   },
            { FMT_BC2_UNORM,        rgba.get(), 9,  false,  "BC2_UNORM"        },
            { FMT_BC3_UNORM,        rgba.get(), 9,  false,  "BC3_UNORM"        },
            { FMT_BC4_UNORM,        r8.get(),   3,  true,   "BC4_UNORM(R8)"    },
            { FMT_BC5_UNORM,        rg8.get(),  3,  true,   "BC5_UNORM(RG8)"   },
            { FMT_BC1_RGB_SRGB,     rgba.get(), 9,  false,  "BC1_RGB_SRGB"     },
            { FMT_BC1_RGBA_SRGB,    rgba.get(), 9,  false,  "BC1_RGBA_SRGB"    },
            { FMT_BC2_SRGB,         rgba.get(), 9,  false,  "BC2_SRGB"         },
            { FMT_BC3_SRGB,         rgba.get(), 9,  false,  "BC3_SRGB"         },
            /// 3 通道源:compress 缺省 alpha 填 255,解码端 BC1_RGBA alpha 恒 255
            { FMT_BC1_RGBA_UNORM,   rgb8.get(), 9,  false,  "BC1_RGBA_UNORM(RGB8 src)" },
        };
        for (const auto& tc : cases)
        {
            ImagePtr    blk =   rgba ? FEImageHelper::compress(*tc.src,tc.dst) : nullptr;
            CHECK(blk != nullptr && blk->cInfo()._format == tc.dst &&
                  blk->cInfo()._levels == 2,        (std::string)"compress -> " + tc.name);
            if  (blk == nullptr)
                continue;
            /// 8x8 → 2x2 块;BC1/BC4 8B/块,其余 16B/块
            const uint32    blkBytes    =   FEFormatHelper::sizeOf(tc.dst);
            CHECK(blk->byteOfLevel(0) == 4 * blkBytes,
                  (std::string)"byteOfLevel(8x8) == 4 blocks * " + std::to_string(blkBytes) + " [" + tc.name + "]");
            CHECK(blk->byteOfLevel(1) == blkBytes,
                  (std::string)"byteOfLevel(4x4) == 1 block [" + tc.name + "]");
            /// 解码回常色往返,两级 mip 各验一次
            /// (level1 为 4x4 单块,覆盖 mip 链上的单块 gather+外推路径;
            ///  容差:5/6bit 量化 + BC2 alpha 4bit)
            ImagePtr    back    =   FEImageHelper::transcode(*blk,
                                    tc.rtDefault ? FMT_NONE : FMT_R8G8B8A8_UNORM);
            CHECK(back != nullptr,                      (std::string)"transcode back [" + tc.name + "]");
            if  (back != nullptr)
                CHECK(isConstantLevel(*back,CLR,tc.tol) && isConstantLevel(*back,CLR,tc.tol,1),
                      (std::string)"roundtrip constant within +-" + std::to_string(tc.tol)
                      + " [levels 0/1] [" + tc.name + "]");
        }

        /// quality 分支:quality>=160 走 STB_DXT_HIGHQUAL(独立代码路径)
        {
            ImagePtr    blk200  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC3_UNORM,200) : nullptr;
            CHECK(blk200 != nullptr && blk200->byteOfLevel(0) == 64,
                                                        "compress BC3 quality=200(HIGHQUAL) ok");
            if  (blk200 != nullptr)
            {
                ImagePtr    back200 =   FEImageHelper::transcode(*blk200,FMT_R8G8B8A8_UNORM);
                CHECK(back200 != nullptr && isConstantLevel(*back200,CLR,9)
                      && isConstantLevel(*back200,CLR,9,1),
                                                        "BC3 quality=200 roundtrip within +-9 [levels 0/1]");
            }
        }

        /// 显式单通道解码目标:BC4 → FMT_R8_UNORM(isDecodeTarget 白名单路径)
        {
            ImagePtr    blk4    =   r8 ? FEImageHelper::compress(*r8,FMT_BC4_UNORM) : nullptr;
            ImagePtr    backR8  =   blk4 ? FEImageHelper::transcode(*blk4,FMT_R8_UNORM) : nullptr;
            CHECK(backR8 != nullptr && backR8->cInfo()._format == FMT_R8_UNORM
                  && isConstantLevel(*backR8,CLR,3,0) && isConstantLevel(*backR8,CLR,3,1),
                                                        "BC4 -> explicit R8_UNORM roundtrip [levels 0/1]");
        }
    }

    /// ===== 2) 直编:5x3 边界块(外推 gather)编码 → 解码有效区常色 =====
    {
        ImagePtr    rgba    =   makeConstantImage(ctx,5,3,FMT_R8G8B8A8_UNORM,RED,2);
        ImagePtr    bc3     =   rgba ? FEImageHelper::compress(*rgba,FMT_BC3_UNORM) : nullptr;
        /// level0:ceil(5/4)x ceil(3/4) = 2x1 块 = 32B;level1:2x1 → 1 块 = 16B
        CHECK(bc3 != nullptr && bc3->byteOfLevel(0) == 32 && bc3->byteOfLevel(1) == 16,
                                                        "compress 5x3 edge blocks -> BC3 (32B + 16B)");
        if  (bc3 != nullptr)
        {
            ImagePtr    back    =   FEImageHelper::transcode(*bc3,FMT_R8G8B8A8_UNORM);
            CHECK(back != nullptr && isConstantLevel(*back,RED,9)
                  && isConstantLevel(*back,RED,9,1),
                                                        "5x3 BC3 roundtrip valid area within +-9 [levels 0/1]");
        }
    }

    /// ===== 3) Basis 间接 10 种:常色往返 =====
    {
        ImagePtr    rgba    =   makeConstantImage(ctx,8,8,FMT_R8G8B8A8_UNORM,CLR,2);
        ImagePtr    r8      =   makeConstantImage(ctx,8,8,FMT_R8_UNORM,CLR,2);
        ImagePtr    rg8     =   makeConstantImage(ctx,8,8,FMT_R8G8_UNORM,CLR,2);
        struct  BasisCase   {   FEFormat    dst;    FEImage*    src;    int tol;    bool rtDefault; const char* name;   };
        /// ETC1S 走全局端点/选择器码本,常色误差高于块内量化理论值,tol 12;
        /// EAC 单/双通道解码用 FMT_NONE(默认 R8/RG8,同直编理由)
        const BasisCase     cases[] =
        {
            { FMT_BC7_UNORM,            rgba.get(), 6,  false,  "BC7_UNORM(UASTC)"          },
            { FMT_BC7_SRGB,             rgba.get(), 6,  false,  "BC7_SRGB(UASTC)"           },
            { FMT_ASTC_4x4_UNORM,       rgba.get(), 6,  false,  "ASTC_4x4_UNORM(UASTC)"     },
            { FMT_ASTC_4x4_SRGB,        rgba.get(), 6,  false,  "ASTC_4x4_SRGB(UASTC)"      },
            { FMT_ETC2_R8G8B8_UNORM,    rgba.get(), 12, false,  "ETC2_RGB_UNORM(ETC1S)"     },
            { FMT_ETC2_R8G8B8_SRGB,     rgba.get(), 12, false,  "ETC2_RGB_SRGB(ETC1S)"      },
            { FMT_ETC2_R8G8B8A8_UNORM,  rgba.get(), 12, false,  "ETC2_RGBA_UNORM(ETC1S)"    },
            { FMT_ETC2_R8G8B8A8_SRGB,   rgba.get(), 12, false,  "ETC2_RGBA_SRGB(ETC1S)"     },
            { FMT_EAC_R11_UNORM,        r8.get(),   12, true,   "EAC_R11_UNORM(R8)"         },
            { FMT_EAC_R11G11_UNORM,     rg8.get(),  12, true,   "EAC_R11G11_UNORM(RG8)"     },
        };
        for (const auto& tc : cases)
        {
            ImagePtr    blk =   rgba ? FEImageHelper::compress(*tc.src,tc.dst) : nullptr;
            CHECK(blk != nullptr && blk->cInfo()._format == tc.dst,
                                                        (std::string)"compress -> " + tc.name);
            if  (blk == nullptr)
                continue;
            ImagePtr    back    =   FEImageHelper::transcode(*blk,
                                    tc.rtDefault ? FMT_NONE : FMT_R8G8B8A8_UNORM);
            CHECK(back != nullptr,                      (std::string)"transcode back [" + tc.name + "]");
            if  (back != nullptr)
                CHECK(isConstantLevel(*back,CLR,tc.tol) && isConstantLevel(*back,CLR,tc.tol,1),
                      (std::string)"roundtrip constant within +-" + std::to_string(tc.tol)
                      + " [levels 0/1] [" + tc.name + "]");
        }

        /// quality 边界:qualityLevel clamp 到 [1,255],两个极值都要不崩溃且可往返
        {
            ImagePtr    q1  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC7_UNORM,1)   : nullptr;
            ImagePtr    q255    =   rgba ? FEImageHelper::compress(*rgba,FMT_BC7_UNORM,255) : nullptr;
            CHECK(q1 != nullptr && q255 != nullptr,     "compress BC7 quality=1/255 ok");
            ImagePtr    b1  =   q1  ? FEImageHelper::transcode(*q1,FMT_R8G8B8A8_UNORM)  : nullptr;
            ImagePtr    b255    =   q255 ? FEImageHelper::transcode(*q255,FMT_R8G8B8A8_UNORM) : nullptr;
            CHECK(b1 != nullptr && isConstantLevel(*b1,CLR,6) && isConstantLevel(*b1,CLR,6,1),
                                                        "BC7 quality=1 roundtrip within +-6 [levels 0/1]");
            CHECK(b255 != nullptr && isConstantLevel(*b255,CLR,6) && isConstantLevel(*b255,CLR,6,1),
                                                        "BC7 quality=255 roundtrip within +-6 [levels 0/1]");
        }
    }

    /// ===== 4) sRGB 变体与 UNORM 块字节一致(直编 BC1 + Basis BC7 各一对) =====
    {
        ImagePtr    rgba    =   makeConstantImage(ctx,8,8,FMT_R8G8B8A8_UNORM,CLR,1);
        ImagePtr    u1  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC1_RGB_UNORM) : nullptr;
        ImagePtr    s1  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC1_RGB_SRGB)  : nullptr;
        CHECK(u1 != nullptr && s1 != nullptr && sameBlockBytes(*u1,*s1),
                                                        "BC1_RGB sRGB bytes == UNORM bytes");
        ImagePtr    u7  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC7_UNORM) : nullptr;
        ImagePtr    s7  =   rgba ? FEImageHelper::compress(*rgba,FMT_BC7_SRGB)  : nullptr;
        CHECK(u7 != nullptr && s7 != nullptr && sameBlockBytes(*u7,*s7),
                                                        "BC7 sRGB bytes == UNORM bytes");
    }

    /// ===== 5) 不支持目标锁定(返回 nullptr) =====
    {
        ImagePtr    rgba    =   makeConstantImage(ctx,8,8,FMT_R8G8B8A8_UNORM,CLR,1);
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_BC6H_UFLOAT)   == nullptr,   "compress BC6H -> nullptr");
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_BC4_SNORM)     == nullptr,   "compress BC4_SNORM -> nullptr");
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_BC5_SNORM)     == nullptr,   "compress BC5_SNORM -> nullptr");
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_ASTC_6x6_UNORM) == nullptr,  "compress ASTC_6x6 -> nullptr");
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_ASTC_8x8_UNORM) == nullptr,  "compress ASTC_8x8 -> nullptr");
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_R8G8B8A8_UNORM) == nullptr,  "compress non-block dst -> nullptr");
        /// 源通道校验:EAC 单通道要求 R8 / 双通道要求 RG8
        CHECK(rgba == nullptr ||
              FEImageHelper::compress(*rgba,FMT_EAC_R11_UNORM)  == nullptr,
                                                        "compress EAC_R11 from RGBA8 -> nullptr");
        ImagePtr    r8  =   makeConstantImage(ctx,8,8,FMT_R8_UNORM,CLR,1);
        CHECK(r8 == nullptr ||
              FEImageHelper::compress(*r8,FMT_EAC_R11G11_UNORM) == nullptr,
                                                        "compress EAC_R11G11 from R8 -> nullptr");
        /// 源格式校验:浮点源拒绝
        ImagePtr    f32    =   makeConstantImage(ctx,8,8,FMT_R32G32B32A32_FLOAT,CLR,1);
        CHECK(f32 == nullptr ||
              FEImageHelper::compress(*f32,FMT_BC1_RGB_UNORM)   == nullptr,
                                                        "compress from float src -> nullptr");
    }

    /// ===== 6) loadKtx 集成:块容器 + 显式未压缩目标 → 自动解码;FMT_NONE 透传 =====
    {
        const char* file    =   "./rt_bc3.ktx2";
        ImagePtr    rgba    =   makeConstantImage(ctx,8,8,FMT_R8G8B8A8_UNORM,CLR,2);
        ImagePtr    bc3     =   rgba ? FEImageHelper::compress(*rgba,FMT_BC3_UNORM) : nullptr;
        CHECK(bc3 != nullptr && FEImageHelper::save(*bc3,file),
                                                        "save BC3 mip chain to ktx2");
        ImagePtr    decoded =   FEImageHelper::load(ctx,file,FMT_R8G8B8A8_UNORM);
        CHECK(decoded != nullptr && decoded->cInfo()._format == FMT_R8G8B8A8_UNORM &&
              decoded->cInfo()._levels == 2,    "loadKtx block container + explicit RGBA8 -> auto decode");
        if  (decoded != nullptr)
            CHECK(isConstantLevel(*decoded,CLR,9) && isConstantLevel(*decoded,CLR,9,1),
                                                        "auto decoded both levels constant within +-9");
        ImagePtr    passthru=   FEImageHelper::load(ctx,file);
        CHECK(passthru != nullptr && passthru->cInfo()._format == FMT_BC3_UNORM,
                                                        "loadKtx FMT_NONE passes block format through");
        std::remove(file);
    }

    /// ===== 7) 多 layer:直编逐 layer 编解码;Basis 间接路线拒绝多 layer =====
    {
        /// 2 layer 常色图:layer0 = CLR,layer1 = RED(8x8,单级 mip)
        FEImage::CreateInfo ci2;
        ci2._extent = uint3(8,8,1); ci2._format = FMT_R8G8B8A8_UNORM;
        ci2._layers = 2; ci2._levels = 1;
        ImagePtr    two =   new FEImage(ctx);
        bool    ok  =   two != nullptr && two->create(ci2) && two->size() > 0;
        if  (ok)
        {
            const uint8_t*  fillers[2]  =   { CLR,RED };
            for (uint32 layer = 0; ok && layer < 2; ++layer)
            {
                auto    sp  =   two->plane(0,layer);
                for (uint32 y = 0; ok && y < sp.height(); ++y)
                    for (uint32 x = 0; x < sp.width(); ++x)
                        std::memcpy(sp.dataOffset<uint8>(y,x),fillers[layer],4);
            }
        }
        CHECK(ok,                                           "2-layer source image created");

        /// 直编:逐 layer 编码 → 解码回 2 layer RGBA8,各 layer 各自常色
        ImagePtr    blk2    =   ok ? FEImageHelper::compress(*two,FMT_BC3_UNORM) : nullptr;
        CHECK(blk2 != nullptr && blk2->cInfo()._layers == 2,
                                                        "compress 2-layer -> BC3 preserves layers");
        if  (blk2 != nullptr)
        {
            ImagePtr    back2   =   FEImageHelper::transcode(*blk2,FMT_R8G8B8A8_UNORM);
            CHECK(back2 != nullptr && back2->cInfo()._layers == 2
                  && isConstantLevel(*back2,CLR,9) && isConstantLevel(*back2,RED,9,0,1),
                                                        "2-layer BC3 roundtrip per-layer constant within +-9");
        }

        /// Basis 间接路线:layers > 1 必须拒绝(返回 nullptr)
        ImagePtr    b2  =   ok ? FEImageHelper::compress(*two,FMT_BC7_UNORM) : nullptr;
        CHECK(b2 == nullptr,                                "compress BC7 2-layer -> nullptr (Basis rejects)");
    }
}

int     main(int argc,char** argv)
{
    /// 由可执行文件位置定位资源目录: <exeDir>/../data/env/Cannon_Exterior.hdr
    const String    hdrFile =   dirOf(argc > 0 ? argv[0] : nullptr)
                            +   "/../data/env/Cannon_Exterior.hdr";

    testCreate();
    testLevelSizeAndOffset();
    testAlign();
    testPixelLayout();
    testPlane();
#if TEST_WITH_FILL_COLOR
    testFillColor();
    testFillColorValues();
#endif
#if TEST_WITH_PBUFFER
    testFillPBuffer();
#endif
    testFillAllFormats();
    testHelperSaveLoad();
    testHdrAndCubeImage(hdrFile.c_str());
    testExplicitFormat(hdrFile.c_str());
    testKtxLoad();
    testKtxExpanded();
    testBlockCompressTranscode();

    std::cout << "--------------------------------\n";
    if (g_failed == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << g_failed << " TEST(S) FAILED\n";
    return  g_failed == 0 ? 0 : 1;
}

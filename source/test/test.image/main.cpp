
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

    std::cout << "--------------------------------\n";
    if (g_failed == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << g_failed << " TEST(S) FAILED\n";
    return  g_failed == 0 ? 0 : 1;
}

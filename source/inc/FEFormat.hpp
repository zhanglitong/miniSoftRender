#pragma     once
#include    <string>
#include    "FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// _UNORM      无符号整数 归一化：将整数映射到 [ 0.0, 1.0] 范围的浮点数      顶点颜色、法线贴图
    /// _SNORM      有符号整数 归一化：将整数映射到 [-1.0, 1.0] 范围的浮点数      切线空间向量、法线贴图
    /// _USCALED    无符号整数 缩放：  将整数原值转换为浮点数（无范围限制）	      自定义顶点属性，如需要原始值的数据
    /// _SSCALED    有符号整数 缩放：  将整数原值转换为浮点数（基本无范围限制）	  传输需精确计算或范围不限于[-1,1]的数据
    /// _UINT       无符号整数 无转换：数据作为无符号整数传递                     骨骼索引、自定义标识符
    /// _SINT       有符号整数 无转换：数据作为有符号整数传递                     同理，需整型计算的属性
    /// </summary>
    enum    FEFormat : uint16_t
    {
        FMT_NONE            =   0,
        FMT_R8_UNORM            ,
        FMT_R8_SNORM            ,
        FMT_R8_USCALED          ,
        FMT_R8_SSCALED          ,
        FMT_R8_UINT             ,
        FMT_R8_SINT             ,
        FMT_R4G4_UNORM          ,

        FMT_R8G8_UNORM          ,
        FMT_R8G8_SNORM          ,
        FMT_R8G8_USCALED        ,
        FMT_R8G8_SSCALED        ,
        FMT_R8G8_UINT           ,
        FMT_R8G8_SINT           ,

        FMT_R4G4B4A4_UNORM      ,
        FMT_B4G4R4A4_UNORM      ,
        FMT_R5G6B5_UNORM        ,
        FMT_B5G6R5_UNORM        ,
        FMT_R5G5B5A1_UNORM      ,
        FMT_B5G5R5A1_UNORM      ,
        FMT_A1R5G5B5_UNORM      ,

        FMT_R8G8B8_UNORM        ,
        FMT_R8G8B8_SNORM        ,
        FMT_R8G8B8_USCALED      ,
        FMT_R8G8B8_SSCALED      ,
        FMT_R8G8B8_UINT         ,
        FMT_R8G8B8_SINT         ,

        FMT_R8G8B8A8_UNORM      ,
        FMT_R8G8B8A8_SNORM      ,
        FMT_R8G8B8A8_USCALED    ,
        FMT_R8G8B8A8_SSCALED    ,
        FMT_R8G8B8A8_UINT       ,
        FMT_R8G8B8A8_SINT       ,

        FMT_B8G8R8A8_UNORM      ,
        FMT_B8G8R8A8_SNORM      ,
        FMT_B8G8R8A8_USCALED    ,
        FMT_B8G8R8A8_SSCALED    ,
        FMT_B8G8R8A8_UINT       ,
        FMT_B8G8R8A8_SINT       ,

        FMT_A2B10G10R10_UNORM   ,
        FMT_A2B10G10R10_SNORM   ,
        FMT_A2B10G10R10_USCALED ,
        FMT_A2B10G10R10_SSCALED ,
        FMT_A2B10G10R10_UINT    ,
        FMT_A2B10G10R10_SINT    ,

        FMT_R16_UNORM           ,
        FMT_R16_SNORM           ,
        FMT_R16_USCALED         ,
        FMT_R16_SSCALED         ,
        FMT_R16_UINT            ,
        FMT_R16_SINT            ,

        FMT_R16G16_UNORM        ,
        FMT_R16G16_SNORM        ,
        FMT_R16G16_USCALED      ,
        FMT_R16G16_SSCALED      ,
        FMT_R16G16_UINT         ,
        FMT_R16G16_SINT         ,

        FMT_R16G16B16_UNORM     ,
        FMT_R16G16B16_SNORM     ,
        FMT_R16G16B16_USCALED   ,
        FMT_R16G16B16_SSCALED   ,
        FMT_R16G16B16_UINT      ,
        FMT_R16G16B16_SINT      ,

        FMT_R16G16B16A16_UNORM  ,
        FMT_R16G16B16A16_SNORM  ,
        FMT_R16G16B16A16_USCALED,
        FMT_R16G16B16A16_SSCALED,
        FMT_R16G16B16A16_UINT   ,
        FMT_R16G16B16A16_SINT   ,

        FMT_R32_UINT            ,
        FMT_R32_SINT            ,

        FMT_R32G32_UINT         ,
        FMT_R32G32_SINT         ,

        FMT_R32G32B32_UINT      ,
        FMT_R32G32B32_SINT      ,

        FMT_R32G32B32A32_UINT   ,
        FMT_R32G32B32A32_SINT   ,

        FMT_R16_FLOAT           ,
        FMT_R16G16_FLOAT        ,
        FMT_R16G16B16_FLOAT     ,
        FMT_R16G16B16A16_FLOAT  ,

        FMT_R32_FLOAT           ,
        FMT_R32G32_FLOAT        ,
        FMT_R32G32B32_FLOAT     ,
        FMT_R32G32B32A32_FLOAT  ,
        
        FMT_D16_UNORM           ,
        FMT_D32_UNORM           ,
        FMT_D32_S8_UNORM        ,

        /// =====================================================================
        /// 块压缩纹理格式(4x4 块,数据为整块字节,无逐像素访问语义)
        /// 主要来源于 KTX/KTX2 容器透传:加载/保存时块数据原样拷贝,
        /// 解码由 GPU 采样硬件完成,CPU 侧不做块解码
        /// BC 系列只支持桌面端;ETC2/EAC 面向移动端;ASTC 全平台
        /// =====================================================================
        FMT_BC1_RGB_UNORM       ,
        FMT_BC1_RGB_SRGB        ,
        FMT_BC1_RGBA_UNORM      ,
        FMT_BC1_RGBA_SRGB       ,
        FMT_BC2_UNORM           ,
        FMT_BC2_SRGB            ,
        FMT_BC3_UNORM           ,
        FMT_BC3_SRGB            ,
        FMT_BC4_UNORM           ,
        FMT_BC4_SNORM           ,
        FMT_BC5_UNORM           ,
        FMT_BC5_SNORM           ,
        FMT_BC6H_UFLOAT         ,
        FMT_BC6H_SFLOAT         ,
        FMT_BC7_UNORM           ,
        FMT_BC7_SRGB            ,

        FMT_ETC2_R8G8B8_UNORM   ,
        FMT_ETC2_R8G8B8_SRGB    ,
        FMT_ETC2_R8G8B8A8_UNORM ,
        FMT_ETC2_R8G8B8A8_SRGB  ,
        FMT_EAC_R11_UNORM       ,
        FMT_EAC_R11_SNORM       ,
        FMT_EAC_R11G11_UNORM    ,
        FMT_EAC_R11G11_SNORM    ,

        FMT_ASTC_4x4_UNORM      ,
        FMT_ASTC_4x4_SRGB       ,
        FMT_ASTC_6x6_UNORM      ,
        FMT_ASTC_6x6_SRGB       ,
        FMT_ASTC_8x8_UNORM      ,
        FMT_ASTC_8x8_SRGB       ,
        FMT_MAX                 ,
    };

}

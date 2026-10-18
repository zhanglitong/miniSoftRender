#pragma     once

#include    "../../inc/FEImage.h"
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FETexture.h"

namespace   FE
{
    /// <summary>
    /// 纹理创建辅助:把 CPU 图像(FEImage)转成 GPU 纹理(实现在 core/graphic/FETextureHelper.cpp)
    /// 尺寸 / 格式 / mip / 层数(cubemap)全部由图像信息推导,
    /// 像素数据按行重排为紧凑布局后上传(图像行本身按 _align 对齐)
    /// </summary>
    class   FE_API  FETextureHelper
    {
    public:
        /// <summary>
        /// 由图像创建纹理
        /// 支持 单层(2D 纹理) 与 6层(cubemap,要求每面为正方形)两种形态,
        /// 数据按 layer 主序、层内 mip 顺序紧密上传(与 FETexture::CreateInfo 约定一致)
        /// </summary>
        /// <returns>失败返回 nullptr(尺寸/格式非法、层数不支持、GPU 资源创建失败)</returns>
        static  TexPtr  create(FEContext& ctx,const FEImage& image,
                               FEFilter      magFilter   =   FILTER_LINEAR,
                               FEFilter      minFilter   =   FILTER_LINEAR,
                               FEAddressMode addressMode =   ADDRESS_REPEAT);
        /// <summary>
        /// 从文件加载图像并创建纹理(解码见 FEImageHelper::load)
        /// </summary>
        static  TexPtr  load(FEContext& ctx,const char* fileName,
                             FEFilter      magFilter   =   FILTER_LINEAR,
                             FEFilter      minFilter   =   FILTER_LINEAR,
                             FEAddressMode addressMode =   ADDRESS_REPEAT);
        /// <summary>
        /// 由 equirect(经纬度)图像生成 float cubemap 图像:6 层 + 完整 mip 链
        /// (CPU 重投影 + float 域 2x2 盒式滤波,保留 HDR 动态范围)
        /// 输入图像需为浮点格式且通道数 >= 3(如 FEImageHelper::loadHdr 的输出);
        /// 面序 +X,-X,+Y,-Y,+Z,-Z(与 Vulkan/GL 规范一致),每面 level0 为 size x size
        /// </summary>
        /// <param name="size">cubemap 每面边长(像素)</param>
        /// <returns>失败返回 nullptr(尺寸非法 / 输入非浮点格式)</returns>
        static  ImagePtr    cubeImageFromEquirect(FEContext& ctx,const FEImage& equirect,uint size);
        /// <summary>
        /// 由 equirect(经纬度)图像生成 IBL 镜面预滤波 cubemap 纹理:
        /// level0 直接重投影,mip 1..N 为 GGX 重要性采样卷积(Hammersley 低差异序列
        /// + saSample/saTexel mip 偏置降噪;mip m ⇔ roughness = m/(levels-1),
        /// 与 shader 侧 textureLod(R, roughness*maxLod) 及 createBRDFLUT 的 split-sum 配套)。
        /// 常用于环境图(IBL):环境光视为无限远,采样默认使用 CLAMP_TO_EDGE 避免面接缝
        /// </summary>
        static  TexPtr      createCube(FEContext& ctx,const FEImage& equirect,uint size,
                                       FEFilter      magFilter   =   FILTER_LINEAR,
                                       FEFilter      minFilter   =   FILTER_LINEAR,
                                       FEAddressMode addressMode =   ADDRESS_CLAMP_TO_EDGE);
        /// <summary>
        /// 由 equirect(经纬度)图像生成漫反射辐照度 cubemap 纹理(单 mip,float 域):
        /// 先把环境投影为 2 阶球谐(9 系数,标准归一化实 SH),
        /// 再按 Ramamoorthi 余弦卷积解析式(A0=π,A1=2π/3,A2=π/4)逐 texel 求值。
        /// 结果平滑无噪声,专用于 IBL 漫反射 irradiance(配合 createCube 的镜面预滤波)。
        /// 输入需为浮点格式且通道数 >= 3(如 FEImageHelper::loadHdr 的输出)
        /// </summary>
        static  TexPtr      createIrradianceCube(FEContext& ctx,const FEImage& equirect,uint size = 32);
        /// <summary>
        /// IBL 镜面 split-sum 第二项的 BRDF 积分 LUT(Karis 2013,与 glTF-Sample-Viewer 同款):
        ///   输出 RG float 纹理,x=NdotV、y=roughness,
        ///   R=scale、G=offset,shader 侧 specular = prefiltered * (F * R + G)
        /// CPU 版 Hammersley 低差异序列 + GGX 重要性采样解析积分,一次性生成
        /// </summary>
        /// <param name="size">LUT 边长(像素),典型 512</param>
        static  TexPtr      createBRDFLUT(FEContext& ctx,uint size = 512);
        /// <summary>
        /// 程序化影棚环境 cubemap(天光渐变 + 三个柔光箱,带 mip 链)
        /// 不依赖外部 HDR 文件,柔光经 mip 预滤波后呈平滑光带,适合车漆/金属展示
        /// </summary>
        static  TexPtr      createStudioCube(FEContext& ctx,uint size = 256);
    };
}

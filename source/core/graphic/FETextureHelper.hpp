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
        /// 由 equirect(经纬度)图像生成 float cubemap 纹理(内部先 cubeImageFromEquirect 再 create)
        /// 常用于环境图(IBL):环境光视为无限远,采样默认使用 CLAMP_TO_EDGE 避免面接缝
        /// </summary>
        static  TexPtr      createCube(FEContext& ctx,const FEImage& equirect,uint size,
                                       FEFilter      magFilter   =   FILTER_LINEAR,
                                       FEFilter      minFilter   =   FILTER_LINEAR,
                                       FEAddressMode addressMode =   ADDRESS_CLAMP_TO_EDGE);
    };
}

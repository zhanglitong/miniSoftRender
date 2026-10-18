#pragma     once
#include    "../inc/FEImage.h"
#include    "image/FEBmpWriter.hpp"
#include    "image/FEBmpReader.hpp"

namespace   FE
{
    /// <summary>
    /// 图像读写辅助(stb_image / stb_image_write 实现,实现在 core/FEImageHelper.cpp)
    /// 加载:普通图片(LDR)解码为 RGBA8,HDR 图片解码为 RGBA float,由 load 自动识别;
    /// 调用方可显式指定目标格式(8bit UNORM / 16bit half / 32bit float)
    /// 保存格式由文件扩展名决定
    /// </summary>
    class   FE_API  FEImageHelper
    {
    public:
        /// <summary>
        /// 保存为 BMP(自实现的 BMP 写出器)
        /// </summary>
        static  bool        saveBmp(FEImage& image,const char* fileName);
        /// <summary>
        /// 从文件加载图片(stb_image 支持 png/jpg/bmp/tga/psd/gif/hdr 等)
        /// fmt == FMT_NONE(默认)时按文件类型自动选择:HDR(Radiance RGBE 等)解码为
        /// RGBA float(FMT_R32G32B32A32_FLOAT),其余解码为 RGBA8(FMT_R8G8B8A8_UNORM);
        /// 显式指定 fmt 时按目标格式解码,支持三类逐通道格式(1/2/3/4 通道):
        ///   8bit  UNORM : R8  / R8G8  / R8G8B8  / R8G8B8A8
        ///   16bit half  : R16 / R16G16 / R16G16B16 / R16G16B16A16_FLOAT
        ///   32bit float : R32 / R32G32 / R32G32B32 / R32G32B32A32_FLOAT
        /// 注:stb 无 half 输出接口,半点由 float 经 float16::float2Half 转换写入
        /// 注:3通道格式(RGB8/RGB16F/RGB32F)在 WebGPU 后端不可作为纹理(映射为 Undefined),
        ///     若仅作 CPU 侧用途(如 equirect 转 cube)可安全使用
        /// 行序:第0行在图像顶部(与引擎约定一致),不做上下翻转
        /// </summary>
        /// <returns>失败或目标格式不支持返回 nullptr</returns>
        static  ImagePtr    load(FEContext& ctx,const char* fileName,FEFormat fmt = FMT_NONE);
        /// <summary>
        /// 从文件以浮点解码(stb_image 浮点路径,线性空间)
        /// fmt == FMT_NONE(默认)时用 RGBA float(FMT_R32G32B32A32_FLOAT);
        /// 也可显式指定浮点系格式(1/2/3/4 通道):
        ///   16bit half  : R16 / R16G16 / R16G16B16 / R16G16B16A16_FLOAT
        ///   32bit float : R32 / R32G32 / R32G32B32 / R32G32B32A32_FLOAT
        /// 指定非浮点系格式(如 UNORM / 整型)返回 nullptr
        /// 行序:第0行在图像顶部(与引擎约定一致),不做上下翻转
        /// 常见用途:equirect(经纬度)环境图,可再经 FETextureHelper 转成 cubemap
        /// </summary>
        /// <returns>失败返回 nullptr</returns>
        static  ImagePtr    loadHdr(FEContext& ctx,const char* fileName,FEFormat fmt = FMT_NONE);
        /// <summary>
        /// 保存图片,格式由扩展名决定:png / jpg(jpeg) / bmp / tga
        /// 仅支持每通道1字节的格式(R8 / R8G8 / R8G8B8 / R8G8B8A8 及其整型/归一化变体);
        /// 浮点/16/32位/位打包格式返回false
        /// 图像行按 _align 对齐时,会自动重排为紧凑行写入
        /// </summary>
        /// <param name="quality">jpg 质量(1~100),其它格式忽略</param>
        /// <returns>写入成功返回 true</returns>
        static  bool        save(const FEImage& image,const char* fileName,int quality = 90);
    protected:
        /// <summary>
        /// 按格式创建单层单mip图像
        /// </summary>
        static  ImagePtr    createImage(FEContext& ctx,uint32 width,uint32 height,FEFormat fmt);
        /// <summary>
        /// 取文件扩展名(小写,不含'.')
        /// </summary>
        static  String      extensionOf(const char* fileName);
    };
}

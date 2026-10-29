#pragma     once
#include    "../inc/FEImage.h"
#include    "image/FEBmpWriter.hpp"
#include    "image/FEBmpReader.hpp"

namespace   FE
{
    /// <summary>
    /// 图像读写辅助(stb_image / stb_image_write 实现,实现在 core/FEImageHelper.cpp;
    /// KTX/KTX2 由 external/ktx 的 libktx 静态库实现)
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
        /// 从文件加载 KTX/KTX2 纹理(external/ktx libktx 静态库),三条路线:
        /// - BasisU/ETC1S/UASTC 压缩块:经 libktx transcode 到目标格式
        ///   (fmt 为 FMT_NONE/R8G8B8A8 时转 RGBA32;也可指定 BC1_RGB/BC3/BC4/
        ///   BC5/BC7/ETC2_R8G8B8/ETC2_R8G8B8A8/EAC_R11/EAC_R11G11/ASTC_4x4 等
        ///   UNORM 目标,ktx2 专属);
        /// - 容器透传:vkFormat/glInternalformat 可映射的未压缩(8/16bit UNORM)
        ///   与块压缩(BC1~BC7/ETC2/EAC/ASTC 4x4~8x8)格式原样拷贝,
        ///   不做 CPU 块解码;sRGB 容器统一映射到宿主 UNORM 格式;
        /// - 未压缩逐通道:R8/R8G8/R8G8B8/R8G8B8A8 + 16bit UNORM
        /// 显式指定 fmt 时必须与容器格式(或转码目标)一致,否则返回 nullptr
        /// - 支持 mip 链:KTX levels 逐层写入 FEImage._levels;
        ///   cube(6 面完整支持)/array 暂只取第 0 层
        /// 行序:按写入端顺序原样拷贝,保证第 0 行在图像顶部(引擎约定)
        /// </summary>
        /// <returns>失败或格式不支持返回 nullptr</returns>
        static  ImagePtr    loadKtx(FEContext& ctx,const char* fileName,FEFormat fmt = FMT_NONE);
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
        /// 从内存中的(压缩)图片字节解码(接口与 load 一致,用于纹理来源对象
        /// FEFileBuffer 的重新解码,避免为每张贴图长期保留展开后的像素)
        /// fmt == FMT_NONE(默认)按内容自动选择:HDR 解码为 RGBA float,其余为 RGBA8
        /// </summary>
        /// <returns>失败或目标格式不支持返回 nullptr</returns>
        static  ImagePtr    loadFromMemory(FEContext& ctx,const void* data,size_t size,FEFormat fmt = FMT_NONE);
        /// <summary>
        /// 保存图片,格式由扩展名决定:png / jpg(jpeg) / bmp / tga / ktx / ktx2
        /// 8bit 格式(R8 / R8G8 / R8G8B8 / R8G8B8A8):
        ///   png/jpg/bmp/tga → stb_image_write,ktx → KTX1(GL 未压缩容器),
        ///   ktx2 → KTX2(UNORM 容器,quality>0 时可经
        ///   ktxTexture2_CompressBasis 转 ETC1S 压缩)
        /// 16bit UNORM(R16/R16G16/R16G16B16/R16G16B16A16):仅 ktx2
        /// 块压缩格式(BC1~BC7/ETC2/EAC/ASTC):仅 ktx2,块数据原样透传直写,
        ///   不做 CPU 压缩编码,quality>0 也不再触发 Basis 压缩
        /// 浮点/32位/其它整型格式返回 false;图像多 level 完整写入 mip 链;
        /// layers/cube/array 暂只写 layer0 face0(2D 单 layer 完整支持)
        /// 图像行按 _align 对齐时,会自动重排为紧凑行写入
        /// </summary>
        /// <param name="quality">jpg(jpeg)质量 / ktx2 ETC1S 压缩质量(0~255,128=中等);其它格式忽略</param>
        /// <returns>写入成功返回 true</returns>
        static  bool        save(const FEImage& image,const char* fileName,int quality = 90);
        /// <summary>
        /// 转码(压缩块 → 未压缩):CPU 逐块解码,支持全部 30 种块压缩格式
        /// (BC1~BC7+BC6H 经 bcdec;ETC2/EAC/ASTC 4x4/6x6/8x8 经自研解码器)
        /// fmt == FMT_NONE(默认)按源格式选择:BC4/BC5/EAC → R8/RG8(SNORM 对应
        /// R8_SNORM/RG8_SNORM);BC6H → RGBA32 float;其余(含 sRGB 变体)→
        /// RGBA8 UNORM(sRGB 不做伽马变换,数值直出)
        /// 显式 fmt 支持:8bit UNORM(R8/RG8/RGB8/RGBA8)、R8/RG8 SNORM、
        /// 16bit half / 32bit float 全通道族;BC4/BC5 → 同量化语义的 R8/RG8 为
        /// 无损字节直出,其余路线经 float 重打包
        /// mip 链与 layers 逐层转换;非法块按规范输出 error color
        /// 源不是块压缩格式或目标格式不支持返回 nullptr
        /// </summary>
        static  ImagePtr    transcode(FEImage& src,FEFormat fmt = FMT_NONE);
        /// <summary>
        /// 压缩编码(未压缩 → 块压缩):CPU 编码,支持 20 种目标格式
        /// 直编(stb_dxt + 自研 BC2 显式 alpha):BC1_RGB/BC1_RGBA/BC2/BC3
        /// (含 sRGB 变体)与 BC4/BC5(仅 UNORM);
        /// Basis 间接路线(libktx:先 ETC1S/UASTC 编码再转码为目标块):
        ///   BC7 / ASTC 4x4(UASTC)、ETC2_RGB / ETC2_RGBA(ETC1S,含 sRGB 变体)、
        ///   EAC_R11(要求源为 R8)、EAC_R11G11(要求源为 RG8)
        /// sRGB 目标变体块字节与 UNORM 相同,直接复用编码结果
        /// 不支持:BC6H、全部 *_SNORM、ASTC 6x6/8x8(Basis 无对应转码目标)
        /// 源限 8bit UNORM(R8/RG8/RGB8/RGBA8);缺失通道按 i==3 补 255、
        /// 其余补 0 规整;有损压缩,结果不保证逐位可逆
        /// quality:1~255,>=160 启用高质量模式(stb 双轮细化 / Basis 高质量转码)
        /// mip 链与 layers 逐层编码;失败返回 nullptr
        /// </summary>
        static  ImagePtr    compress(FEImage& src,FEFormat dstFmt,int quality = 128);
    protected:
        /// <summary>
        /// 按格式创建单层单mip图像
        /// </summary>
        static  ImagePtr    createImage(FEContext& ctx,uint32 width,uint32 height,FEFormat fmt);
        /// <summary>
        /// 把 stb 解码出的紧凑像素(逐行无 padding)写入新建图像(行按 _align 对齐)
        /// </summary>
        /// <param name="pixels">float 系为 float*,8bit 为 uint8*</param>
        static  ImagePtr    createFromDecoded(FEContext& ctx,const void* pixels,int width,int height,int channel,FEFormat fmt);
        /// <summary>
        /// 取文件扩展名(小写,不含'.')
        /// </summary>
        static  String      extensionOf(const char* fileName);
    };
}

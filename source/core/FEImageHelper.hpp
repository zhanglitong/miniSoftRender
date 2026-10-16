#pragma     once
#include    "../inc/FEImage.h"
#include    "image/FEBmpWriter.hpp"
#include    "image/FEBmpReader.hpp"

namespace   FE
{
    /// <summary>
    /// 图像读写辅助(stb_image / stb_image_write 实现,实现在 core/FEImageHelper.cpp)
    /// 加载统一解码为 RGBA8,保存格式由文件扩展名决定
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
        /// 统一解码为 RGBA8(FMT_R8G8B8A8_UNORM):3通道源补 alpha=255
        /// 行序:第0行在图像顶部(与引擎约定一致),不做上下翻转
        /// </summary>
        /// <returns>失败返回 nullptr</returns>
        static  ImagePtr    load(FEContext& ctx,const char* fileName);
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

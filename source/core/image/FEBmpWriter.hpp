#pragma     once

#include    <fstream>
#include    <vector>
#include    <cstdint>
#include    <stdexcept>
#include    "../../inc/FEImage.h"

namespace   FE
{
#pragma pack(push, 1)
    /// <summary>
    /// BMP 文件头 (14 字节)
    /// </summary>
    struct  BmpFileHeader 
    {
        uint16_t bfType;            /// 固定为 'BM' (0x4D42)
        uint32_t bfSize;            /// 整个文件大小
        uint16_t bfReserved1;       /// 保留，必须为 0
        uint16_t bfReserved2;       /// 保留，必须为 0
        uint32_t bfOffBits;         /// 从文件头到像素数据的偏移量
    };

    /// BMP 信息头 (40 字节)
    struct  BmpInfoHeader 
    {
        uint32_t biSize;            /// 本结构大小，固定为 40
        int32_t  biWidth;           /// 图像宽度 (像素)
        int32_t  biHeight;          /// 图像高度 (像素)，正数表示自底向上
        uint16_t biPlanes;          /// 平面数，必须为 1
        uint16_t biBitCount;        /// 位深度，24 表示真彩色
        uint32_t biCompression;     /// 压缩方式，0 表示不压缩 (BI_RGB)
        uint32_t biSizeImage;       /// 图像数据大小 (字节)，可为 0
        int32_t  biXPelsPerMeter;   /// 水平分辨率 (像素/米)
        int32_t  biYPelsPerMeter;   /// 垂直分辨率 (像素/米)
        uint32_t biClrUsed;         /// 实际使用的颜色数，0 表示 2^biBitCount
        uint32_t biClrImportant;    /// 重要颜色数，0 表示都重要
    };

    typedef struct tagRGBQUAD {
        uint8_t rgbBlue;
        uint8_t rgbGreen;
        uint8_t rgbRed;
        uint8_t rgbReserved;
    } RGBQUAD;
#pragma pack(pop)

    class   FEBmpWriter 
    {
    public:
        /// <summary>
        /// 将 iamge 缓冲区保存为 BMP 文件
        /// </summary>
        /// <param name="fileName">输出文件名</param>
        /// <param name="image"></param>
        /// <returns></returns>
        static bool     save(const char* fileName,const FEImage& image) 
        {
            auto        plane           =   image.plane(0,0);
            uint32_t    nMul            =   FEFormatHelper::sizeOf(image.cInfo()._format);
            int32_t     width           =   image.cInfo()._extent.x;
            int32_t     height          =   image.cInfo()._extent.y;
            /// 计算每行需要的填充字节数 (使行字节数为 4 的倍数)
            /// 每行实际 RGB 字节数
            /// 填充字节数
            auto        rowSize         =   (uint32_t)plane.bytesOfRow();               
            auto        padding         =   (4 - (rowSize % 4)) % 4; 
            auto        dataSize        =   (rowSize + padding) * height;

            // 2. 构建调色板 (256个灰度色)
            RGBQUAD palette[256];
            for (uint32_t i = 0; i < 256; i++) 
            {
                palette[i].rgbRed       = i;   // R = 灰阶值
                palette[i].rgbGreen     = i;   // G = 灰阶值
                palette[i].rgbBlue      = i;   // B = 灰阶值
                palette[i].rgbReserved  = 0;
            }

            uint32_t    paSize          =   (nMul == 1) ? sizeof(palette) : 0;
            /// 填充文件头
            BmpFileHeader fileHeader;
            fileHeader.bfType           =   0x4D42;  // 'BM'
            fileHeader.bfSize           =   sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) +  paSize + dataSize;
            fileHeader.bfReserved1      =   0;
            fileHeader.bfReserved2      =   0;
            fileHeader.bfOffBits        =   sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) +  paSize;

            /// 填充信息头
            BmpInfoHeader infoHeader;
            infoHeader.biSize           =   sizeof(BmpInfoHeader);
            infoHeader.biWidth          =   width;
            infoHeader.biHeight         =   height;         /// 正数表示从底向上，大多数查看器都支持
            infoHeader.biPlanes         =   1;
            infoHeader.biBitCount       =   nMul * 8;
            infoHeader.biCompression    =   0;              /// BI_RGB
            infoHeader.biSizeImage      =   dataSize;
            infoHeader.biXPelsPerMeter  =   0;              /// 可设为默认值 0
            infoHeader.biYPelsPerMeter  =   0;
            infoHeader.biClrUsed        =   nMul == 1 ? 256:0;
            infoHeader.biClrImportant   =   0;

            /// 打开文件 (二进制模式)
            std::ofstream file(fileName, std::ios::binary);
            if (!file) 
            {
                return  false;
            }

            /// 写入文件头
            file.write((const char*)(&fileHeader),  sizeof(fileHeader));
            file.write((const char*)(&infoHeader),  sizeof(infoHeader));
            if (paSize)
                file.write((const char*)(&palette), sizeof(palette));
            char    pad[16] =   {};
            memset(pad,0,sizeof(pad));
            auto    buffer  =   (const char*)image.cInfo()._buffers.front()->cInfo().dataPtr();
            
            for (int y = 0; y < height; ++y) 
            {
                /// 注意: BMP 存储顺序为从最后一行开始 (底部) 到第一行 (顶部)
                /// 而我们的 buffer 通常是从顶向下存储。下面两种方式都可：
                /// 方式1: 将高度设为负值表示从上到下，但为简化，我们显式地反转行顺序。
                /// 为了让生成的 BMP 能被大多数软件正确显示，这里将数据按从底到顶写入。
                auto    srcRow = plane.rowAt<char>(height - 1 - y);
                // 填充字节已在构造时置 0，无需额外处理
                file.write(srcRow,  rowSize);
                if (padding)
                    file.write(pad, padding);
            }
            file.close();

            return  true;
        }
    };
}

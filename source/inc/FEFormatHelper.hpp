#pragma     once
#include    <cassert>
#include    <cstring>
#include    "FEFlags.hpp"
#include    "FEFormat.hpp"
#include    "FEMath.hpp"
#include    "FEMathUtil.hpp"
#include    "FEColor.hpp"

namespace   FE
{
    class   FEFormatHelper
    {
    protected:
        FEFormat    _format;
    public:
        FEFormatHelper(FEFormat fmt)
            :_format(fmt)
        {}
    public:
        /// <summary>
        /// 获取格式需要的内存大小，单位字节
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  uint8_t     sizeOf(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM       :
            case FMT_R8_SNORM       :
            case FMT_R8_USCALED     :
            case FMT_R8_SSCALED     :
            case FMT_R8_UINT        :
            case FMT_R8_SINT        :
            case FMT_R4G4_UNORM     :
                return  1 * sizeof(int8_t);

            case FMT_R8G8_UNORM      :
            case FMT_R8G8_SNORM      :
            case FMT_R8G8_USCALED    :
            case FMT_R8G8_SSCALED    :
            case FMT_R8G8_UINT       :
            case FMT_R8G8_SINT       :
            case FMT_R4G4B4A4_UNORM :
            case FMT_B4G4R4A4_UNORM :
            case FMT_R5G6B5_UNORM   :
            case FMT_B5G6R5_UNORM   :
            case FMT_R5G5B5A1_UNORM :
            case FMT_B5G5R5A1_UNORM :
            case FMT_A1R5G5B5_UNORM :
                return  2 * sizeof(int8_t);

            case FMT_R8G8B8_UNORM   :
            case FMT_R8G8B8_SNORM   :
            case FMT_R8G8B8_USCALED :
            case FMT_R8G8B8_SSCALED :
            case FMT_R8G8B8_UINT    :
            case FMT_R8G8B8_SINT    :
                return  3 * sizeof(int8_t);

            case FMT_R8G8B8A8_UNORM    :  
            case FMT_R8G8B8A8_SNORM    :  
            case FMT_R8G8B8A8_USCALED  :  
            case FMT_R8G8B8A8_SSCALED  :  
            case FMT_R8G8B8A8_UINT     :  
            case FMT_R8G8B8A8_SINT     :  
                return  4 * sizeof(int8_t);
            case FMT_A2B10G10R10_UNORM   :
            case FMT_A2B10G10R10_SNORM   :
            case FMT_A2B10G10R10_USCALED :
            case FMT_A2B10G10R10_SSCALED :
            case FMT_A2B10G10R10_UINT    :
            case FMT_A2B10G10R10_SINT    :
                return  sizeof(int32_t);

            case FMT_R16_UNORM  :
            case FMT_R16_SNORM  :
            case FMT_R16_USCALED:
            case FMT_R16_SSCALED:
            case FMT_R16_UINT   :
            case FMT_R16_SINT   :
                return  sizeof(int16_t);
            case FMT_R16G16_UNORM  :
            case FMT_R16G16_SNORM  :
            case FMT_R16G16_USCALED:
            case FMT_R16G16_SSCALED:
            case FMT_R16G16_UINT   :
            case FMT_R16G16_SINT   :
                return  2 * sizeof(int16_t);
            case FMT_R16G16B16_UNORM   :  
            case FMT_R16G16B16_SNORM   :  
            case FMT_R16G16B16_USCALED :  
            case FMT_R16G16B16_SSCALED :  
            case FMT_R16G16B16_UINT    :  
            case FMT_R16G16B16_SINT    :  
                return 3 * sizeof(int16_t);
            case FMT_R16G16B16A16_UNORM   :  
            case FMT_R16G16B16A16_SNORM   :  
            case FMT_R16G16B16A16_USCALED :  
            case FMT_R16G16B16A16_SSCALED :  
            case FMT_R16G16B16A16_UINT    :  
            case FMT_R16G16B16A16_SINT    :  
                return 4 * sizeof(int16_t);

            case FMT_R32_UINT   :
            case FMT_R32_SINT   :
                return 1 * sizeof(int32_t);
            case FMT_R32G32_UINT  :
            case FMT_R32G32_SINT  :
                return 2 * sizeof(int32_t); 
            case FMT_R32G32B32_UINT :
            case FMT_R32G32B32_SINT :
                return 3 * sizeof(int32_t); 
            case FMT_R32G32B32A32_UINT:
            case FMT_R32G32B32A32_SINT:
                return  4 * sizeof(int32_t);
            case FMT_R16_FLOAT   :
                return  1 * sizeof(float16);
            case FMT_R16G16_FLOAT  :
                return  2 * sizeof(float16);
            case FMT_R16G16B16_FLOAT :
                return  3 * sizeof(float16);
            case FMT_R16G16B16A16_FLOAT:
                return  4 * sizeof(float16);
            case FMT_R32_FLOAT:
                return  1 * sizeof(float);
            case FMT_R32G32_FLOAT:
                return  2 * sizeof(float);
            case FMT_R32G32B32_FLOAT :
                return  3 * sizeof(float);
            case FMT_R32G32B32A32_FLOAT:
                return  4 * sizeof(float);
            case FMT_D16_UNORM  :
                return  1 * sizeof(float16);
            case FMT_D32_UNORM :
                return  1 * sizeof(float32);
            /// 按32位深度 + 8位模板，按8字节对齐
            case FMT_D32_S8_UNORM :
                return  2 * sizeof(float32);

            /// =====================================================================
            /// 块压缩格式:一个"元素"是一个 4x4 像素块,返回每块字节数
            /// 64bit 块(8字节):BC1/BC4/ETC2_RGB/EAC_R11
            /// =====================================================================
            case FMT_BC1_RGB_UNORM       :
            case FMT_BC1_RGB_SRGB        :
            case FMT_BC1_RGBA_UNORM      :
            case FMT_BC1_RGBA_SRGB       :
            case FMT_BC4_UNORM           :
            case FMT_BC4_SNORM           :
            case FMT_ETC2_R8G8B8_UNORM   :
            case FMT_ETC2_R8G8B8_SRGB    :
            case FMT_EAC_R11_UNORM       :
            case FMT_EAC_R11_SNORM       :
                return  8;
            /// 128bit 块(16字节):BC2/BC3/BC5/BC6H/BC7/ETC2_ALPHA/EAC_R11G11/ASTC
            case FMT_BC2_UNORM           :
            case FMT_BC2_SRGB            :
            case FMT_BC3_UNORM           :
            case FMT_BC3_SRGB            :
            case FMT_BC5_UNORM           :
            case FMT_BC5_SNORM           :
            case FMT_BC6H_UFLOAT         :
            case FMT_BC6H_SFLOAT         :
            case FMT_BC7_UNORM           :
            case FMT_BC7_SRGB            :
            case FMT_ETC2_R8G8B8A8_UNORM :
            case FMT_ETC2_R8G8B8A8_SRGB  :
            case FMT_EAC_R11G11_UNORM    :
            case FMT_EAC_R11G11_SNORM    :
            case FMT_ASTC_4x4_UNORM      :
            case FMT_ASTC_4x4_SRGB       :
            case FMT_ASTC_6x6_UNORM      :
            case FMT_ASTC_6x6_SRGB       :
            case FMT_ASTC_8x8_UNORM      :
            case FMT_ASTC_8x8_SRGB       :
                return  16;
            }
            return  1;
        }
        /// <summary>
        /// 是否块压缩格式(4x4 像素块,KTX/DDS 家族;数据为整块字节,无逐像素语义)
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  bool    isBlockFormat(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_BC1_RGB_UNORM       :
            case FMT_BC1_RGB_SRGB        :
            case FMT_BC1_RGBA_UNORM      :
            case FMT_BC1_RGBA_SRGB       :
            case FMT_BC2_UNORM           :
            case FMT_BC2_SRGB            :
            case FMT_BC3_UNORM           :
            case FMT_BC3_SRGB            :
            case FMT_BC4_UNORM           :
            case FMT_BC4_SNORM           :
            case FMT_BC5_UNORM           :
            case FMT_BC5_SNORM           :
            case FMT_BC6H_UFLOAT         :
            case FMT_BC6H_SFLOAT         :
            case FMT_BC7_UNORM           :
            case FMT_BC7_SRGB            :
            case FMT_ETC2_R8G8B8_UNORM   :
            case FMT_ETC2_R8G8B8_SRGB    :
            case FMT_ETC2_R8G8B8A8_UNORM :
            case FMT_ETC2_R8G8B8A8_SRGB  :
            case FMT_EAC_R11_UNORM       :
            case FMT_EAC_R11_SNORM       :
            case FMT_EAC_R11G11_UNORM    :
            case FMT_EAC_R11G11_SNORM    :
            case FMT_ASTC_4x4_UNORM      :
            case FMT_ASTC_4x4_SRGB       :
            case FMT_ASTC_6x6_UNORM      :
            case FMT_ASTC_6x6_SRGB       :
            case FMT_ASTC_8x8_UNORM      :
            case FMT_ASTC_8x8_SRGB       :
                return  true;
            default:
                return  false;
            }
        }
        /// <summary>
        /// 块压缩格式的块尺寸(像素):BC/ETC2/EAC 为 4x4,ASTC 按块型 4x4/6x6/8x8;
        /// 其余格式 1x1
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  uint2   blockExtent(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_ASTC_6x6_UNORM     :
            case FMT_ASTC_6x6_SRGB      :   return  uint2(6,6);
            case FMT_ASTC_8x8_UNORM     :
            case FMT_ASTC_8x8_SRGB      :   return  uint2(8,8);
            default:                        break;
            }
            if (isBlockFormat(fmt))
                return  uint2(4,4);
            return  uint2(1,1);
        }
        /// <summary>
        /// 获取颜色的通道数，R:1,RG:2,RGB:3,RGBA:4;
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  uint8_t     channelOf(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM       :
            case FMT_R8_SNORM       :
            case FMT_R8_USCALED     :
            case FMT_R8_SSCALED     :
            case FMT_R8_UINT        :
            case FMT_R8_SINT        :
                return  1;
            case FMT_R4G4_UNORM     :
                return  2;

            case FMT_R8G8_UNORM         :
            case FMT_R8G8_SNORM         :
            case FMT_R8G8_USCALED       :
            case FMT_R8G8_SSCALED       :
            case FMT_R8G8_UINT          :
            case FMT_R8G8_SINT          :
            case FMT_R4G4B4A4_UNORM     :
            case FMT_B4G4R4A4_UNORM     :
            case FMT_R5G6B5_UNORM       :
            case FMT_B5G6R5_UNORM       :
            case FMT_R5G5B5A1_UNORM     :
            case FMT_B5G5R5A1_UNORM     :
            case FMT_A1R5G5B5_UNORM     :
                return  2;

            case FMT_R8G8B8_UNORM       :
            case FMT_R8G8B8_SNORM       :
            case FMT_R8G8B8_USCALED     :
            case FMT_R8G8B8_SSCALED     :
            case FMT_R8G8B8_UINT        :
            case FMT_R8G8B8_SINT        :
                return  3;

            case FMT_R8G8B8A8_UNORM     :  
            case FMT_R8G8B8A8_SNORM     :  
            case FMT_R8G8B8A8_USCALED   :  
            case FMT_R8G8B8A8_SSCALED   :  
            case FMT_R8G8B8A8_UINT      :  
            case FMT_R8G8B8A8_SINT      :  
                return  4;
            case FMT_A2B10G10R10_UNORM   :
            case FMT_A2B10G10R10_SNORM   :
            case FMT_A2B10G10R10_USCALED :
            case FMT_A2B10G10R10_SSCALED :
            case FMT_A2B10G10R10_UINT    :
            case FMT_A2B10G10R10_SINT    :
                return  4;

            case FMT_R16_UNORM  :
            case FMT_R16_SNORM  :
            case FMT_R16_USCALED:
            case FMT_R16_SSCALED:
            case FMT_R16_UINT   :
            case FMT_R16_SINT   :
                return  1;
            case FMT_R16G16_UNORM  :
            case FMT_R16G16_SNORM  :
            case FMT_R16G16_USCALED:
            case FMT_R16G16_SSCALED:
            case FMT_R16G16_UINT   :
            case FMT_R16G16_SINT   :
                return  2;
            case FMT_R16G16B16_UNORM   :  
            case FMT_R16G16B16_SNORM   :  
            case FMT_R16G16B16_USCALED :  
            case FMT_R16G16B16_SSCALED :  
            case FMT_R16G16B16_UINT    :  
            case FMT_R16G16B16_SINT    :  
                return 3;
            case FMT_R16G16B16A16_UNORM   :  
            case FMT_R16G16B16A16_SNORM   :  
            case FMT_R16G16B16A16_USCALED :  
            case FMT_R16G16B16A16_SSCALED :  
            case FMT_R16G16B16A16_UINT    :  
            case FMT_R16G16B16A16_SINT    :  
                return 4;

            case FMT_R32_UINT   :
            case FMT_R32_SINT   :
                return 1;
            case FMT_R32G32_UINT  :
            case FMT_R32G32_SINT  :
                return 2; 
            case FMT_R32G32B32_UINT :
            case FMT_R32G32B32_SINT :
                return 3; 
            case FMT_R32G32B32A32_UINT:
            case FMT_R32G32B32A32_SINT:
                return  4;
            case FMT_R16_FLOAT   :
                return  1;
            case FMT_R16G16_FLOAT  :
                return  2;
            case FMT_R16G16B16_FLOAT :
                return  3;
            case FMT_R16G16B16A16_FLOAT:
                return  4;
            case FMT_R32_FLOAT:
                return  1;
            case FMT_R32G32_FLOAT:
                return  2;
            case FMT_R32G32B32_FLOAT :
                return  3;
            case FMT_R32G32B32A32_FLOAT:
                return  4;
            case FMT_D16_UNORM  :
                return  1;
            case FMT_D32_UNORM :
                return  1;
            case FMT_D32_S8_UNORM:
                return  2;

            /// 块压缩格式:按语义通道数(GPU 解码前的整块数据无独立分量可寻址)
            case FMT_BC4_UNORM           :
            case FMT_BC4_SNORM           :
            case FMT_EAC_R11_UNORM       :
            case FMT_EAC_R11_SNORM       :
                return  1;
            case FMT_BC5_UNORM           :
            case FMT_BC5_SNORM           :
            case FMT_EAC_R11G11_UNORM    :
            case FMT_EAC_R11G11_SNORM    :
                return  2;
            case FMT_BC1_RGB_UNORM       :
            case FMT_BC1_RGB_SRGB        :
            case FMT_BC6H_UFLOAT         :
            case FMT_BC6H_SFLOAT         :
            case FMT_ETC2_R8G8B8_UNORM   :
            case FMT_ETC2_R8G8B8_SRGB    :
                return  3;
            case FMT_BC1_RGBA_UNORM      :
            case FMT_BC1_RGBA_SRGB       :
            case FMT_BC2_UNORM           :
            case FMT_BC2_SRGB            :
            case FMT_BC3_UNORM           :
            case FMT_BC3_SRGB            :
            case FMT_BC7_UNORM           :
            case FMT_BC7_SRGB            :
            case FMT_ETC2_R8G8B8A8_UNORM :
            case FMT_ETC2_R8G8B8A8_SRGB  :
            case FMT_ASTC_4x4_UNORM      :
            case FMT_ASTC_4x4_SRGB       :
            case FMT_ASTC_6x6_UNORM      :
            case FMT_ASTC_6x6_SRGB       :
            case FMT_ASTC_8x8_UNORM      :
            case FMT_ASTC_8x8_SRGB       :
                return  4;
            }
            return  1;
        }
        /// <summary>
        /// 获取名称
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  auto        enumName(FEFormat fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM               :  return  "R8_UNORM";
            case FMT_R8_SNORM               :  return  "R8_SNORM";
            case FMT_R8_USCALED             :  return  "R8_USCALED";
            case FMT_R8_SSCALED             :  return  "R8_SSCALED";
            case FMT_R8_UINT                :  return  "R8_UINT";
            case FMT_R8_SINT                :  return  "R8_SINT";
            case FMT_R4G4_UNORM             :  return  "R4G4_UNORM";

            case FMT_R8G8_UNORM             :  return  "RG8_UNORM";
            case FMT_R8G8_SNORM             :  return  "RG8_SNORM";
            case FMT_R8G8_USCALED           :  return  "RG8_USCALED";
            case FMT_R8G8_SSCALED           :  return  "RG8_SSCALED";
            case FMT_R8G8_UINT              :  return  "RG8_UINT";
            case FMT_R8G8_SINT              :  return  "RG8_SINT";

            case FMT_R4G4B4A4_UNORM         :  return  "R4G4B4A4_UNORM";
            case FMT_B4G4R4A4_UNORM         :  return  "B4G4R4A4_UNORM";
            case FMT_R5G6B5_UNORM           :  return  "R5G6B5_UNORM";
            case FMT_B5G6R5_UNORM           :  return  "B5G6R5_UNORM";
            case FMT_R5G5B5A1_UNORM         :  return  "R5G5B5A1_UNORM";
            case FMT_B5G5R5A1_UNORM         :  return  "B5G5R5A1_UNORM";
            case FMT_A1R5G5B5_UNORM         :  return  "A1R5G5B5_UNORM";

            case FMT_R8G8B8_UNORM           :  return  "R8G8B8_UNORM";
            case FMT_R8G8B8_SNORM           :  return  "R8G8B8_SNORM";
            case FMT_R8G8B8_USCALED         :  return  "R8G8B8_USCALED";
            case FMT_R8G8B8_SSCALED         :  return  "R8G8B8_SSCALED";
            case FMT_R8G8B8_UINT            :  return  "R8G8B8_UINT";
            case FMT_R8G8B8_SINT            :  return  "R8G8B8_SINT";

            case FMT_R8G8B8A8_UNORM         :  return  "R8G8B8A8_UNORM";
            case FMT_R8G8B8A8_SNORM         :  return  "R8G8B8A8_SNORM";
            case FMT_R8G8B8A8_USCALED       :  return  "R8G8B8A8_USCALED";
            case FMT_R8G8B8A8_SSCALED       :  return  "R8G8B8A8_SSCALED";
            case FMT_R8G8B8A8_UINT          :  return  "R8G8B8A8_UINT";
            case FMT_R8G8B8A8_SINT          :  return  "R8G8B8A8_SINT";

            case FMT_A2B10G10R10_UNORM      :  return  "A2B10G10R10_UNORM";
            case FMT_A2B10G10R10_SNORM      :  return  "A2B10G10R10_SNORM";
            case FMT_A2B10G10R10_USCALED    :  return  "A2B10G10R10_USCALED";
            case FMT_A2B10G10R10_SSCALED    :  return  "A2B10G10R10_SSCALED";
            case FMT_A2B10G10R10_UINT       :  return  "A2B10G10R10_UINT";
            case FMT_A2B10G10R10_SINT       :  return  "A2B10G10R10_SINT";

            case FMT_R16_UNORM              :  return  "R16_UNORM";
            case FMT_R16_SNORM              :  return  "R16_SNORM";
            case FMT_R16_USCALED            :  return  "R16_USCALED";
            case FMT_R16_SSCALED            :  return  "R16_SSCALED";
            case FMT_R16_UINT               :  return  "R16_UINT";
            case FMT_R16_SINT               :  return  "R16_SINT";

            case FMT_R16G16_UNORM           :  return  "R16G16_UNORM";
            case FMT_R16G16_SNORM           :  return  "R16G16_SNORM";
            case FMT_R16G16_USCALED         :  return  "R16G16_USCALED";
            case FMT_R16G16_SSCALED         :  return  "R16G16_SSCALED";
            case FMT_R16G16_UINT            :  return  "R16G16_UINT";
            case FMT_R16G16_SINT            :  return  "R16G16_SINT";

            case FMT_R16G16B16_UNORM        :  return  "R16G16B16_UNORM";
            case FMT_R16G16B16_SNORM        :  return  "R16G16B16_SNORM";
            case FMT_R16G16B16_USCALED      :  return  "R16G16B16_USCALED";
            case FMT_R16G16B16_SSCALED      :  return  "R16G16B16_SSCALED";
            case FMT_R16G16B16_UINT         :  return  "R16G16B16_UINT";
            case FMT_R16G16B16_SINT         :  return  "R16G16B16_SINT";

            case  FMT_R16G16B16A16_UNORM    :  return  "R16G16B16A16_UNORM";
            case  FMT_R16G16B16A16_SNORM    :  return  "R16G16B16A16_SNORM";
            case  FMT_R16G16B16A16_USCALED  :  return  "R16G16B16A16_USCALED";
            case  FMT_R16G16B16A16_SSCALED  :  return  "R16G16B16A16_SSCALED";
            case  FMT_R16G16B16A16_UINT     :  return  "R16G16B16A16_UINT";
            case  FMT_R16G16B16A16_SINT     :  return  "R16G16B16A16_SINT";

            case FMT_R32_UINT               :  return  "R32_UINT";
            case FMT_R32_SINT               :  return  "R32_SINT";

            case FMT_R32G32_UINT            :  return  "R32G32_UINT";
            case FMT_R32G32_SINT            :  return  "R32G32_SINT";

            case FMT_R32G32B32_UINT         :  return  "R32G32B32_UINT";
            case FMT_R32G32B32_SINT         :  return  "R32G32B32_SINT";

            case FMT_R32G32B32A32_UINT      :  return  "R32G32B32A32_UINT";
            case FMT_R32G32B32A32_SINT      :  return  "R32G32B32A32_SINT";

            case FMT_R16_FLOAT              :  return  "R16_FLOAT";
            case FMT_R16G16_FLOAT           :  return  "R16G16_FLOAT";
            case FMT_R16G16B16_FLOAT        :  return  "R16G16B16_FLOAT";
            case FMT_R16G16B16A16_FLOAT     :  return  "R16G16B16A16_FLOAT";

            case FMT_R32_FLOAT              :  return  "R32_FLOAT";
            case FMT_R32G32_FLOAT           :  return  "R32G32_FLOAT";
            case FMT_R32G32B32_FLOAT        :  return  "R32G32B32_FLOAT";
            case FMT_R32G32B32A32_FLOAT     :  return  "R32G32B32A32_FLOAT";

            case FMT_D16_UNORM              :  return  "D16_UNORM";
            case FMT_D32_UNORM              :  return  "D32_UNORM";
            case FMT_D32_S8_UNORM           :  return  "D32_S8_UNORM";

            case FMT_BC1_RGB_UNORM          :  return  "BC1_RGB_UNORM";
            case FMT_BC1_RGB_SRGB           :  return  "BC1_RGB_SRGB";
            case FMT_BC1_RGBA_UNORM         :  return  "BC1_RGBA_UNORM";
            case FMT_BC1_RGBA_SRGB          :  return  "BC1_RGBA_SRGB";
            case FMT_BC2_UNORM              :  return  "BC2_UNORM";
            case FMT_BC2_SRGB               :  return  "BC2_SRGB";
            case FMT_BC3_UNORM              :  return  "BC3_UNORM";
            case FMT_BC3_SRGB               :  return  "BC3_SRGB";
            case FMT_BC4_UNORM              :  return  "BC4_UNORM";
            case FMT_BC4_SNORM              :  return  "BC4_SNORM";
            case FMT_BC5_UNORM              :  return  "BC5_UNORM";
            case FMT_BC5_SNORM              :  return  "BC5_SNORM";
            case FMT_BC6H_UFLOAT            :  return  "BC6H_UFLOAT";
            case FMT_BC6H_SFLOAT            :  return  "BC6H_SFLOAT";
            case FMT_BC7_UNORM              :  return  "BC7_UNORM";
            case FMT_BC7_SRGB               :  return  "BC7_SRGB";

            case FMT_ETC2_R8G8B8_UNORM      :  return  "ETC2_R8G8B8_UNORM";
            case FMT_ETC2_R8G8B8_SRGB       :  return  "ETC2_R8G8B8_SRGB";
            case FMT_ETC2_R8G8B8A8_UNORM    :  return  "ETC2_R8G8B8A8_UNORM";
            case FMT_ETC2_R8G8B8A8_SRGB     :  return  "ETC2_R8G8B8A8_SRGB";
            case FMT_EAC_R11_UNORM          :  return  "EAC_R11_UNORM";
            case FMT_EAC_R11_SNORM          :  return  "EAC_R11_SNORM";
            case FMT_EAC_R11G11_UNORM       :  return  "EAC_R11G11_UNORM";
            case FMT_EAC_R11G11_SNORM       :  return  "EAC_R11G11_SNORM";

            case FMT_ASTC_4x4_UNORM         :  return  "ASTC_4x4_UNORM";
            case FMT_ASTC_4x4_SRGB          :  return  "ASTC_4x4_SRGB";
            case FMT_ASTC_6x6_UNORM         :  return  "ASTC_6x6_UNORM";
            case FMT_ASTC_6x6_SRGB          :  return  "ASTC_6x6_SRGB";
            case FMT_ASTC_8x8_UNORM         :  return  "ASTC_8x8_UNORM";
            case FMT_ASTC_8x8_SRGB          :  return  "ASTC_8x8_SRGB";
            }
            assert(0!=0);
            return  "NONE";
        }
        /// <summary>
        /// 根据名称获取格式
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        static  FEFormat    formatFromName(const char* name)
        {
            if (_stricmp(enumName(      FMT_R8_UNORM            ),    name) == 0)   return  FMT_R8_UNORM            ;
            else if (_stricmp(enumName( FMT_R8_SNORM            ),    name) == 0)   return  FMT_R8_SNORM            ;
            else if (_stricmp(enumName( FMT_R8_USCALED          ),    name) == 0)   return  FMT_R8_USCALED          ;
            else if (_stricmp(enumName( FMT_R8_SSCALED          ),    name) == 0)   return  FMT_R8_SSCALED          ;
            else if (_stricmp(enumName( FMT_R8_UINT             ),    name) == 0)   return  FMT_R8_UINT             ;
            else if (_stricmp(enumName( FMT_R8_SINT             ),    name) == 0)   return  FMT_R8_SINT             ;
            else if (_stricmp(enumName( FMT_R4G4_UNORM          ),    name) == 0)   return  FMT_R4G4_UNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_UNORM          ),    name) == 0)   return  FMT_R8G8_UNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_SNORM          ),    name) == 0)   return  FMT_R8G8_SNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_USCALED        ),    name) == 0)   return  FMT_R8G8_USCALED        ;
            else if (_stricmp(enumName( FMT_R8G8_SSCALED        ),    name) == 0)   return  FMT_R8G8_SSCALED        ;
            else if (_stricmp(enumName( FMT_R8G8_UINT           ),    name) == 0)   return  FMT_R8G8_UINT           ;
            else if (_stricmp(enumName( FMT_R8G8_SINT           ),    name) == 0)   return  FMT_R8G8_SINT           ;
            else if (_stricmp(enumName( FMT_R4G4B4A4_UNORM      ),    name) == 0)   return  FMT_R4G4B4A4_UNORM      ;
            else if (_stricmp(enumName( FMT_B4G4R4A4_UNORM      ),    name) == 0)   return  FMT_B4G4R4A4_UNORM      ;
            else if (_stricmp(enumName( FMT_R5G6B5_UNORM        ),    name) == 0)   return  FMT_R5G6B5_UNORM        ;
            else if (_stricmp(enumName( FMT_B5G6R5_UNORM        ),    name) == 0)   return  FMT_B5G6R5_UNORM        ;
            else if (_stricmp(enumName( FMT_R5G5B5A1_UNORM      ),    name) == 0)   return  FMT_R5G5B5A1_UNORM      ;
            else if (_stricmp(enumName( FMT_B5G5R5A1_UNORM      ),    name) == 0)   return  FMT_B5G5R5A1_UNORM      ;
            else if (_stricmp(enumName( FMT_A1R5G5B5_UNORM      ),    name) == 0)   return  FMT_A1R5G5B5_UNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8_UNORM        ),    name) == 0)   return  FMT_R8G8B8_UNORM        ;
            else if (_stricmp(enumName( FMT_R8G8B8_SNORM        ),    name) == 0)   return  FMT_R8G8B8_SNORM        ;
            else if (_stricmp(enumName( FMT_R8G8B8_USCALED      ),    name) == 0)   return  FMT_R8G8B8_USCALED      ;
            else if (_stricmp(enumName( FMT_R8G8B8_SSCALED      ),    name) == 0)   return  FMT_R8G8B8_SSCALED      ;
            else if (_stricmp(enumName( FMT_R8G8B8_UINT         ),    name) == 0)   return  FMT_R8G8B8_UINT         ;
            else if (_stricmp(enumName( FMT_R8G8B8_SINT         ),    name) == 0)   return  FMT_R8G8B8_SINT         ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_UNORM      ),    name) == 0)   return  FMT_R8G8B8A8_UNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SNORM      ),    name) == 0)   return  FMT_R8G8B8A8_SNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_USCALED    ),    name) == 0)   return  FMT_R8G8B8A8_USCALED    ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SSCALED    ),    name) == 0)   return  FMT_R8G8B8A8_SSCALED    ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_UINT       ),    name) == 0)   return  FMT_R8G8B8A8_UINT       ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SINT       ),    name) == 0)   return  FMT_R8G8B8A8_SINT       ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_UNORM   ),    name) == 0)   return  FMT_A2B10G10R10_UNORM   ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SNORM   ),    name) == 0)   return  FMT_A2B10G10R10_SNORM   ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_USCALED ),    name) == 0)   return  FMT_A2B10G10R10_USCALED ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SSCALED ),    name) == 0)   return  FMT_A2B10G10R10_SSCALED ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_UINT    ),    name) == 0)   return  FMT_A2B10G10R10_UINT    ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SINT    ),    name) == 0)   return  FMT_A2B10G10R10_SINT    ;
            else if (_stricmp(enumName( FMT_R16_UNORM           ),    name) == 0)   return  FMT_R16_UNORM           ;
            else if (_stricmp(enumName( FMT_R16_SNORM           ),    name) == 0)   return  FMT_R16_SNORM           ;
            else if (_stricmp(enumName( FMT_R16_USCALED         ),    name) == 0)   return  FMT_R16_USCALED         ;
            else if (_stricmp(enumName( FMT_R16_SSCALED         ),    name) == 0)   return  FMT_R16_SSCALED         ;
            else if (_stricmp(enumName( FMT_R16_UINT            ),    name) == 0)   return  FMT_R16_UINT            ;
            else if (_stricmp(enumName( FMT_R16_SINT            ),    name) == 0)   return  FMT_R16_SINT            ;
            else if (_stricmp(enumName( FMT_R16G16_UNORM        ),    name) == 0)   return  FMT_R16G16_UNORM        ;
            else if (_stricmp(enumName( FMT_R16G16_SNORM        ),    name) == 0)   return  FMT_R16G16_SNORM        ;
            else if (_stricmp(enumName( FMT_R16G16_USCALED      ),    name) == 0)   return  FMT_R16G16_USCALED      ;
            else if (_stricmp(enumName( FMT_R16G16_SSCALED      ),    name) == 0)   return  FMT_R16G16_SSCALED      ;
            else if (_stricmp(enumName( FMT_R16G16_UINT         ),    name) == 0)   return  FMT_R16G16_UINT         ;
            else if (_stricmp(enumName( FMT_R16G16_SINT         ),    name) == 0)   return  FMT_R16G16_SINT         ;
            else if (_stricmp(enumName( FMT_R16G16B16_UNORM     ),    name) == 0)   return  FMT_R16G16B16_UNORM     ;
            else if (_stricmp(enumName( FMT_R16G16B16_SNORM     ),    name) == 0)   return  FMT_R16G16B16_SNORM     ;
            else if (_stricmp(enumName( FMT_R16G16B16_USCALED   ),    name) == 0)   return  FMT_R16G16B16_USCALED   ;
            else if (_stricmp(enumName( FMT_R16G16B16_SSCALED   ),    name) == 0)   return  FMT_R16G16B16_SSCALED   ;
            else if (_stricmp(enumName( FMT_R16G16B16_UINT      ),    name) == 0)   return  FMT_R16G16B16_UINT      ;
            else if (_stricmp(enumName( FMT_R16G16B16_SINT      ),    name) == 0)   return  FMT_R16G16B16_SINT      ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_UNORM  ),    name) == 0)   return  FMT_R16G16B16A16_UNORM  ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SNORM  ),    name) == 0)   return  FMT_R16G16B16A16_SNORM  ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_USCALED),    name) == 0)   return  FMT_R16G16B16A16_USCALED;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SSCALED),    name) == 0)   return  FMT_R16G16B16A16_SSCALED;
            else if (_stricmp(enumName( FMT_R16G16B16A16_UINT   ),    name) == 0)   return  FMT_R16G16B16A16_UINT   ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SINT   ),    name) == 0)   return  FMT_R16G16B16A16_SINT   ;
            else if (_stricmp(enumName( FMT_R32_UINT            ),    name) == 0)   return  FMT_R32_UINT            ;
            else if (_stricmp(enumName( FMT_R32_SINT            ),    name) == 0)   return  FMT_R32_SINT            ;
            else if (_stricmp(enumName( FMT_R32G32_UINT         ),    name) == 0)   return  FMT_R32G32_UINT         ;
            else if (_stricmp(enumName( FMT_R32G32_SINT         ),    name) == 0)   return  FMT_R32G32_SINT         ;
            else if (_stricmp(enumName( FMT_R32G32B32_UINT      ),    name) == 0)   return  FMT_R32G32B32_UINT      ;
            else if (_stricmp(enumName( FMT_R32G32B32_SINT      ),    name) == 0)   return  FMT_R32G32B32_SINT      ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_UINT   ),    name) == 0)   return  FMT_R32G32B32A32_UINT   ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_SINT   ),    name) == 0)   return  FMT_R32G32B32A32_SINT   ;
            else if (_stricmp(enumName( FMT_R16_FLOAT           ),    name) == 0)   return  FMT_R16_FLOAT           ;
            else if (_stricmp(enumName( FMT_R16G16_FLOAT        ),    name) == 0)   return  FMT_R16G16_FLOAT        ;
            else if (_stricmp(enumName( FMT_R16G16B16_FLOAT     ),    name) == 0)   return  FMT_R16G16B16_FLOAT     ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_FLOAT  ),    name) == 0)   return  FMT_R16G16B16A16_FLOAT  ;
            else if (_stricmp(enumName( FMT_R32_FLOAT           ),    name) == 0)   return  FMT_R32_FLOAT           ;
            else if (_stricmp(enumName( FMT_R32G32_FLOAT        ),    name) == 0)   return  FMT_R32G32_FLOAT        ;
            else if (_stricmp(enumName( FMT_R32G32B32_FLOAT     ),    name) == 0)   return  FMT_R32G32B32_FLOAT     ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_FLOAT  ),    name) == 0)   return  FMT_R32G32B32A32_FLOAT  ;
            else if (_stricmp(enumName( FMT_D16_UNORM           ),    name) == 0)   return  FMT_D16_UNORM           ;
            else if (_stricmp(enumName( FMT_D32_UNORM           ),    name) == 0)   return  FMT_D32_UNORM           ;
            else if (_stricmp(enumName( FMT_D32_S8_UNORM        ),    name) == 0)   return  FMT_D32_S8_UNORM        ;
            else if (_stricmp(enumName( FMT_BC1_RGB_UNORM       ),    name) == 0)   return  FMT_BC1_RGB_UNORM       ;
            else if (_stricmp(enumName( FMT_BC1_RGB_SRGB        ),    name) == 0)   return  FMT_BC1_RGB_SRGB        ;
            else if (_stricmp(enumName( FMT_BC1_RGBA_UNORM      ),    name) == 0)   return  FMT_BC1_RGBA_UNORM      ;
            else if (_stricmp(enumName( FMT_BC1_RGBA_SRGB       ),    name) == 0)   return  FMT_BC1_RGBA_SRGB       ;
            else if (_stricmp(enumName( FMT_BC2_UNORM           ),    name) == 0)   return  FMT_BC2_UNORM           ;
            else if (_stricmp(enumName( FMT_BC2_SRGB            ),    name) == 0)   return  FMT_BC2_SRGB            ;
            else if (_stricmp(enumName( FMT_BC3_UNORM           ),    name) == 0)   return  FMT_BC3_UNORM           ;
            else if (_stricmp(enumName( FMT_BC3_SRGB            ),    name) == 0)   return  FMT_BC3_SRGB            ;
            else if (_stricmp(enumName( FMT_BC4_UNORM           ),    name) == 0)   return  FMT_BC4_UNORM           ;
            else if (_stricmp(enumName( FMT_BC4_SNORM           ),    name) == 0)   return  FMT_BC4_SNORM           ;
            else if (_stricmp(enumName( FMT_BC5_UNORM           ),    name) == 0)   return  FMT_BC5_UNORM           ;
            else if (_stricmp(enumName( FMT_BC5_SNORM           ),    name) == 0)   return  FMT_BC5_SNORM           ;
            else if (_stricmp(enumName( FMT_BC6H_UFLOAT         ),    name) == 0)   return  FMT_BC6H_UFLOAT         ;
            else if (_stricmp(enumName( FMT_BC6H_SFLOAT         ),    name) == 0)   return  FMT_BC6H_SFLOAT         ;
            else if (_stricmp(enumName( FMT_BC7_UNORM           ),    name) == 0)   return  FMT_BC7_UNORM           ;
            else if (_stricmp(enumName( FMT_BC7_SRGB            ),    name) == 0)   return  FMT_BC7_SRGB            ;
            else if (_stricmp(enumName( FMT_ETC2_R8G8B8_UNORM   ),    name) == 0)   return  FMT_ETC2_R8G8B8_UNORM   ;
            else if (_stricmp(enumName( FMT_ETC2_R8G8B8_SRGB    ),    name) == 0)   return  FMT_ETC2_R8G8B8_SRGB    ;
            else if (_stricmp(enumName( FMT_ETC2_R8G8B8A8_UNORM ),    name) == 0)   return  FMT_ETC2_R8G8B8A8_UNORM ;
            else if (_stricmp(enumName( FMT_ETC2_R8G8B8A8_SRGB  ),    name) == 0)   return  FMT_ETC2_R8G8B8A8_SRGB  ;
            else if (_stricmp(enumName( FMT_EAC_R11_UNORM       ),    name) == 0)   return  FMT_EAC_R11_UNORM       ;
            else if (_stricmp(enumName( FMT_EAC_R11_SNORM       ),    name) == 0)   return  FMT_EAC_R11_SNORM       ;
            else if (_stricmp(enumName( FMT_EAC_R11G11_UNORM    ),    name) == 0)   return  FMT_EAC_R11G11_UNORM    ;
            else if (_stricmp(enumName( FMT_EAC_R11G11_SNORM    ),    name) == 0)   return  FMT_EAC_R11G11_SNORM    ;
            else if (_stricmp(enumName( FMT_ASTC_4x4_UNORM      ),    name) == 0)   return  FMT_ASTC_4x4_UNORM      ;
            else if (_stricmp(enumName( FMT_ASTC_4x4_SRGB       ),    name) == 0)   return  FMT_ASTC_4x4_SRGB       ;
            else if (_stricmp(enumName( FMT_ASTC_6x6_UNORM      ),    name) == 0)   return  FMT_ASTC_6x6_UNORM      ;
            else if (_stricmp(enumName( FMT_ASTC_6x6_SRGB       ),    name) == 0)   return  FMT_ASTC_6x6_SRGB       ;
            else if (_stricmp(enumName( FMT_ASTC_8x8_UNORM      ),    name) == 0)   return  FMT_ASTC_8x8_UNORM      ;
            else if (_stricmp(enumName( FMT_ASTC_8x8_SRGB       ),    name) == 0)   return  FMT_ASTC_8x8_SRGB       ;
            assert(0!=0);
            return  FMT_NONE                ;
        }

        /// <summary>
        /// float 分量 → 8bit 无符号归一化分量:clamp 到[0,1]后线性映射到[0,255]
        /// </summary>
        static  inline  uint8_t     f2unorm8(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            return  uint8_t(v * 255.0f + 0.5f);
        }
        /// <summary>
        /// float 分量 → 8bit 有符号归一化分量:clamp 到[-1,1]后线性映射到[-127,127]
        /// </summary>
        static  inline  int8_t      f2snorm8(float v)
        {
            v   =   v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
            return  int8_t(v * 127.0f + (v < 0.0f ? -0.5f : 0.5f));
        }
        /// <summary>
        /// float 分量 → 16bit 无符号归一化分量:clamp 到[0,1]后线性映射到[0,65535]
        /// </summary>
        static  inline  uint16_t    f2unorm16(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            return  uint16_t(v * 65535.0f + 0.5f);
        }
        /// <summary>
        /// float 分量 → 16bit 有符号归一化分量:clamp 到[-1,1]后线性映射到[-32767,32767]
        /// </summary>
        static  inline  int16_t     f2snorm16(float v)
        {
            v   =   v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
            return  int16_t(v * 32767.0f + (v < 0.0f ? -0.5f : 0.5f));
        }
        /// <summary>
        /// float 分量 → 8bit 无符号整数分量(USCALED/UINT):四舍五入并 clamp 到[0,255]
        /// </summary>
        static  inline  uint8_t     f2u8(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v);
            return  uint8_t(v + 0.5f);
        }
        /// <summary>
        /// float 分量 → 8bit 有符号整数分量(SSCALED/SINT):四舍五入并 clamp 到[-128,127]
        /// </summary>
        static  inline  int8_t      f2s8(float v)
        {
            v   =   v < -128.0f ? -128.0f : (v > 127.0f ? 127.0f : v);
            return  int8_t(v + (v < 0.0f ? -0.5f : 0.5f));
        }
        /// <summary>
        /// float 分量 → 16bit 无符号整数分量(USCALED/UINT):四舍五入并 clamp 到[0,65535]
        /// </summary>
        static  inline  uint16_t    f2u16(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 65535.0f ? 65535.0f : v);
            return  uint16_t(v + 0.5f);
        }
        /// <summary>
        /// float 分量 → 16bit 有符号整数分量(SSCALED/SINT):四舍五入并 clamp 到[-32768,32767]
        /// </summary>
        static  inline  int16_t     f2s16(float v)
        {
            v   =   v < -32768.0f ? -32768.0f : (v > 32767.0f ? 32767.0f : v);
            return  int16_t(v + (v < 0.0f ? -0.5f : 0.5f));
        }
        /// <summary>
        /// float 分量 → 10bit 无符号整数分量(USCALED/UINT):四舍五入并 clamp 到[0,1023]
        /// </summary>
        static  inline  uint32_t    f2u10(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 1023.0f ? 1023.0f : v);
            return  uint32_t(v + 0.5f);
        }
        /// <summary>
        /// float 分量 → 2bit 无符号 A 分量(USCALED/UINT):四舍五入并 clamp 到[0,3]
        /// </summary>
        static  inline  uint32_t    f2u2(float v)
        {
            v   =   v < 0.0f ? 0.0f : (v > 3.0f ? 3.0f : v);
            return  uint32_t(v + 0.5f);
        }
        /// <summary>
        /// float 分量 → 10bit 有符号整数分量(SSCALED/SINT):四舍五入并 clamp 到[-512,511]
        /// </summary>
        static  inline  int32_t     f2s10(float v)
        {
            v   =   v < -512.0f ? -512.0f : (v > 511.0f ? 511.0f : v);
            return  int32_t(v + (v < 0.0f ? -0.5f : 0.5f));
        }
        /// <summary>
        /// float 分量 → 2bit 有符号 A 分量(SSCALED/SINT):四舍五入并 clamp 到[-2,1]
        /// </summary>
        static  inline  int32_t     f2s2(float v)
        {
            v   =   v < -2.0f ? -2.0f : (v > 1.0f ? 1.0f : v);
            return  int32_t(v + (v < 0.0f ? -0.5f : 0.5f));
        }

    public:
        /// <summary>
        /// 打包内核:将 float 源数据按每元素 srcComponents 分量,以 dstChannels 个 S 标量
        /// 连续写入输出字节流(源分量不足补 0,超出截断)
        /// </summary>
        template<typename S,typename Conv>
        static  void    packVec(uint8s& out,const float* src,size_t elements,uint dstChannels,uint srcComponents,Conv conv)
        {
            out.resize(elements*(size_t)dstChannels*sizeof(S));
            S*  p   =   reinterpret_cast<S*>(out.data());
            for (size_t e = 0; e < elements; e++)
            {
                const   float*  s   =   src + e*(size_t)srcComponents;
                S*              d   =   p + e*(size_t)dstChannels;
                for (uint c = 0; c < dstChannels; c++)
                    d[c]    =   conv(c < srcComponents ? s[c] : 0.0f);
            }
        }

        /// <summary>
        /// packed A2B10G10R10 打包内核:每元素取 4 分量(不足补 0,超出截断),由组装回调生成 uint32 word
        /// (组装复用 FEMathUtil 的 glm packUnorm3x10_1x2/packSnorm3x10_1x2/packU3x10_1x2/packI3x10_1x2)
        /// </summary>
        template<typename Assemble>
        static  void    packVec1032(uint8s& out,const float* src,size_t elements,uint srcComponents,Assemble assemble)
        {
            out.resize(elements*sizeof(uint32_t));
            uint32_t*   p   =   reinterpret_cast<uint32_t*>(out.data());
            for (size_t e = 0; e < elements; e++)
            {
                const   float*  s   =   src + e*(size_t)srcComponents;
                float   comp[4];
                for (uint c = 0; c < 4; c++)
                    comp[c] =   c < srcComponents ? s[c] : 0.0f;
                p[e]    =   assemble(comp);
            }
        }

        /// <summary>
        /// 打包分发:按目标顶点格式选择标量类型与分量转换器;
        /// 不支持的格式 assert 中断并返回空字节流(Release 退化为返回空)
        /// </summary>
        static  uint8s  packFloatsCore(const float* src,size_t elements,uint srcComponents,FEFormat dst)
        {
            uint8s  out;
            #define FE_PACK_CASE(FMT,S,CONV,N)  case FMT: packVec<S>(out,src,elements,N,srcComponents,CONV); break;
            #define FE_PACK_CASE_1032(FMT,PACK,VEC) \
                case FMT: packVec1032(out,src,elements,srcComponents,[](const float* c) \
                {   return  uint(PACK(VEC(c[0],c[1],c[2],c[3])));   }); break;
            #define FE_PACK_CASE_1032_CVT(FMT,PACK,VEC,CVT10,CVT2) \
                case FMT: packVec1032(out,src,elements,srcComponents,[](const float* c) \
                {   return  uint(PACK(VEC(CVT10(c[0]),CVT10(c[1]),CVT10(c[2]),CVT2(c[3]))));    }); break;
            switch(dst)
            {
            /// 32 位 FLOAT 族:直拷
            FE_PACK_CASE(FMT_R32_FLOAT          ,float  ,[](float v){return v;}          ,1)
            FE_PACK_CASE(FMT_R32G32_FLOAT       ,float  ,[](float v){return v;}          ,2)
            FE_PACK_CASE(FMT_R32G32B32_FLOAT    ,float  ,[](float v){return v;}          ,3)
            FE_PACK_CASE(FMT_R32G32B32A32_FLOAT ,float  ,[](float v){return v;}          ,4)
            /// 16 位 FLOAT 族:half 编码
            FE_PACK_CASE(FMT_R16_FLOAT          ,float16,[](float v){return float16(v);} ,1)
            FE_PACK_CASE(FMT_R16G16_FLOAT       ,float16,[](float v){return float16(v);} ,2)
            FE_PACK_CASE(FMT_R16G16B16_FLOAT    ,float16,[](float v){return float16(v);} ,3)
            FE_PACK_CASE(FMT_R16G16B16A16_FLOAT ,float16,[](float v){return float16(v);} ,4)
            /// 8bit 归一化族
            FE_PACK_CASE(FMT_R8_UNORM           ,uint8_t,f2unorm8 ,1)
            FE_PACK_CASE(FMT_R8G8_UNORM         ,uint8_t,f2unorm8 ,2)
            FE_PACK_CASE(FMT_R8G8B8_UNORM       ,uint8_t,f2unorm8 ,3)
            FE_PACK_CASE(FMT_R8G8B8A8_UNORM     ,uint8_t,f2unorm8 ,4)
            FE_PACK_CASE(FMT_R8_SNORM           ,int8_t ,f2snorm8 ,1)
            FE_PACK_CASE(FMT_R8G8_SNORM         ,int8_t ,f2snorm8 ,2)
            FE_PACK_CASE(FMT_R8G8B8_SNORM       ,int8_t ,f2snorm8 ,3)
            FE_PACK_CASE(FMT_R8G8B8A8_SNORM     ,int8_t ,f2snorm8 ,4)
            /// 16bit 归一化族
            FE_PACK_CASE(FMT_R16_UNORM          ,uint16_t,f2unorm16,1)
            FE_PACK_CASE(FMT_R16G16_UNORM       ,uint16_t,f2unorm16,2)
            FE_PACK_CASE(FMT_R16G16B16_UNORM    ,uint16_t,f2unorm16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_UNORM ,uint16_t,f2unorm16,4)
            FE_PACK_CASE(FMT_R16_SNORM          ,int16_t,f2snorm16,1)
            FE_PACK_CASE(FMT_R16G16_SNORM       ,int16_t,f2snorm16,2)
            FE_PACK_CASE(FMT_R16G16B16_SNORM    ,int16_t,f2snorm16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_SNORM ,int16_t,f2snorm16,4)
            /// 8bit 整数族(USCALED/UINT 与 SSCALED/SINT 分别共用无/有符号转换)
            FE_PACK_CASE(FMT_R8_USCALED         ,uint8_t,f2u8,1)
            FE_PACK_CASE(FMT_R8G8_USCALED       ,uint8_t,f2u8,2)
            FE_PACK_CASE(FMT_R8G8B8_USCALED     ,uint8_t,f2u8,3)
            FE_PACK_CASE(FMT_R8G8B8A8_USCALED   ,uint8_t,f2u8,4)
            FE_PACK_CASE(FMT_R8_UINT            ,uint8_t,f2u8,1)
            FE_PACK_CASE(FMT_R8G8_UINT          ,uint8_t,f2u8,2)
            FE_PACK_CASE(FMT_R8G8B8_UINT        ,uint8_t,f2u8,3)
            FE_PACK_CASE(FMT_R8G8B8A8_UINT      ,uint8_t,f2u8,4)
            FE_PACK_CASE(FMT_R8_SSCALED         ,int8_t ,f2s8,1)
            FE_PACK_CASE(FMT_R8G8_SSCALED       ,int8_t ,f2s8,2)
            FE_PACK_CASE(FMT_R8G8B8_SSCALED     ,int8_t ,f2s8,3)
            FE_PACK_CASE(FMT_R8G8B8A8_SSCALED   ,int8_t ,f2s8,4)
            FE_PACK_CASE(FMT_R8_SINT            ,int8_t ,f2s8,1)
            FE_PACK_CASE(FMT_R8G8_SINT          ,int8_t ,f2s8,2)
            FE_PACK_CASE(FMT_R8G8B8_SINT        ,int8_t ,f2s8,3)
            FE_PACK_CASE(FMT_R8G8B8A8_SINT      ,int8_t ,f2s8,4)
            /// 16bit 整数族
            FE_PACK_CASE(FMT_R16_USCALED        ,uint16_t,f2u16,1)
            FE_PACK_CASE(FMT_R16G16_USCALED     ,uint16_t,f2u16,2)
            FE_PACK_CASE(FMT_R16G16B16_USCALED  ,uint16_t,f2u16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_USCALED,uint16_t,f2u16,4)
            FE_PACK_CASE(FMT_R16_UINT           ,uint16_t,f2u16,1)
            FE_PACK_CASE(FMT_R16G16_UINT        ,uint16_t,f2u16,2)
            FE_PACK_CASE(FMT_R16G16B16_UINT     ,uint16_t,f2u16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_UINT  ,uint16_t,f2u16,4)
            FE_PACK_CASE(FMT_R16_SSCALED        ,int16_t,f2s16,1)
            FE_PACK_CASE(FMT_R16G16_SSCALED     ,int16_t,f2s16,2)
            FE_PACK_CASE(FMT_R16G16B16_SSCALED  ,int16_t,f2s16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_SSCALED,int16_t,f2s16,4)
            FE_PACK_CASE(FMT_R16_SINT           ,int16_t,f2s16,1)
            FE_PACK_CASE(FMT_R16G16_SINT        ,int16_t,f2s16,2)
            FE_PACK_CASE(FMT_R16G16B16_SINT     ,int16_t,f2s16,3)
            FE_PACK_CASE(FMT_R16G16B16A16_SINT  ,int16_t,f2s16,4)
            /// packed A2B10G10R10 族:R/G/B 10bit + A 2bit,单 word(组装复用 glm pack*3x10_1x2)
            FE_PACK_CASE_1032(FMT_A2B10G10R10_UNORM     ,FE::packUnorm3x10_1x2,float4)
            FE_PACK_CASE_1032(FMT_A2B10G10R10_SNORM     ,FE::packSnorm3x10_1x2,float4)
            FE_PACK_CASE_1032_CVT(FMT_A2B10G10R10_USCALED   ,FE::packU3x10_1x2,glm::uvec4,f2u10,f2u2)
            FE_PACK_CASE_1032_CVT(FMT_A2B10G10R10_UINT      ,FE::packU3x10_1x2,glm::uvec4,f2u10,f2u2)
            FE_PACK_CASE_1032_CVT(FMT_A2B10G10R10_SSCALED   ,FE::packI3x10_1x2,glm::ivec4,f2s10,f2s2)
            FE_PACK_CASE_1032_CVT(FMT_A2B10G10R10_SINT      ,FE::packI3x10_1x2,glm::ivec4,f2s10,f2s2)
            default:
                assert(false && "unsupported vertex attribute format");
                break;
            }
            #undef  FE_PACK_CASE
            #undef  FE_PACK_CASE_1032
            #undef  FE_PACK_CASE_1032_CVT
            return  out;
        }
    public:
        /// <summary>
        /// 将扁平 float 顶点数据按目标属性格式打包
        /// (目标分量数多于源补 0,少于源截断;格式不支持时 assert 并返回空)
        /// </summary>
        /// <param name="src">扁平 float 数据,按元素、按分量连续存储</param>
        /// <param name="dst">目标顶点属性格式</param>
        /// <param name="srcComponents">源每元素分量数(POS/NOR=3,UV=2)</param>
        /// <returns>目标格式的字节流;无数据或格式不支持时返回空</returns>
        static  uint8s  packFloats(const floats& src,FEFormat dst,uint srcComponents)
        {
            if (src.empty() || srcComponents == 0)
                return  {};
            return  packFloatsCore(src.data(),src.size()/(size_t)srcComponents,srcComponents,dst);
        }
        /// <summary>
        /// 将 float3 顶点数据按目标属性格式打包(每元素 3 分量)
        /// </summary>
        /// <param name="src">float3 数据</param>
        /// <param name="dst">目标顶点属性格式</param>
        /// <returns>目标格式的字节流;无数据或格式不支持时返回空</returns>
        static  uint8s  packFloats(const float3s& src,FEFormat dst)
        {
            if (src.empty())
                return  {};
            return  packFloatsCore(reinterpret_cast<const float*>(src.data()),src.size(),3,dst);
        }
        /// <summary>
        /// 将 Rgba8 顶点色按目标属性格式打包
        /// (R8G8B8A8_UNORM 走直拷恒等路径,与既有行为字节一致)
        /// </summary>
        /// <param name="src">Rgba8 顶点色数据</param>
        /// <param name="dst">目标顶点属性格式</param>
        /// <returns>目标格式的字节流;无数据或格式不支持时返回空</returns>
        static  uint8s  packColors(const Rgba8s& src,FEFormat dst)
        {
            if (src.empty())
                return  {};
            if (dst == FMT_R8G8B8A8_UNORM)
            {
                uint8s  out(src.size()*sizeof(Rgba8));
                memcpy(out.data(),src.data(),out.size());
                return  out;
            }
            /// 展开为 [0,1] float 后走通用打包(颜色固定 4 分量)
            floats      tmp;
            tmp.reserve(src.size()*4);
            const   uint8_t*    p   =   reinterpret_cast<const uint8_t*>(src.data());
            for (size_t i = 0; i < src.size()*4; i++)
                tmp.push_back(p[i]/255.0f);
            return  packFloatsCore(tmp.data(),src.size(),4,dst);
        }

        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// 块压缩格式:w 以像素计,一行 = ceil(w/块宽) 个块 × 每块字节
        /// </summary>
        /// <param name="w">宽度</param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="fmt">像素格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    pitch(uint32_t w,uint16_t align,FEFormat fmt)
        {
            if (isBlockFormat(fmt))
            {
                const uint2 be  =   blockExtent(fmt);
                w               =   (w + be.x - 1) / be.x;
            }
            return  (w * FEFormatHelper::sizeOf(fmt) + (align - 1)) / align * align;
        }
        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// </summary>
        /// <param name="w"></param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="pixelByte">每个像素的字节数</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    pitch(uint32_t w,uint16_t align,uint16_t pixelByte)
        {
            return  (w * pixelByte + (align - 1)) / align * align;
        }
        /// <summary>
        /// 计算单层(w*h*d,像素单位)数据的大小(单位字节)
        /// 块压缩格式:高(深)度按块行数 ceil(h/块高) 折算
        /// </summary>
        /// <param name="w">宽度(像素)</param>
        /// <param name="h">高度(像素)</param>
        /// <param name="d">深度(像素)</param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="fmt">像素格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    bytesOfPlane(uint32_t w,uint32_t h,uint32_t d,uint32_t align,FEFormat fmt)
        {
            if (isBlockFormat(fmt))
            {
                const uint2 be  =   blockExtent(fmt);
                h               =   (h + be.y - 1) / be.y;
            }
            return  pitch(w,align,fmt) * h * d;
        }
        /// <summary>
        /// 根据图像的大小,mipmap,对齐方式,数据格式,计算需要的空间字节数
        /// </summary>
        /// <param name="extent">长宽高</param>
        /// <param name="levels">mipmap</param>
        /// <param name="align">对齐1,2,4,8,16,32,64...</param>
        /// <param name="fmt">格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    bytesOfLayer(const uint3& extent,uint32_t levels,uint32_t align,FEFormat fmt)
        {
            uint32_t    mips    =   (std::max<uint32_t>)(1,levels);
            uint32_t    nByte   =   0;
            for (uint32_t m = 0; m < mips; ++m)
            {
                const auto    w     =   (std::max<uint32_t>)(extent.x >> m,1);
                const auto    h     =   (std::max<uint32_t>)(extent.y >> m,1);
                const auto    d     =   (std::max<uint32_t>)(extent.z >> m,1);
                nByte               +=   bytesOfPlane(w,h,d,align,fmt);
            }
            return  nByte;
        }
    };
    template<FEFormat fmt>
    struct  TypeOfFormat
    {
        /// <summary>
        /// 标记自身格式,供switch调用点把FMT透传给转换内核
        /// </summary>
        static  constexpr   FEFormat    value   =   fmt;
    };

    /// 特化宏: 每个特化携带存储类型type与自身格式value
    #define FE_TYPE_OF_FORMAT(FMT,T)  template<>  struct TypeOfFormat<FMT>  { using type    =   T;  static  constexpr   FEFormat    value   =   FMT;  };

    FE_TYPE_OF_FORMAT(FMT_R8_UNORM            ,uint8_t)
    FE_TYPE_OF_FORMAT(FMT_R8_SNORM            ,int8_t )
    FE_TYPE_OF_FORMAT(FMT_R8_USCALED          ,uint8_t)
    FE_TYPE_OF_FORMAT(FMT_R8_SSCALED          ,int8_t )
    FE_TYPE_OF_FORMAT(FMT_R8_UINT             ,uint8_t)
    FE_TYPE_OF_FORMAT(FMT_R8_SINT             ,int8_t )
    FE_TYPE_OF_FORMAT(FMT_R4G4_UNORM          ,uint8_t)

    FE_TYPE_OF_FORMAT(FMT_R8G8_UNORM          ,uint8x2)
    FE_TYPE_OF_FORMAT(FMT_R8G8_SNORM          ,int8x2 )
    FE_TYPE_OF_FORMAT(FMT_R8G8_USCALED        ,uint8x2)
    FE_TYPE_OF_FORMAT(FMT_R8G8_SSCALED        ,int8x2 )
    FE_TYPE_OF_FORMAT(FMT_R8G8_UINT           ,uint8x2)
    FE_TYPE_OF_FORMAT(FMT_R8G8_SINT           ,int8x2 )

    FE_TYPE_OF_FORMAT(FMT_R4G4B4A4_UNORM      ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_B4G4R4A4_UNORM      ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_R5G6B5_UNORM        ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_B5G6R5_UNORM        ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_R5G5B5A1_UNORM      ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_B5G5R5A1_UNORM      ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_A1R5G5B5_UNORM      ,uint16_t)

    FE_TYPE_OF_FORMAT(FMT_R8G8B8_UNORM        ,uint8x3)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8_SNORM        ,int8x3 )
    FE_TYPE_OF_FORMAT(FMT_R8G8B8_USCALED      ,uint8x3)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8_SSCALED      ,int8x3 )
    FE_TYPE_OF_FORMAT(FMT_R8G8B8_UINT         ,uint8x3)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8_SINT         ,int8x3 )

    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_UNORM      ,uint8x4)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_SNORM      ,int8x4 )
    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_USCALED    ,uint8x4)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_SSCALED    ,int8x4 )
    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_UINT       ,uint8x4)
    FE_TYPE_OF_FORMAT(FMT_R8G8B8A8_SINT       ,int8x4 )

    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_UNORM   ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_SNORM   ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_USCALED ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_SSCALED ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_UINT    ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_A2B10G10R10_SINT    ,uint32_t)

    FE_TYPE_OF_FORMAT(FMT_R16_UNORM           ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_R16_SNORM           ,int16_t )
    FE_TYPE_OF_FORMAT(FMT_R16_USCALED         ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_R16_SSCALED         ,int16_t )
    FE_TYPE_OF_FORMAT(FMT_R16_UINT            ,uint16_t)
    FE_TYPE_OF_FORMAT(FMT_R16_SINT            ,int16_t )

    FE_TYPE_OF_FORMAT(FMT_R16G16_UNORM        ,uint16x2)
    FE_TYPE_OF_FORMAT(FMT_R16G16_SNORM        ,int16x2 )
    FE_TYPE_OF_FORMAT(FMT_R16G16_USCALED      ,uint16x2)
    FE_TYPE_OF_FORMAT(FMT_R16G16_SSCALED      ,int16x2 )
    FE_TYPE_OF_FORMAT(FMT_R16G16_UINT         ,uint16x2)
    FE_TYPE_OF_FORMAT(FMT_R16G16_SINT         ,int16x2 )

    FE_TYPE_OF_FORMAT(FMT_R16G16B16_UNORM     ,uint16x3)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_SNORM     ,int16x3 )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_USCALED   ,uint16x3)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_SSCALED   ,int16x3 )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_UINT      ,uint16x3)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_SINT      ,int16x3 )

    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_UNORM  ,uint16x4)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_SNORM  ,int16x4 )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_USCALED,uint16x4)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_SSCALED,int16x4 )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_UINT   ,uint16x4)
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_SINT   ,int16x4 )

    FE_TYPE_OF_FORMAT(FMT_R32_UINT            ,uint32_t)
    FE_TYPE_OF_FORMAT(FMT_R32_SINT            ,int32_t )

    FE_TYPE_OF_FORMAT(FMT_R32G32_UINT         ,uint32x2)
    FE_TYPE_OF_FORMAT(FMT_R32G32_SINT         ,int32x2 )

    FE_TYPE_OF_FORMAT(FMT_R32G32B32_UINT      ,uint32x3)
    FE_TYPE_OF_FORMAT(FMT_R32G32B32_SINT      ,int32x3 )

    FE_TYPE_OF_FORMAT(FMT_R32G32B32A32_UINT   ,uint32x4)
    FE_TYPE_OF_FORMAT(FMT_R32G32B32A32_SINT   ,int32x4 )

    FE_TYPE_OF_FORMAT(FMT_R16_FLOAT           ,half    )
    FE_TYPE_OF_FORMAT(FMT_R16G16_FLOAT        ,half2   )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16_FLOAT     ,half3   )
    FE_TYPE_OF_FORMAT(FMT_R16G16B16A16_FLOAT  ,half4   )

    FE_TYPE_OF_FORMAT(FMT_R32_FLOAT           ,float   )
    FE_TYPE_OF_FORMAT(FMT_R32G32_FLOAT        ,float2  )
    FE_TYPE_OF_FORMAT(FMT_R32G32B32_FLOAT     ,float3  )
    FE_TYPE_OF_FORMAT(FMT_R32G32B32A32_FLOAT  ,float4  )

    FE_TYPE_OF_FORMAT(FMT_D16_UNORM           ,float16 )
    FE_TYPE_OF_FORMAT(FMT_D32_UNORM           ,float   )
    FE_TYPE_OF_FORMAT(FMT_D32_S8_UNORM        ,float2  )

    #undef  FE_TYPE_OF_FORMAT

}

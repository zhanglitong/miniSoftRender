#pragma     once
#include    <string>
#include    "FEObject.h"
#include    "FEMath.hpp"
#include    "FEColor.hpp"
#include    "FEFlags.hpp"
#include    "FESmallVector.h"
#include    "FEBuffer.hpp"
#include    "FEFormatHelper.hpp"

#include    "graphic/FEPBuffer.h"
#include    "FEImageSource.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEImage,"{CACB6053-A4DB-4479-9BBD-D9D4433263AD}");

    class   FE_API  FEImage:public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEImage)
    public:
        struct  CreateInfo
        {
            /// <summary>
            /// _extent.x : 宽度
            /// _extent.y : 高度
            /// _extent.z : 深度
            /// </summary>
            uint3           _extent;
            /// <summary>
            /// 数据格式
            /// </summary>
            FEFormat        _format     =   FMT_NONE;
            /// <summary>
            /// 描述数组大小
            /// </summary>
            uint16_t        _layers     =   1;
            /// <summary>
            /// mip级别
            /// </summary>
            uint16_t        _levels     =   1;
            /// <summary>
            /// 用来对其使用
            /// </summary>
            uint16_t        _align      =   4;
            /// <summary>
            /// 一个layer 对应一个FEBuffer，避免申请较大的连续内存，造成申请不到内存的情况
            /// 同时如果动态修改layer的大小，也不会造成全部数据失效的情况
            /// </summary>
            Buffers         _buffers;
        };
        /// <summary>
        /// 图像的最基本单元: [x*y*z*fmt]
        /// 数组[layers] * ( [x*y*z*fmt] + [x>>1*y>>1*z>>1*fmt] + ...]
        /// </summary>
        using  ImgSource    =   FEImageSource;
    public:
        FEImage(FEContext& ctx,uint32 w = 0,uint32 h = 0,FEFormat fmt = FMT_NONE);

        FEImage(const FEImage& other);
        virtual ~FEImage();
        /// <summary>
        /// 获取创建信息
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        inline  auto&   cInfo()
        {
            return  _cInfo;
        }
        /// <summary>
        /// 创建图片
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        inline  bool    create(const FEImage::CreateInfo& info);
        template<class T>
        inline  T*      dataAs()
        {
            return  (T*)_cInfo._buffer;
        }
        template<class T>
        const   T*      dataAs() const
        {
            return  (T*)_cInfo._buffer;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="level">0-mips</param>
        /// <param name="layer">0-layers</param>
        /// <returns>像素数据的内存位置</returns>
        inline  auto    pixel(uint32 layer,uint32 level = 0)
        {
            auto    pixel   =   _cInfo._buffers[layer]->cInfo().dataPtr();
            size_t  nByte   =   offsetOfLevel(level);
            return  pixel + nByte;
        }
        const   auto    pixel(uint32 level,uint32 layer = 0) const
        {
            auto    pixel   =   _cInfo._buffers[layer]->cInfo().dataPtr();
            size_t  nByte   =   offsetOfLevel(level);
            return  pixel + nByte;
        }
        /// <summary>
        /// 获取图片的一个最小图像单元
        /// </summary>
        /// <param name="level">0-mips</param>
        /// <param name="layer">0-levels</param>
        /// <returns></returns>
        ImgSource       plane(uint32 level,uint32 layer = 0) const
        {
            auto            pData   =   _cInfo._buffers[layer]->cInfo().dataPtr();
            const size_t    nByte   =   offsetOfLevel(level);
            const auto      w       =   (std::max<uint32>)(_cInfo._extent.x >> level,1);
            const auto      h       =   (std::max<uint32>)(_cInfo._extent.y >> level,1);
            FEImageSource   data;
            
            data._buffer    =   (uint8*)pixel(level,layer);
            data._width     =   w;
            data._height    =   h;
            data._align     =   _cInfo._align;
            data._format    =   _cInfo._format;
            data.update();
            return  data;
        }
        /// <summary>
        /// 计算单张图的大小(单位字节，包含了mipmap)
        /// </summary>
        /// <returns></returns>
        inline  auto    bytesOfLayer() const
        {
            return  bytesOfLayer(_cInfo._extent,_cInfo._levels,_cInfo._align,_cInfo._format);
        }
        /// <summary>
        /// 计算指定级别数据的大小(单位字节)
        /// </summary>
        /// <param name="level"></param>
        /// <returns></returns>
        inline  uint32  byteOfLevel(uint32 level) const
        {
            const auto    w     =   (std::max<uint32>)(_cInfo._extent.x >> level,1);
            const auto    h     =   (std::max<uint32>)(_cInfo._extent.y >> level,1);
            const auto    d     =   (std::max<uint32>)(_cInfo._extent.z >> level,1);
            return  pitch(w,_cInfo._align,_cInfo._format) * h * d;
        }
        /// <summary>
        /// 返回指定级别数据在整个layer中的偏移量
        /// </summary>
        /// <param name="level">0 ~ mips - 1</param>
        /// <returns></returns>
        inline  uint32  offsetOfLevel(uint32 level) const
        {   
            uint32    offset  =   0;
            for (uint32 i = 1; i < level; i++)
                offset  +=  byteOfLevel(i - 1);
            return  offset;
        }
        /// <summary>
        /// 获取图形占用空间的大小(单位字节)
        /// </summary>
        /// <returns></returns>
        inline  uint64  size() const
        {
            return  calcImageSize(_cInfo);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <typeparam name="TFrom">uint8,uint8x2,uint8x3,uint8x4,uint16,uint16x2,uint16x3,uint16x4,float,float2,float3,float4</typeparam>
        /// <param name="color"></param>
        /// <returns></returns>
        template<class TFrom>
        inline  bool    fill(const TFrom& color)
        {
            switch(_cInfo._format)
            {
            case FMT_R8_UNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM             >::type>(color);
            case FMT_R8_SNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM             >::type>(color);
            case FMT_R8_USCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED           >::type>(color);
            case FMT_R8_SSCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED           >::type>(color);
            case FMT_R8_UINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT              >::type>(color);
            case FMT_R8_SINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT              >::type>(color);
            case FMT_R4G4_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM           >::type>(color);

            case FMT_R8G8_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >::type>(color);
            case FMT_R8G8_SNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >::type>(color);
            case FMT_R8G8_USCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >::type>(color);
            case FMT_R8G8_SSCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >::type>(color);
            case FMT_R8G8_UINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >::type>(color);
            case FMT_R8G8_SINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >::type>(color);

            case FMT_R4G4B4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM       >::type>(color);
            case FMT_B4G4R4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM       >::type>(color);
            case FMT_R5G6B5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM         >::type>(color);
            case FMT_B5G6R5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM         >::type>(color);
            case FMT_R5G5B5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM       >::type>(color);
            case FMT_B5G5R5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM       >::type>(color);
            case FMT_A1R5G5B5_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM       >::type>(color);

            case FMT_R8G8B8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM          >::type>(color);
            case FMT_R8G8B8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM          >::type>(color);
            case FMT_R8G8B8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED        >::type>(color);
            case FMT_R8G8B8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED        >::type>(color);
            case FMT_R8G8B8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT           >::type>(color);
            case FMT_R8G8B8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT           >::type>(color);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM        >::type>(color);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM        >::type>(color);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED      >::type>(color);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED      >::type>(color);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT         >::type>(color);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT         >::type>(color);

            case FMT_A2B10G10R10_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM    >::type>(color);
            case FMT_A2B10G10R10_SNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM    >::type>(color);
            case FMT_A2B10G10R10_USCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED  >::type>(color);
            case FMT_A2B10G10R10_SSCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED  >::type>(color);
            case FMT_A2B10G10R10_UINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT     >::type>(color);
            case FMT_A2B10G10R10_SINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT     >::type>(color);

            case FMT_R16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM            >::type>(color);
            case FMT_R16_SNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM            >::type>(color);
            case FMT_R16_USCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED          >::type>(color);
            case FMT_R16_SSCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED          >::type>(color);
            case FMT_R16_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT             >::type>(color);
            case FMT_R16_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT             >::type>(color);

            case FMT_R16G16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM          >::type>(color);
            case FMT_R16G16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM          >::type>(color);
            case FMT_R16G16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED        >::type>(color);
            case FMT_R16G16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED        >::type>(color);
            case FMT_R16G16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT           >::type>(color);
            case FMT_R16G16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT           >::type>(color);

            case FMT_R16G16B16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM       >::type>(color);
            case FMT_R16G16B16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM       >::type>(color);
            case FMT_R16G16B16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED     >::type>(color);
            case FMT_R16G16B16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED     >::type>(color);
            case FMT_R16G16B16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT        >::type>(color);
            case FMT_R16G16B16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT        >::type>(color);

            case FMT_R16G16B16A16_UNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM    >::type>(color);
            case FMT_R16G16B16A16_SNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM    >::type>(color);
            case FMT_R16G16B16A16_USCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED  >::type>(color);
            case FMT_R16G16B16A16_SSCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED  >::type>(color);
            case FMT_R16G16B16A16_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT     >::type>(color);
            case FMT_R16G16B16A16_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT     >::type>(color);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT             >::type>(color);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT             >::type>(color);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >::type>(color);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >::type>(color);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >::type>(color);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >::type>(color);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >::type>(color);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >::type>(color);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >::type>(color);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >::type>(color);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >::type>(color);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >::type>(color);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >::type>(color);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >::type>(color);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >::type>(color);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >::type>(color);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM            >::type>(color);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM            >::type>(color);
            }
            return  false;
        }
        template<class TFrom>
        inline  bool    fill(const TPBuffer<TFrom>& pBuffer)
        {
            auto    color   =   this->plane(0,0);
            switch(_cInfo._format)
            {
            case FMT_R8_UNORM            :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM             >::type>(pBuffer,color);
            case FMT_R8_SNORM            :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM             >::type>(pBuffer,color);
            case FMT_R8_USCALED          :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED           >::type>(pBuffer,color);
            case FMT_R8_SSCALED          :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED           >::type>(pBuffer,color);
            case FMT_R8_UINT             :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT              >::type>(pBuffer,color);
            case FMT_R8_SINT             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT              >::type>(pBuffer,color);
            case FMT_R4G4_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM           >::type>(pBuffer,color);

            case FMT_RG8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_RG8_UNORM            >::type>(pBuffer,color);
            case FMT_RG8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_RG8_SNORM            >::type>(pBuffer,color);
            case FMT_RG8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_RG8_USCALED          >::type>(pBuffer,color);
            case FMT_RG8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_RG8_SSCALED          >::type>(pBuffer,color);
            case FMT_RG8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_RG8_UINT             >::type>(pBuffer,color);
            case FMT_RG8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_RG8_SINT             >::type>(pBuffer,color);

            case FMT_R4G4B4A4_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM       >::type>(pBuffer,color);
            case FMT_B4G4R4A4_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM       >::type>(pBuffer,color);
            case FMT_R5G6B5_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM         >::type>(pBuffer,color);
            case FMT_B5G6R5_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM         >::type>(pBuffer,color);
            case FMT_R5G5B5A1_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM       >::type>(pBuffer,color);
            case FMT_B5G5R5A1_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM       >::type>(pBuffer,color);
            case FMT_A1R5G5B5_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM       >::type>(pBuffer,color);

            case FMT_RGB8_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_UNORM           >::type>(pBuffer,color);
            case FMT_RGB8_SNORM          :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_SNORM           >::type>(pBuffer,color);
            case FMT_RGB8_USCALED        :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_USCALED         >::type>(pBuffer,color);
            case FMT_RGB8_SSCALED        :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_SSCALED         >::type>(pBuffer,color);
            case FMT_RGB8_UINT           :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_UINT            >::type>(pBuffer,color);
            case FMT_RGB8_SINT           :  return  convert<TFrom,TypeOfFormat<FMT_RGB8_SINT            >::type>(pBuffer,color);

            case FMT_RGBA8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_UNORM          >::type>(pBuffer,color);
            case FMT_RGBA8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_SNORM          >::type>(pBuffer,color);
            case FMT_RGBA8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_USCALED        >::type>(pBuffer,color);
            case FMT_RGBA8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_SSCALED        >::type>(pBuffer,color);
            case FMT_RGBA8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_UINT           >::type>(pBuffer,color);
            case FMT_RGBA8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGBA8_SINT           >::type>(pBuffer,color);

            case FMT_A2B10G10R10_UNORM   :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM    >::type>(pBuffer,color);
            case FMT_A2B10G10R10_SNORM   :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM    >::type>(pBuffer,color);
            case FMT_A2B10G10R10_USCALED :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED  >::type>(pBuffer,color);
            case FMT_A2B10G10R10_SSCALED :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED  >::type>(pBuffer,color);
            case FMT_A2B10G10R10_UINT    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT     >::type>(pBuffer,color);
            case FMT_A2B10G10R10_SINT    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT     >::type>(pBuffer,color);

            case FMT_R16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM            >::type>(pBuffer,color);
            case FMT_R16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM            >::type>(pBuffer,color);
            case FMT_R16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED          >::type>(pBuffer,color);
            case FMT_R16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED          >::type>(pBuffer,color);
            case FMT_R16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT             >::type>(pBuffer,color);
            case FMT_R16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT             >::type>(pBuffer,color);

            case FMT_RG16_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_RG16_UNORM           >::type>(pBuffer,color);
            case FMT_RG16_SNORM          :  return  convert<TFrom,TypeOfFormat<FMT_RG16_SNORM           >::type>(pBuffer,color);
            case FMT_RG16_USCALED        :  return  convert<TFrom,TypeOfFormat<FMT_RG16_USCALED         >::type>(pBuffer,color);
            case FMT_RG16_SSCALED        :  return  convert<TFrom,TypeOfFormat<FMT_RG16_SSCALED         >::type>(pBuffer,color);
            case FMT_RG16_UINT           :  return  convert<TFrom,TypeOfFormat<FMT_RG16_UINT            >::type>(pBuffer,color);
            case FMT_RG16_SINT           :  return  convert<TFrom,TypeOfFormat<FMT_RG16_SINT            >::type>(pBuffer,color);

            case FMT_RGB16_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_UNORM          >::type>(pBuffer,color);
            case FMT_RGB16_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_SNORM          >::type>(pBuffer,color);
            case FMT_RGB16_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_USCALED        >::type>(pBuffer,color);
            case FMT_RGB16_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_SSCALED        >::type>(pBuffer,color);
            case FMT_RGB16_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_UINT           >::type>(pBuffer,color);
            case FMT_RGB16_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGB16_SINT           >::type>(pBuffer,color);

            case FMT_RGBA16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_UNORM         >::type>(pBuffer,color);
            case FMT_RGBA16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_SNORM         >::type>(pBuffer,color);
            case FMT_RGBA16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_USCALED       >::type>(pBuffer,color);
            case FMT_RGBA16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_SSCALED       >::type>(pBuffer,color);
            case FMT_RGBA16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_UINT          >::type>(pBuffer,color);
            case FMT_RGBA16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA16_SINT          >::type>(pBuffer,color);

            case FMT_R32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT             >::type>(pBuffer,color);
            case FMT_R32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT             >::type>(pBuffer,color);

            case FMT_RG32_UINT           :  return  convert<TFrom,TypeOfFormat<FMT_RG32_UINT            >::type>(pBuffer,color);
            case FMT_RG32_SINT           :  return  convert<TFrom,TypeOfFormat<FMT_RG32_SINT            >::type>(pBuffer,color);

            case FMT_RGB32_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGB32_UINT           >::type>(pBuffer,color);
            case FMT_RGB32_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_RGB32_SINT           >::type>(pBuffer,color);

            case FMT_RGBA32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA32_UINT          >::type>(pBuffer,color);
            case FMT_RGBA32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_RGBA32_SINT          >::type>(pBuffer,color);

            case FMT_RF16                :  return  convert<TFrom,TypeOfFormat<FMT_RF16                 >::type>(pBuffer,color);
            case FMT_RGF16               :  return  convert<TFrom,TypeOfFormat<FMT_RGF16                >::type>(pBuffer,color);
            case FMT_RGBF16              :  return  convert<TFrom,TypeOfFormat<FMT_RGBF16               >::type>(pBuffer,color);
            case FMT_RGBAF16             :  return  convert<TFrom,TypeOfFormat<FMT_RGBAF16              >::type>(pBuffer,color);

            case FMT_RF32                :  return  convert<TFrom,TypeOfFormat<FMT_RF32                 >::type>(pBuffer,color);
            case FMT_RGF32               :  return  convert<TFrom,TypeOfFormat<FMT_RGF32                >::type>(pBuffer,color);
            case FMT_RGBF32              :  return  convert<TFrom,TypeOfFormat<FMT_RGBF32               >::type>(pBuffer,color);
            case FMT_RGBAF32             :  return  convert<TFrom,TypeOfFormat<FMT_RGBAF32              >::type>(pBuffer,color);

            case FMT_D16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM            >::type>(pBuffer,color);
            case FMT_D32_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM            >::type>(pBuffer,color);
            }
            return  false;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
    protected:

        /// <summary>
        /// 实现情况缓冲区的作用(用指定的值填充)
        /// </summary>
        /// <typeparam name="TFrom"></typeparam>
        /// <typeparam name="TTo"></typeparam>
        /// <param name="data"></param>
        /// <param name="src"></param>
        /// <returns></returns>
        template<class TFrom,class TTo>
        inline  bool    convert(const TFrom& src)
        {
            size_t      nSize       =   0;
            uint32      mips        =   (std::max<uint32>)(1,_cInfo._levels);
            uint32      layer       =   (std::max<uint32>)(1,_cInfo._layers);
            TTo         tSrc        =   castTo<TFrom,TTo>(src);
            /// 一个像素的字节数
            const auto  pixelSize   =   FEFormatHelper::sizeOf(_cInfo._format);
            const auto  align       =   _cInfo._align;
            for (uint32 l = 0; l < layer; ++l)
            {
                /// 数据的首地址
                auto        pStart      =   _cInfo._buffers[l]->cInfo().dataPtr();
                for (uint32 m = 0; m < mips; ++m)
                {
                    const auto  width   =   (std::max<uint32>)(_cInfo._extent.x >> m,1);
                    const auto  height  =   (std::max<uint32>)(_cInfo._extent.y >> m,1);
                    const auto  depth   =   (std::max<uint32>)(_cInfo._extent.z >> m,1);
                    /// 一行数据大小
                    const auto  nPitch  =   pitch(width,align,pixelSize) ;
                    for (uint32 d = 0; d < depth;  ++d)
                    {
                        for (uint32 h = 0; h < height; ++h)
                        {   
                            TTo*    pDst    =   (TTo*)(pStart);
                            std::fill(pDst, pDst + width, tSrc);
                            pStart  +=  nPitch;
                        }
                    }
                }
            }
            return  true;
        }
        /// <summary>
        /// TPBuffer 一般采用了tiled ，而不是按行线性存储的
        /// 函数的作用实现转换过程
        /// </summary>
        /// <typeparam name="TFrom"></typeparam>
        /// <typeparam name="TTo"></typeparam>
        /// <param name="pbo"></param>
        /// <param name="plane"></param>
        /// <returns></returns>
        template<class TFrom,class TTo>
        inline  bool    convert(const TPBuffer<TFrom>& pbo,ImgSource& plane)
        {
            const auto&   tiles   =   pbo.tiles();
            for (auto& var : tiles)
            {   
                const   RectU16     rt  =   var.rect();
                const   uint16_t    w   =   rt.width();
                const   uint16_t    h   =   rt.height();
                for (uint16_t y = 0; y < h; ++y)
                {
                    auto    src =   var.rowAt(y);
                    auto    dst =   plane.dataOffset<TTo>(rt.top(),rt.left());
                    if constexpr (std::is_same_v<TFrom, TTo>)
                    {
                        std::memcpy(dst,src,sizeof(TTo) * w);
                    }
                    else
                    {
                        /// 避免频繁切换内存地址,引起cache失效
                        TTo tmp[_TileW];
                        for (uint16_t x = 0; x < w; ++x)
                        {
                            tmp[x]  =   castTo<TFrom,TTo>(src[x]);
                        }
                        std::memcpy(dst,tmp,sizeof(TTo) * w);
                    }   
                }
            }
            return  true;
        }
    protected:
        static  bool    allocMemory(FEContext& ctx,FEImage::CreateInfo& info);
        /// <summary>
        /// 完整图像占用空间的大小(单位字节)
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        static  size_t  calcImageSize(const FEImage::CreateInfo& info)
        {
            uint32  levels  =   (std::max<uint32>)(1,info._levels);
            uint32  layers  =   (std::max<uint32>)(1,info._layers);
            return  layers * bytesOfLayer(info._extent,levels,info._align,info._format);
        }
        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// </summary>
        /// <param name="w">宽度</param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="fmt">像素格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32  pitch(uint32 w,uint16_t align,FEFormat fmt)
        {
            return  FEFormatHelper::pitch(w,align,fmt);
        }
        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// </summary>
        /// <param name="w"></param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="pixelByte">每个像素的字节数</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32  pitch(uint32 w,uint16_t align,uint16_t pixelByte)
        {
            return  FEFormatHelper::pitch(w,align,pixelByte);
        }
        /// <summary>
        /// 根据图像的大小,mipmap,对齐方式,数据格式,计算需要的空间字节数
        /// </summary>
        /// <param name="extent">长宽高</param>
        /// <param name="levels">mipmap</param>
        /// <param name="align">对齐1,2,4,8,16,32,64...</param>
        /// <param name="fmt">格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32  bytesOfLayer(const uint3& extent,uint32 levels,uint32 align,FEFormat fmt)
        {
            return  FEFormatHelper::bytesOfLayer(extent,levels,align,fmt);
        }
    protected:
        FEImage::CreateInfo _cInfo;
    };
    using   IMCreateInfo        =   FEImage::CreateInfo;
    using   ImagePtr            =   SharedPtr<FEImage>;
}
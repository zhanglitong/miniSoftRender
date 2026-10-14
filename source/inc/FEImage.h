#pragma     once
#include    <string>
#include    <cassert>
#include    "FEObject.h"
#include    "FEMath.hpp"
#include    "FEColor.hpp"
#include    "FEFlags.hpp"
#include    "FEBuffer.hpp"
#include    "FEFormatHelper.hpp"
#include    "graphic/FEPBuffer.h"
#include    "FEImageSource.hpp"

#include    "FEFormat.hpp"
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
        bool        create(const FEImage::CreateInfo& info);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="layer">0-layers</param>
        /// <param name="level">0-mips</param>
        /// <returns>像素数据的内存位置,参数越界时返回nullptr</returns>
        inline  uint8*      pixel(uint32 layer,uint32 level = 0)
        {
            const bool  bValid  =   layer < _cInfo._buffers.size()
                                &&  level < _cInfo._levels
                                &&  _cInfo._buffers[layer] != nullptr;
            assert(bValid);
            if (!bValid)
                return  nullptr;
            auto    pixel   =   _cInfo._buffers[layer]->cInfo().dataPtr();
            size_t  nByte   =   offsetOfLevel(level);
            return  pixel + nByte;
        }
        inline  const uint8* pixel(uint32 layer,uint32 level = 0) const
        {
            const bool  bValid  =   layer < _cInfo._buffers.size()
                                &&  level < _cInfo._levels
                                &&  _cInfo._buffers[layer] != nullptr;
            assert(bValid);
            if (!bValid)
                return  nullptr;
            auto    pixel   =   _cInfo._buffers[layer]->cInfo().dataPtr();
            size_t  nByte   =   offsetOfLevel(level);
            return  pixel + nByte;
        }
        /// <summary>
        /// 获取图片的一个最小图像单元
        /// </summary>
        /// <param name="level">0-mips</param>
        /// <param name="layer">0-layers</param>
        /// <returns></returns>
        ImgSource       plane(uint32 level,uint32 layer = 0) const
        {
            FEImageSource   data;
            auto            pData   =   pixel(layer,level);
            if (pData == nullptr)
                return  data;
            const auto      w       =   (std::max<uint32>)(_cInfo._extent.x >> level,1);
            const auto      h       =   (std::max<uint32>)(_cInfo._extent.y >> level,1);
            
            data._buffer    =   (uint8*)pData;
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
            for (uint32 i = 0; i < level; i++)
                offset  +=  byteOfLevel(i);
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
            case FMT_R8_UNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM             >>(color);
            case FMT_R8_SNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM             >>(color);
            case FMT_R8_USCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED           >>(color);
            case FMT_R8_SSCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED           >>(color);
            case FMT_R8_UINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT              >>(color);
            case FMT_R8_SINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT              >>(color);
            case FMT_R4G4_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM           >>(color);

            case FMT_R8G8_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >>(color);
            case FMT_R8G8_SNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >>(color);
            case FMT_R8G8_USCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >>(color);
            case FMT_R8G8_SSCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >>(color);
            case FMT_R8G8_UINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >>(color);
            case FMT_R8G8_SINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >>(color);

            case FMT_R4G4B4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM       >>(color);
            case FMT_B4G4R4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM       >>(color);
            case FMT_R5G6B5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM         >>(color);
            case FMT_B5G6R5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM         >>(color);
            case FMT_R5G5B5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM       >>(color);
            case FMT_B5G5R5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM       >>(color);
            case FMT_A1R5G5B5_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM       >>(color);

            case FMT_R8G8B8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM          >>(color);
            case FMT_R8G8B8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM          >>(color);
            case FMT_R8G8B8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED        >>(color);
            case FMT_R8G8B8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED        >>(color);
            case FMT_R8G8B8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT           >>(color);
            case FMT_R8G8B8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT           >>(color);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM        >>(color);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM        >>(color);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED      >>(color);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED      >>(color);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT         >>(color);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT         >>(color);

            case FMT_A2B10G10R10_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM    >>(color);
            case FMT_A2B10G10R10_SNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM    >>(color);
            case FMT_A2B10G10R10_USCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED  >>(color);
            case FMT_A2B10G10R10_SSCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED  >>(color);
            case FMT_A2B10G10R10_UINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT     >>(color);
            case FMT_A2B10G10R10_SINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT     >>(color);

            case FMT_R16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM            >>(color);
            case FMT_R16_SNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM            >>(color);
            case FMT_R16_USCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED          >>(color);
            case FMT_R16_SSCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED          >>(color);
            case FMT_R16_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT             >>(color);
            case FMT_R16_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT             >>(color);

            case FMT_R16G16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM          >>(color);
            case FMT_R16G16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM          >>(color);
            case FMT_R16G16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED        >>(color);
            case FMT_R16G16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED        >>(color);
            case FMT_R16G16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT           >>(color);
            case FMT_R16G16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT           >>(color);

            case FMT_R16G16B16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM       >>(color);
            case FMT_R16G16B16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM       >>(color);
            case FMT_R16G16B16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED     >>(color);
            case FMT_R16G16B16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED     >>(color);
            case FMT_R16G16B16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT        >>(color);
            case FMT_R16G16B16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT        >>(color);

            case FMT_R16G16B16A16_UNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM    >>(color);
            case FMT_R16G16B16A16_SNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM    >>(color);
            case FMT_R16G16B16A16_USCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED  >>(color);
            case FMT_R16G16B16A16_SSCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED  >>(color);
            case FMT_R16G16B16A16_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT     >>(color);
            case FMT_R16G16B16A16_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT     >>(color);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT             >>(color);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT             >>(color);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >>(color);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >>(color);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >>(color);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >>(color);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >>(color);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >>(color);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >>(color);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >>(color);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >>(color);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >>(color);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >>(color);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >>(color);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >>(color);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >>(color);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM            >>(color);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM            >>(color);
            }
            return  false;
        }
        template<class TFrom>
        inline  bool    fill(const TPBuffer<TFrom>& pBuffer)
        {
            /// 安全校验:pBuffer尺寸不能超过图像尺寸
            const auto  clip    =   pBuffer.clip();
            const bool  bCheck  =   clip.width() <= _cInfo._extent.x && clip.height() <= _cInfo._extent.y;
            assert(bCheck);
            if (!bCheck)
                return  false;
            /// pBuffer只有全分辨率数据,填充每个layer的level 0
            for (uint32 l = 0; l < _cInfo._layers; ++l)
            {
                auto    dst =   this->plane(0,l);
                if (dst._buffer == nullptr || !fillPlane(pBuffer,dst))
                    return  false;
            }
            return  true;
        }
    protected:
        /// <summary>
        /// 用pBuffer填充单个plane的数据(仅level 0)
        /// </summary>
        /// <param name="pBuffer"></param>
        /// <param name="plane"></param>
        /// <returns></returns>
        template<class TFrom>
        inline  bool    fillPlane(const TPBuffer<TFrom>& pBuffer,ImgSource& plane)
        {
            switch(_cInfo._format)
            {
            case FMT_R8_UNORM            :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM             >>(pBuffer,plane);
            case FMT_R8_SNORM            :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM             >>(pBuffer,plane);
            case FMT_R8_USCALED          :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED           >>(pBuffer,plane);
            case FMT_R8_SSCALED          :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED           >>(pBuffer,plane);
            case FMT_R8_UINT             :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT              >>(pBuffer,plane);
            case FMT_R8_SINT             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT              >>(pBuffer,plane);
            case FMT_R4G4_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM           >>(pBuffer,plane);

            case FMT_R8G8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >>(pBuffer,plane);
            case FMT_R8G8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >>(pBuffer,plane);
            case FMT_R8G8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >>(pBuffer,plane);
            case FMT_R8G8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >>(pBuffer,plane);
            case FMT_R8G8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >>(pBuffer,plane);
            case FMT_R8G8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >>(pBuffer,plane);

            case FMT_R4G4B4A4_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM       >>(pBuffer,plane);
            case FMT_B4G4R4A4_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM       >>(pBuffer,plane);
            case FMT_R5G6B5_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM         >>(pBuffer,plane);
            case FMT_B5G6R5_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM         >>(pBuffer,plane);
            case FMT_R5G5B5A1_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM       >>(pBuffer,plane);
            case FMT_B5G5R5A1_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM       >>(pBuffer,plane);
            case FMT_A1R5G5B5_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM       >>(pBuffer,plane);

            case FMT_R8G8B8_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM           >>(pBuffer,plane);
            case FMT_R8G8B8_SNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM           >>(pBuffer,plane);
            case FMT_R8G8B8_USCALED        :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED         >>(pBuffer,plane);
            case FMT_R8G8B8_SSCALED        :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED         >>(pBuffer,plane);
            case FMT_R8G8B8_UINT           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT            >>(pBuffer,plane);
            case FMT_R8G8B8_SINT           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT            >>(pBuffer,plane);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM          >>(pBuffer,plane);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM          >>(pBuffer,plane);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED        >>(pBuffer,plane);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED        >>(pBuffer,plane);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT           >>(pBuffer,plane);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT           >>(pBuffer,plane);

            case FMT_A2B10G10R10_UNORM   :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM    >>(pBuffer,plane);
            case FMT_A2B10G10R10_SNORM   :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM    >>(pBuffer,plane);
            case FMT_A2B10G10R10_USCALED :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED  >>(pBuffer,plane);
            case FMT_A2B10G10R10_SSCALED :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED  >>(pBuffer,plane);
            case FMT_A2B10G10R10_UINT    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT     >>(pBuffer,plane);
            case FMT_A2B10G10R10_SINT    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT     >>(pBuffer,plane);

            case FMT_R16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM            >>(pBuffer,plane);
            case FMT_R16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM            >>(pBuffer,plane);
            case FMT_R16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED          >>(pBuffer,plane);
            case FMT_R16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED          >>(pBuffer,plane);
            case FMT_R16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT             >>(pBuffer,plane);
            case FMT_R16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT             >>(pBuffer,plane);

            case FMT_R16G16_UNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM           >>(pBuffer,plane);
            case FMT_R16G16_SNORM          :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM           >>(pBuffer,plane);
            case FMT_R16G16_USCALED        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED         >>(pBuffer,plane);
            case FMT_R16G16_SSCALED        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED         >>(pBuffer,plane);
            case FMT_R16G16_UINT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT            >>(pBuffer,plane);
            case FMT_R16G16_SINT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT            >>(pBuffer,plane);

            case FMT_R16G16B16_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM          >>(pBuffer,plane);
            case FMT_R16G16B16_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM          >>(pBuffer,plane);
            case FMT_R16G16B16_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED        >>(pBuffer,plane);
            case FMT_R16G16B16_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED        >>(pBuffer,plane);
            case FMT_R16G16B16_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT           >>(pBuffer,plane);
            case FMT_R16G16B16_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT           >>(pBuffer,plane);

            case FMT_R16G16B16A16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM         >>(pBuffer,plane);
            case FMT_R16G16B16A16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM         >>(pBuffer,plane);
            case FMT_R16G16B16A16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED       >>(pBuffer,plane);
            case FMT_R16G16B16A16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED       >>(pBuffer,plane);
            case FMT_R16G16B16A16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT          >>(pBuffer,plane);
            case FMT_R16G16B16A16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT          >>(pBuffer,plane);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT             >>(pBuffer,plane);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT             >>(pBuffer,plane);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >>(pBuffer,plane);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >>(pBuffer,plane);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >>(pBuffer,plane);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >>(pBuffer,plane);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >>(pBuffer,plane);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >>(pBuffer,plane);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >>(pBuffer,plane);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >>(pBuffer,plane);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >>(pBuffer,plane);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >>(pBuffer,plane);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >>(pBuffer,plane);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >>(pBuffer,plane);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >>(pBuffer,plane);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >>(pBuffer,plane);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM             >>(pBuffer,plane);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM             >>(pBuffer,plane);
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
        /// Tag: TypeOfFormat<FMT>标签,携带宿存储类型type与宿格式value(决定宿量化语义)
        /// </summary>
        /// <typeparam name="TFrom"></typeparam>
        /// <typeparam name="Tag"></typeparam>
        /// <param name="data"></param>
        /// <param name="src"></param>
        /// <returns></returns>
        template<class TFrom,class Tag>
        inline  bool    convert(const TFrom& src)
        {
            using   TTo                 =   typename Tag::type;
            constexpr FEFormat  toFMT   =   Tag::value;
            /// 不支持的转换对直接返回false,避免实例化产生编译错误
            if constexpr (!IsTConvertible<ExtractColorValueType<TFrom>,TTo,toFMT,ColorFormatOf<TFrom>::value>::value)
            {
                return  false;
            }
            else
            {
            size_t      nSize       =   0;
            uint32      mips        =   (std::max<uint32>)(1,_cInfo._levels);
            uint32      layer       =   (std::max<uint32>)(1,_cInfo._layers);
            TTo         tSrc        =   castTo<TFrom,TTo,toFMT>(src);
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
        }
        /// <summary>
        /// TPBuffer 一般采用了tiled ，而不是按行线性存储的
        /// 函数的作用实现转换过程
        /// Tag: TypeOfFormat<FMT>标签,携带宿存储类型type与宿格式value(决定宿量化语义)
        /// </summary>
        /// <typeparam name="TFrom"></typeparam>
        /// <typeparam name="Tag"></typeparam>
        /// <param name="pbo"></param>
        /// <param name="plane"></param>
        /// <returns></returns>
        template<class TFrom,class Tag>
        inline  bool    convert(const TPBuffer<TFrom>& pbo,ImgSource& plane)
        {
            using   TTo                 =   typename Tag::type;
            constexpr FEFormat  toFMT   =   Tag::value;
            /// 不支持的转换对直接返回false,避免实例化产生编译错误
            if constexpr (!IsTConvertible<ExtractColorValueType<TFrom>,TTo,toFMT,ColorFormatOf<TFrom>::value>::value)
            {
                return  false;
            }
            else
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
                    /// 目标行必须随y递增(原实现漏加y,每个瓦片都重复写回首行)
                    auto    dst =   plane.dataOffset<TTo>(rt.top() + y,rt.left());
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
                            tmp[x]  =   castTo<TFrom,TTo,toFMT>(src[x]);
                        }
                        std::memcpy(dst,tmp,sizeof(TTo) * w);
                    }
                }
            }
            return  true;
            }
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
            return  size_t(layers) * bytesOfLayer(info._extent,levels,info._align,info._format);
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

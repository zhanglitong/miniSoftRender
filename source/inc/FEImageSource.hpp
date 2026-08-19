#pragma     once
#include    <string>
#include    "../inc/FEObject.h"
#include    "../inc/FEMath.hpp"
#include    "../inc/FEColor.hpp"
#include    "../inc/FEFlags.hpp"
#include    "../inc/FEFormatHelper.hpp"

namespace   FE
{
    /// <summary>
    /// 图像的最基本单元: [x*y*z*fmt]
    /// 数组[layers] * ( [x*y*z*fmt] + [x>>1*y>>1*z>>1*fmt] + ...]
    /// </summary>
    struct  FEImageSource
    {
        uint8_t*    _buffer =   nullptr;
        uint32_t    _width  =   0;
        uint32_t    _height =   0;
        FEFormat    _format =   FMT_NONE;
        /// <summary>
        /// 单位字节
        /// </summary>
        uint16_t    _align  =   4;
        /// <summary>
        /// 缓存，可以通过 _align + _format 得出
        /// </summary>
        uint32_t    _pitch  =   0;
        /// <summary>
        /// 如果 _align / format 发生变化，需要更新一下
        /// </summary>
        inline  void    update()
        {
            _pitch  =   FEFormatHelper::pitch(_width,_align,_format);
        }
        /// <summary>
        /// 格式
        /// </summary>
        /// <returns></returns>
        inline  auto    format() const
        {
            return  _format;
        }
        /// <summary>
        /// 宽度
        /// </summary>
        /// <returns></returns>
        inline  auto    width()  const
        {
            return  _width;
        }
        /// <summary>
        /// 高度
        /// </summary>
        /// <returns></returns>
        inline  auto    height() const
        {
            return  _height;
        }
        /// <summary>
        /// 一行数据大小
        /// </summary>
        /// <returns></returns>
        inline  auto    pitch()  const
        {
            return  _pitch;
        }
        /// <summary>
        /// 原始数据地址
        /// </summary>
        /// <returns></returns>
        inline  auto    data() 
        {
            return  _buffer;
        }
        const   auto    data() const
        {
            return  _buffer;
        }
        template<class TType>
        inline  TType*  dataAs() 
        {
            return  (TType*)_buffer;
        }
        /// <summary>
        /// 得到当前数据的首地址
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        template<class TType>
        const   TType*  dataAs()  const
        {
            return  (const TType*)_buffer;
        }
        /// <summary>
        /// 获取指定行数据首地址
        /// </summary>
        /// <param name="row"></param>
        /// <returns></returns>
        template<class TType = uint8>
        inline  TType*  dataOffset(uint32_t row,uint32_t col) 
        {
            return  (TType*)(_buffer + row * pitch() + col * FEFormatHelper::sizeOf(_format));
        }
        template<class TType = uint8>
        const   TType*  dataOffset(uint32_t row,uint32_t col) const
        {
            return  (TType*)(_buffer + row * pitch() + col *  FEFormatHelper::sizeOf(_format));
        }
        /// <summary>
        /// 得到一行数据的首地址
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="row"></param>
        /// <returns></returns>
        template<class TType  = uint8>
        inline  TType*  rowAt(uint32_t row) 
        {
            auto    pData   =   _buffer + pitch() * row;
            return  (TType*)pData;
        }
        /// <summary>
        /// 得到一行数据的首地址
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        template<class TType  = uint8>
        const   TType*  rowAt(uint32_t row)  const
        {
            const auto  pData   =   _buffer + pitch() * row;
            return  (const TType*)pData;
        }
        /// <summary>
        /// 设置一个像素
        /// </summary>
        /// <typeparam name="TType"></typeparam>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="data"></param>
        /// <returns></returns>
        template<class TType>
        inline  auto&   setPixel(uint32_t x,uint32_t y,const TType& data)
        {
            TType*  pRow    =   rowAt<TType>(y);
            TType*  pixel   =   pRow + x;
                    *pixel  =   data;
            return  *this;
        }
        /// <summary>
        /// 对指定的区域填充固定值
        /// </summary>
        /// <typeparam name="TFrom">颜色对象类型</typeparam>
        /// <param name="rect">区域范围</param>
        /// <param name="color">颜色值</param>
        /// <returns></returns>
        template<class TFrom>
        inline  auto&   fillRect(const RectU32& rect,const TFrom& color)
        {
            switch(_format)
            {
            case FMT_R8_UNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM             >::type>(rect,color);
            case FMT_R8_SNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM             >::type>(rect,color);
            case FMT_R8_USCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED           >::type>(rect,color);
            case FMT_R8_SSCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED           >::type>(rect,color);
            case FMT_R8_UINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT              >::type>(rect,color);
            case FMT_R8_SINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT              >::type>(rect,color);
            case FMT_R4G4_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM           >::type>(rect,color);

            case FMT_R8G8_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >::type>(rect,color);
            case FMT_R8G8_SNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >::type>(rect,color);
            case FMT_R8G8_USCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >::type>(rect,color);
            case FMT_R8G8_SSCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >::type>(rect,color);
            case FMT_R8G8_UINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >::type>(rect,color);
            case FMT_R8G8_SINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >::type>(rect,color);

            case FMT_R4G4B4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM       >::type>(rect,color);
            case FMT_B4G4R4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM       >::type>(rect,color);
            case FMT_R5G6B5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM         >::type>(rect,color);
            case FMT_B5G6R5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM         >::type>(rect,color);
            case FMT_R5G5B5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM       >::type>(rect,color);
            case FMT_B5G5R5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM       >::type>(rect,color);
            case FMT_A1R5G5B5_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM       >::type>(rect,color);

            case FMT_R8G8B8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM          >::type>(rect,color);
            case FMT_R8G8B8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM          >::type>(rect,color);
            case FMT_R8G8B8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED        >::type>(rect,color);
            case FMT_R8G8B8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED        >::type>(rect,color);
            case FMT_R8G8B8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT           >::type>(rect,color);
            case FMT_R8G8B8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT           >::type>(rect,color);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM        >::type>(rect,color);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM        >::type>(rect,color);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED      >::type>(rect,color);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED      >::type>(rect,color);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT         >::type>(rect,color);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT         >::type>(rect,color);

            case FMT_A2B10G10R10_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM    >::type>(rect,color);
            case FMT_A2B10G10R10_SNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM    >::type>(rect,color);
            case FMT_A2B10G10R10_USCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED  >::type>(rect,color);
            case FMT_A2B10G10R10_SSCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED  >::type>(rect,color);
            case FMT_A2B10G10R10_UINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT     >::type>(rect,color);
            case FMT_A2B10G10R10_SINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT     >::type>(rect,color);

            case FMT_R16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM            >::type>(rect,color);
            case FMT_R16_SNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM            >::type>(rect,color);
            case FMT_R16_USCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED          >::type>(rect,color);
            case FMT_R16_SSCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED          >::type>(rect,color);
            case FMT_R16_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT             >::type>(rect,color);
            case FMT_R16_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT             >::type>(rect,color);

            case FMT_R16G16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM          >::type>(rect,color);
            case FMT_R16G16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM          >::type>(rect,color);
            case FMT_R16G16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED        >::type>(rect,color);
            case FMT_R16G16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED        >::type>(rect,color);
            case FMT_R16G16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT           >::type>(rect,color);
            case FMT_R16G16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT           >::type>(rect,color);

            case FMT_R16G16B16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM       >::type>(rect,color);
            case FMT_R16G16B16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM       >::type>(rect,color);
            case FMT_R16G16B16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED     >::type>(rect,color);
            case FMT_R16G16B16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED     >::type>(rect,color);
            case FMT_R16G16B16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT        >::type>(rect,color);
            case FMT_R16G16B16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT        >::type>(rect,color);

            case FMT_R16G16B16A16_UNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM    >::type>(rect,color);
            case FMT_R16G16B16A16_SNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM    >::type>(rect,color);
            case FMT_R16G16B16A16_USCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED  >::type>(rect,color);
            case FMT_R16G16B16A16_SSCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED  >::type>(rect,color);
            case FMT_R16G16B16A16_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT     >::type>(rect,color);
            case FMT_R16G16B16A16_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT     >::type>(rect,color);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT              >::type>(rect,color);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT              >::type>(rect,color);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >::type>(rect,color);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >::type>(rect,color);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >::type>(rect,color);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >::type>(rect,color);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >::type>(rect,color);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >::type>(rect,color);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >::type>(rect,color);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >::type>(rect,color);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >::type>(rect,color);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >::type>(rect,color);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >::type>(rect,color);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >::type>(rect,color);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >::type>(rect,color);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >::type>(rect,color);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM             >::type>(rect,color);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM             >::type>(rect,color);
            }
            return  *this;
        }
        /// <summary>
        /// 对指定的区域填充值
        /// </summary>
        /// <typeparam name="TFrom">颜色对象类型</typeparam>
        /// <param name="rect">区域范围</param>
        /// <param name="color">颜色数组(必须连续)</param>
        /// <returns></returns>
        template<class TFrom>
        inline  auto&   fillRect(const RectU32& rect,const TFrom* color)
        {
            switch(_format)
            {
            case FMT_R8_UNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM              >::type>(rect,color);
            case FMT_R8_SNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM              >::type>(rect,color);
            case FMT_R8_USCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED            >::type>(rect,color);
            case FMT_R8_SSCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED            >::type>(rect,color);
            case FMT_R8_UINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT               >::type>(rect,color);
            case FMT_R8_SINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT               >::type>(rect,color);
            case FMT_R4G4_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM            >::type>(rect,color);

            case FMT_R8G8_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >::type>(rect,color);
            case FMT_R8G8_SNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >::type>(rect,color);
            case FMT_R8G8_USCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >::type>(rect,color);
            case FMT_R8G8_SSCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >::type>(rect,color);
            case FMT_R8G8_UINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >::type>(rect,color);
            case FMT_R8G8_SINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >::type>(rect,color);

            case FMT_R4G4B4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM        >::type>(rect,color);
            case FMT_B4G4R4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM        >::type>(rect,color);
            case FMT_R5G6B5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM          >::type>(rect,color);
            case FMT_B5G6R5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM          >::type>(rect,color);
            case FMT_R5G5B5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM        >::type>(rect,color);
            case FMT_B5G5R5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM        >::type>(rect,color);
            case FMT_A1R5G5B5_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM        >::type>(rect,color);

            case FMT_R8G8B8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM          >::type>(rect,color);
            case FMT_R8G8B8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM          >::type>(rect,color);
            case FMT_R8G8B8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED        >::type>(rect,color);
            case FMT_R8G8B8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED        >::type>(rect,color);
            case FMT_R8G8B8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT           >::type>(rect,color);
            case FMT_R8G8B8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT           >::type>(rect,color);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM        >::type>(rect,color);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM        >::type>(rect,color);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED      >::type>(rect,color);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED      >::type>(rect,color);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT         >::type>(rect,color);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT         >::type>(rect,color);

            case FMT_A2B10G10R10_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM     >::type>(rect,color);
            case FMT_A2B10G10R10_SNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM     >::type>(rect,color);
            case FMT_A2B10G10R10_USCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED   >::type>(rect,color);
            case FMT_A2B10G10R10_SSCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED   >::type>(rect,color);
            case FMT_A2B10G10R10_UINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT      >::type>(rect,color);
            case FMT_A2B10G10R10_SINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT      >::type>(rect,color);

            case FMT_R16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM             >::type>(rect,color);
            case FMT_R16_SNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM             >::type>(rect,color);
            case FMT_R16_USCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED           >::type>(rect,color);
            case FMT_R16_SSCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED           >::type>(rect,color);
            case FMT_R16_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT              >::type>(rect,color);
            case FMT_R16_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT              >::type>(rect,color);

            case FMT_R16G16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM          >::type>(rect,color);
            case FMT_R16G16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM          >::type>(rect,color);
            case FMT_R16G16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED        >::type>(rect,color);
            case FMT_R16G16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED        >::type>(rect,color);
            case FMT_R16G16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT           >::type>(rect,color);
            case FMT_R16G16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT           >::type>(rect,color);

            case FMT_R16G16B16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM       >::type>(rect,color);
            case FMT_R16G16B16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM       >::type>(rect,color);
            case FMT_R16G16B16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED     >::type>(rect,color);
            case FMT_R16G16B16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED     >::type>(rect,color);
            case FMT_R16G16B16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT        >::type>(rect,color);
            case FMT_R16G16B16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT        >::type>(rect,color);

            case FMT_R16G16B16A16_UNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM    >::type>(rect,color);
            case FMT_R16G16B16A16_SNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM    >::type>(rect,color);
            case FMT_R16G16B16A16_USCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED  >::type>(rect,color);
            case FMT_R16G16B16A16_SSCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED  >::type>(rect,color);
            case FMT_R16G16B16A16_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT     >::type>(rect,color);
            case FMT_R16G16B16A16_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT     >::type>(rect,color);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT              >::type>(rect,color);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT              >::type>(rect,color);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >::type>(rect,color);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >::type>(rect,color);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >::type>(rect,color);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >::type>(rect,color);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >::type>(rect,color);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >::type>(rect,color);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >::type>(rect,color);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >::type>(rect,color);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >::type>(rect,color);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >::type>(rect,color);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >::type>(rect,color);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >::type>(rect,color);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >::type>(rect,color);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >::type>(rect,color);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM             >::type>(rect,color);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM             >::type>(rect,color);
            }
            return  *this;
        }
        /// <summary>
        /// 对指定的区域填充值
        /// rtSrc.width()  == rtDst.width()
        /// rtSrc.height() == rtDst.height()
        /// 
        /// </summary>
        /// <typeparam name="TFrom">数据类型</typeparam>
        /// <param name="rtDst">目标矩形区域，数据覆盖的区域</param>
        /// <param name="src">数据源</param>
        /// <param name="rtSrc">数据源区域,主要top,left有效,right,bottom 暂时无效,</param>
        /// <returns></returns>
        template<class TFrom>
        inline  auto&   fillRect(const RectU32& rtDst,const FEImageSource& src,const RectU32& rtSrc)
        {
            switch(_format)
            {
            case FMT_R8_UNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_UNORM              >::type>(rtDst,src,rtSrc);
            case FMT_R8_SNORM               :  return  convert<TFrom,TypeOfFormat<FMT_R8_SNORM              >::type>(rtDst,src,rtSrc);
            case FMT_R8_USCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_USCALED            >::type>(rtDst,src,rtSrc);
            case FMT_R8_SSCALED             :  return  convert<TFrom,TypeOfFormat<FMT_R8_SSCALED            >::type>(rtDst,src,rtSrc);
            case FMT_R8_UINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_UINT               >::type>(rtDst,src,rtSrc);
            case FMT_R8_SINT                :  return  convert<TFrom,TypeOfFormat<FMT_R8_SINT               >::type>(rtDst,src,rtSrc);
            case FMT_R4G4_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R4G4_UNORM            >::type>(rtDst,src,rtSrc);

            case FMT_R8G8_UNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UNORM            >::type>(rtDst,src,rtSrc);
            case FMT_R8G8_SNORM             :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SNORM            >::type>(rtDst,src,rtSrc);
            case FMT_R8G8_USCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_USCALED          >::type>(rtDst,src,rtSrc);
            case FMT_R8G8_SSCALED           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SSCALED          >::type>(rtDst,src,rtSrc);
            case FMT_R8G8_UINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_UINT             >::type>(rtDst,src,rtSrc);
            case FMT_R8G8_SINT              :  return  convert<TFrom,TypeOfFormat<FMT_R8G8_SINT             >::type>(rtDst,src,rtSrc);

            case FMT_R4G4B4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R4G4B4A4_UNORM        >::type>(rtDst,src,rtSrc);
            case FMT_B4G4R4A4_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B4G4R4A4_UNORM        >::type>(rtDst,src,rtSrc);
            case FMT_R5G6B5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R5G6B5_UNORM          >::type>(rtDst,src,rtSrc);
            case FMT_B5G6R5_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_B5G6R5_UNORM          >::type>(rtDst,src,rtSrc);
            case FMT_R5G5B5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R5G5B5A1_UNORM        >::type>(rtDst,src,rtSrc);
            case FMT_B5G5R5A1_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_B5G5R5A1_UNORM        >::type>(rtDst,src,rtSrc);
            case FMT_A1R5G5B5_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_A1R5G5B5_UNORM        >::type>(rtDst,src,rtSrc);

            case FMT_R8G8B8_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UNORM          >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SNORM          >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_USCALED        >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SSCALED        >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_UINT           >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8_SINT           >::type>(rtDst,src,rtSrc);

            case FMT_R8G8B8A8_UNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UNORM        >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8A8_SNORM         :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SNORM        >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8A8_USCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_USCALED      >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8A8_SSCALED       :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SSCALED      >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8A8_UINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_UINT         >::type>(rtDst,src,rtSrc);
            case FMT_R8G8B8A8_SINT          :  return  convert<TFrom,TypeOfFormat<FMT_R8G8B8A8_SINT         >::type>(rtDst,src,rtSrc);

            case FMT_A2B10G10R10_UNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UNORM     >::type>(rtDst,src,rtSrc);
            case FMT_A2B10G10R10_SNORM      :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SNORM     >::type>(rtDst,src,rtSrc);
            case FMT_A2B10G10R10_USCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_USCALED   >::type>(rtDst,src,rtSrc);
            case FMT_A2B10G10R10_SSCALED    :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SSCALED   >::type>(rtDst,src,rtSrc);
            case FMT_A2B10G10R10_UINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_UINT      >::type>(rtDst,src,rtSrc);
            case FMT_A2B10G10R10_SINT       :  return  convert<TFrom,TypeOfFormat<FMT_A2B10G10R10_SINT      >::type>(rtDst,src,rtSrc);

            case FMT_R16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_UNORM             >::type>(rtDst,src,rtSrc);
            case FMT_R16_SNORM              :  return  convert<TFrom,TypeOfFormat<FMT_R16_SNORM             >::type>(rtDst,src,rtSrc);
            case FMT_R16_USCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_USCALED           >::type>(rtDst,src,rtSrc);
            case FMT_R16_SSCALED            :  return  convert<TFrom,TypeOfFormat<FMT_R16_SSCALED           >::type>(rtDst,src,rtSrc);
            case FMT_R16_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_UINT              >::type>(rtDst,src,rtSrc);
            case FMT_R16_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R16_SINT              >::type>(rtDst,src,rtSrc);

            case FMT_R16G16_UNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UNORM          >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_SNORM           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SNORM          >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_USCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_USCALED        >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_SSCALED         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SSCALED        >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_UINT           >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_SINT           >::type>(rtDst,src,rtSrc);

            case FMT_R16G16B16_UNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UNORM       >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_SNORM        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SNORM       >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_USCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_USCALED     >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_SSCALED      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SSCALED     >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_UINT        >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_SINT        >::type>(rtDst,src,rtSrc);

            case FMT_R16G16B16A16_UNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UNORM    >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_SNORM     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SNORM    >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_USCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_USCALED  >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_SSCALED   :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SSCALED  >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_UINT     >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_SINT     >::type>(rtDst,src,rtSrc);

            case FMT_R32_UINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_UINT              >::type>(rtDst,src,rtSrc);
            case FMT_R32_SINT               :  return  convert<TFrom,TypeOfFormat<FMT_R32_SINT              >::type>(rtDst,src,rtSrc);

            case FMT_R32G32_UINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_UINT           >::type>(rtDst,src,rtSrc);
            case FMT_R32G32_SINT            :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_SINT           >::type>(rtDst,src,rtSrc);

            case FMT_R32G32B32_UINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_UINT        >::type>(rtDst,src,rtSrc);
            case FMT_R32G32B32_SINT         :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_SINT        >::type>(rtDst,src,rtSrc);

            case FMT_R32G32B32A32_UINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_UINT     >::type>(rtDst,src,rtSrc);
            case FMT_R32G32B32A32_SINT      :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_SINT     >::type>(rtDst,src,rtSrc);

            case FMT_R16_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R16_FLOAT             >::type>(rtDst,src,rtSrc);
            case FMT_R16G16_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R16G16_FLOAT          >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16_FLOAT       >::type>(rtDst,src,rtSrc);
            case FMT_R16G16B16A16_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R16G16B16A16_FLOAT    >::type>(rtDst,src,rtSrc);

            case FMT_R32_FLOAT              :  return  convert<TFrom,TypeOfFormat<FMT_R32_FLOAT             >::type>(rtDst,src,rtSrc);
            case FMT_R32G32_FLOAT           :  return  convert<TFrom,TypeOfFormat<FMT_R32G32_FLOAT          >::type>(rtDst,src,rtSrc);
            case FMT_R32G32B32_FLOAT        :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32_FLOAT       >::type>(rtDst,src,rtSrc);
            case FMT_R32G32B32A32_FLOAT     :  return  convert<TFrom,TypeOfFormat<FMT_R32G32B32A32_FLOAT    >::type>(rtDst,src,rtSrc);

            case FMT_D16_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D16_UNORM             >::type>(rtDst,src,rtSrc);
            case FMT_D32_UNORM              :  return  convert<TFrom,TypeOfFormat<FMT_D32_UNORM             >::type>(rtDst,src,rtSrc);
            }
            return  *this;
        }
        template<class TFrom>
        inline  auto&   fillLine(uint32 x,uint32 y,uint32 width,const TFrom& color)
        {
            const   RectU32 rect(x,y,x + width,y + 1);
            return  fillRect<TFrom>(rect,color);
        }
        /// <summary>
        /// 一行数据的真实数据大小，不包含对齐填补部分
        /// 与pitch区别 bytesOfRow() <= pitch
        /// pitch有对齐信息
        /// </summary>
        /// <returns></returns>
        inline  uint64  bytesOfRow() const
        {
            return  FEFormatHelper::sizeOf(_format) * _width;
        }
        /// <summary>
        /// 数据大小,单位字节
        /// </summary>
        /// <returns></returns>
        inline  uint64  size() const
        {
            return  pitch() * _height;
        }
        /// <summary>
        /// 水平反转图像
        /// </summary>
        inline  void    flipY()
        {
            uint32  halfH       =   _height >> 1;
            auto    pData       =   _buffer;
            uint8   temp[1<<12] =   {};  

            uint32  nBlock      =   _pitch >> 12;
            uint32  nLast       =   _pitch - nBlock * sizeof(temp);
            for (uint32 y = 0; y < halfH; ++y) 
            {
                auto    pTop    =   (uint8_t*)(pData + y * _pitch);
                auto    pBottom =   (uint8_t*)(pData + (_height - 1 - y) * _pitch);
                for (uint32 i = 0; i < nBlock; i++)
                {
                    std::memcpy(temp,    pTop,       sizeof(temp));
                    std::memcpy(pTop,    pBottom,    sizeof(temp));
                    std::memcpy(pBottom, temp,       sizeof(temp));
                    pTop        +=  sizeof(temp);
                    pBottom     +=  sizeof(temp);
                }
                if (nLast)
                {
                    std::memcpy(temp,    pTop,       nLast);
                    std::memcpy(pTop,    pBottom,    nLast);
                    std::memcpy(pBottom, temp,       nLast);
                }
            }
        }
        /// <summary>
        /// 垂直反转
        /// </summary>
        inline  void    flipX()
        {
            auto    nByte   =   FEFormatHelper::sizeOf(_format);
            for (uint32 y = 0; y < _height; ++y) 
            {
                auto    pRow    =   _buffer + y * _pitch;
                auto    left    =   pRow;
                auto    right   =   pRow + (_width - 1) * nByte;

                while (left < right) 
                {
                    for (uint c = 0; c < nByte; ++c) 
                    {
                        std::swap(left[c], right[c]);
                    }
                    left    +=  nByte;
                    right   -=  nByte;
                }
            }
        }
    protected:
        template<class TFrom,class TTo>
        inline  auto&   convert(const RectU32& rect,const TFrom& src)
        {
            const auto  tSrc    =   castTo<TFrom,TTo>(src);
            return  fill<TTo>(rect,tSrc);
        }
        template<class TFrom,class TTo>
        inline  auto&   convert(const RectU32& rect,const TFrom* src)
        {
            const auto  width   =   rect.width();
            const auto  height  =   rect.height();
            const auto  left    =   rect.left();
            const auto  top     =   rect.top();
            const auto  bottom  =   rect.bottom();
            for (uint32_t r = top; r < bottom; ++r)
            {   
                TTo*    pData   =   dataOffset<TTo>(r,c);
                for (uint32_t c = 0; c < width; c++)
                {
                    pData[c]  =   castTo<TFrom,TTo>(src[c]);
                }
            }
            return  *this;
        }
        template<class TFrom,class TTo>
        inline  auto&   convert(const RectU32& rect,const FEImageSource& src,const RectU32& rtSrc)
        {
            assert(rect.width() == rtSrc.width());
            assert(rect.height()== rtSrc.height());

            const auto  width   =   rect.width();
            const auto  height  =   rect.height();
            const auto  left    =   rect.left();
            const auto  top     =   rect.top();
            const auto  bottom  =   rect.bottom();

            const auto  srcTop  =   rtSrc.top();
            const auto  srcLeft =   rtSrc.left();

            for (uint32_t r = top; r < bottom; ++r)
            {   
                TTo*    pDst    =   dataOffset<TTo>(r,c);
                TFrom*  pSrc    =   src.dataOffset<TFrom>(srcTop + r,srcLeft);
                if (std::is_same_v<TTo,TFrom>)
                {
                    std::memcpy(pDst,pSrc,sizeof(TTo) * width);
                }
                else
                {
                    for (uint32_t c = 0; c < width; c++)
                    {
                        pData[c]  =   castTo<TFrom,TTo>(src[c]);
                    }
                }
            }
            return  *this;
        }
        template<class TTo>
        inline  auto&   fill(const RectU32& rect,const TTo& color)
        {
            const auto  width   =   rect.width();
            const auto  height  =   rect.height();
            const auto  col     =   rect.left();
            const auto  top     =   rect.top();
            const auto  bottom  =   rect.bottom();
            for (uint32_t r = top; r < bottom; ++r)
            {   
                fill<TTo>(r,col,width,color);
            }
            return  *this;
        }
        template<class TTo>
        inline  auto&   fill(const RectU32& rect,const TTo* pColor)
        {
            const auto  width   =   rect.width();
            const auto  height  =   rect.height();
            const auto  col     =   rect.left();
            const auto  top     =   rect.top();
            const auto  bottom  =   rect.bottom();
            for (uint32_t r = top; r < bottom; ++r)
            {   
                fill<TTo>(r,col,width,pColor);
                pColor  +=  width;
            }
            return  *this;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <typeparam name="TTo"></typeparam>
        /// <param name="r">行</param>
        /// <param name="c">列</param>
        /// <param name="width">像素数</param>
        /// <param name="src">颜色值</param>
        /// <returns></returns>
        template<class TTo>
        inline  auto&   fill(uint32 r,uint32 c,uint32 width,const TTo& color)
        {
            TTo*    pData   =   dataOffset<TTo>(r,c);
            std::fill(pData, pData + width, color);
            return  *this;
        }
        template<class TTo>
        inline  auto&   fill(uint32 r,uint32 c,uint32 width,const TTo* pColor)
        {
            TTo*    pData   =   dataOffset<TTo>(r,c);
            std::memcpy(pData,pColor,sizeof(TTo) * width);
            return  *this;
        }
    };
}

#pragma     once

#include    "../FEObject.h"
#include    "../FEBuffer.hpp"
#include    "../FEUserObject.hpp"
#include    "../graphic/FEGPUBuffer.h"
#include    "FEPrimitive.hpp"
#include    "FEAttribute.hpp"

namespace   FE
{
    struct  FEAttribyteBuffer
    {
    public:
        FEAttribyteBuffer(FEContext& ctx,Buffer buf,FEAttribute id)
            :_ctx(ctx)
        {
            _buffer     =   buf;
            _attrId     =   id;
        }
        FEAttribyteBuffer(const FEAttribyteBuffer& other)
            :_ctx(other._ctx)
        {
            _buffer     =   other._buffer;
            _attrId     =   other._attrId;
        }
        auto&   operator    =   (const FE::FEAttribyteBuffer& other)
        {
            _buffer     =   other._buffer;
            _attrId     =   other._attrId;
            return  *this;
        }
        /// <summary>
        /// 返回字节数量
        /// </summary>
        /// <returns></returns>
        inline  uint64  length() const
        {
            assert(_buffer!=nullptr);
            if (_buffer)
                return  _buffer->length();
            else
                return  0;
        }
        /// <summary>
        /// 获取元素格式
        /// </summary>
        /// <returns></returns>
        inline  uint    count() const
        {
            assert(_buffer!=nullptr);
            if (_buffer)
                return  uint(_buffer->length() / _attrId.stride());
            else
                return  0;  
        }
        const   uint8*  data() const
        {
            assert(_buffer!=nullptr);
            if (_buffer)
                return  _buffer->data();
            else
                return  nullptr;  
        }
        const   auto&   buffer() const
        {
            return  _buffer;
        }
        inline  auto&   setBuffer(const void* pData,size_t length)
        {
            if (_buffer == nullptr)
                _buffer =   new FEBuffer(_ctx);

            auto&   buffer  =   _buffer->cInfo()._buffer;
            buffer.resize(length);
            memcpy(buffer.data(),pData,length);
            return  *this;
        }
        /// <summary>
        /// 设置缓冲区
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="notify"></param>
        /// <returns></returns>
        inline  auto&   setBuffer(Buffer buffer)
        {
            if (_buffer == buffer)
                return  *this;
            _buffer =   buffer;
            return  *this;
        }
        /// <summary>
        /// 设置缓冲区
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="notify"></param>
        /// <returns></returns>
        inline  auto&   setBuffer(uint8s&& buffer)
        {
            if (_buffer == nullptr)
            {
                _buffer =   new FEBuffer(_ctx);
                _buffer->cInfo()._buffer = std::move(buffer);
            }
            else
            {
                _buffer->cInfo()._buffer = std::move(buffer);
            }
            return  *this;
        }
        FEAttribute     attr() const
        {
            return  _attrId;
        }
        inline  auto&   setAttr(FEAttribute attrId)
        {
            _attrId =   attrId;
            return  *this;
        }
    public:
        /// <summary>
        /// 统一读取内核:按向量类型 V 与分量数 N 逐元素还原位置分量(缺失分量补 0),
        /// 由转换器完成各格式族的取值语义后并入包围盒
        /// </summary>
        template<typename V,uint N,typename Conv>
        inline  static  void    mergeElements(aabb3f& box,const Buffer& buff,size_t elements,Conv conv)
        {
            auto    pData   =   buff->dataAs<V>();
            for (size_t i = 0; i < elements; i++)
            {
                float3  value(0.0f);
                if constexpr (N == 1)
                {
                    value.x =   conv(pData[i]);
                }
                else
                {
                    value.x =   conv(pData[i].x);
                    value.y =   conv(pData[i].y);
                    if constexpr (N > 2)
                        value.z =   conv(pData[i].z);
                }
                box.merge(value);
            }
        }
        /// <summary>
        /// 会更新包围盒信息并返回
        /// </summary>
        /// <returns></returns>
        inline  aabb3f  calcAabb() const
        {
            aabb3f  box;
            do
            {
                if (_buffer == nullptr)
                    break;
                auto    attr    =   _attrId;
                auto    buff    =   _buffer;
                
                if (buff == nullptr)
                    break;
                auto    stride      =   attr.stride();
                auto    length      =   buff->length();
                auto    elements    =   length / stride;
                
                /// 分量转换器:各格式族共用的取值语义
                auto    asFloat     =   [](auto v) { return  float(v); };
                auto    asUnorm     =   [](auto v) { return  float(v) / 65535.0f; };
                auto    asSnorm     =   [](auto v) { float r = float(v) / 32767.0f; return  r < -1.0f ? -1.0f : r; };

                switch(attr.format())
                {
                /// FLOAT 族:直读 half/float
                case FMT_R16_FLOAT              :   mergeElements<half,  1>(box,buff,elements,asFloat);  break;
                case FMT_R16G16_FLOAT           :   mergeElements<half2, 2>(box,buff,elements,asFloat);  break;
                case FMT_R16G16B16_FLOAT        :   mergeElements<half3, 3>(box,buff,elements,asFloat);  break;
                case FMT_R16G16B16A16_FLOAT     :   mergeElements<half4, 3>(box,buff,elements,asFloat);  break;
                case FMT_R32_FLOAT              :   mergeElements<float, 1>(box,buff,elements,asFloat);  break;
                case FMT_R32G32_FLOAT           :   mergeElements<float2,2>(box,buff,elements,asFloat);  break;
                case FMT_R32G32B32_FLOAT        :   mergeElements<float3,3>(box,buff,elements,asFloat);  break;
                case FMT_R32G32B32A32_FLOAT     :   mergeElements<float4,3>(box,buff,elements,asFloat);  break;
                /// 16bit UNORM:[0,65535] -> [0,1]
                case FMT_R16G16B16_UNORM        :   mergeElements<ushort3,3>(box,buff,elements,asUnorm); break;
                case FMT_R16G16B16A16_UNORM     :   mergeElements<ushort4,3>(box,buff,elements,asUnorm); break;
                /// 16bit SNORM:[-32767,32767] -> [-1,1]
                case FMT_R16G16B16_SNORM        :   mergeElements<short3,3>(box,buff,elements,asSnorm);  break;
                case FMT_R16G16B16A16_SNORM     :   mergeElements<short4,3>(box,buff,elements,asSnorm);  break;
                /// 16bit 无符号整数族(USCALED/UINT):原值
                case FMT_R16G16B16_USCALED      :
                case FMT_R16G16B16_UINT         :   mergeElements<ushort3,3>(box,buff,elements,asFloat); break;
                case FMT_R16G16B16A16_USCALED   :
                case FMT_R16G16B16A16_UINT      :   mergeElements<ushort4,3>(box,buff,elements,asFloat); break;
                /// 16bit 有符号整数族(SSCALED/SINT):原值
                case FMT_R16G16B16_SSCALED      :
                case FMT_R16G16B16_SINT         :   mergeElements<short3,3>(box,buff,elements,asFloat); break;
                case FMT_R16G16B16A16_SSCALED   :
                case FMT_R16G16B16A16_SINT      :   mergeElements<short4,3>(box,buff,elements,asFloat); break;
                }
                return  box;
            } while (false);
            return  {};
        }
    protected:
        FEContext&      _ctx;
        Buffer          _buffer;
        FEAttribute     _attrId;
    };
}

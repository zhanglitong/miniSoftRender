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
                
                switch(attr.format())
                {
                case FMT_R16_FLOAT          : 
                    {
                        auto    pData   =   buff->dataAs<half>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float   x   =  pData[i]; 
                            float3  value(x,0.0f,0.0f);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R16G16_FLOAT       :
                    {
                        auto    pData   =   buff->dataAs<half2>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float   x   =  pData[i].x; 
                            float   y   =  pData[i].y; 
                            float3  value(x,y,0.0f);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R16G16B16_FLOAT    :   
                    {
                        auto    pData   =   buff->dataAs<half3>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float   x   =  pData[i].x; 
                            float   y   =  pData[i].y; 
                            float   z   =  pData[i].z;
                            float3  value(x,y,z);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R16G16B16A16_FLOAT :
                    {
                        auto    pData   =   buff->dataAs<half4>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float   x   =  pData[i].x; 
                            float   y   =  pData[i].y; 
                            float   z   =  pData[i].z;
                            float3  value(x,y,z);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R32_FLOAT          : 
                    {
                        auto    pData   =   buff->dataAs<float>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float3  value(pData[i],0.0f,0.0f);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R32G32_FLOAT       :   
                    {
                        auto    pData   =   buff->dataAs<float2>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float3  value(pData[i].x,pData[i].y,0.0f);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R32G32B32_FLOAT    :  
                    {
                        auto    pData   =   buff->dataAs<float3>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float3  value(pData[i].x,pData[i].y,pData[i].z);
                            box.merge(value);
                        }
                    }
                    break;
                case FMT_R32G32B32A32_FLOAT :
                    {
                        auto    pData   =   buff->dataAs<float4>();
                        for (size_t i = 0; i < elements; i++)
                        {
                            float3  value(pData[i].x,pData[i].y,pData[i].z);
                            box.merge(value);
                        }
                    }
                    break;
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

#pragma     once

#include    "../FEBuffer.hpp"
#include    "FEReader.hpp"

namespace   FE
{
    class   FEReaderMemory : public FEReader
    {
    protected:
        uint64_t    _cur    =   0;
        Buffer      _buffer;
    public:
        FEReaderMemory(FEContext& ctx,Buffer buffer)
            :FEReader(ctx)
        {
            _buffer =   buffer;
        }
        inline  bool    end() const
        {
            if (_buffer == nullptr)
                return  true;
            else
                return  (_cur >= _buffer->length());
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="data"></param>
        /// <param name="len"></param>
        /// <returns></returns>
        virtual uint64  readBuffer(void* data,uint64 len) override
        {
            assert(_buffer!=nullptr && _buffer->length() != 0 && data != nullptr && len != 0);
            if (data == 0 || len == 0 || _buffer == nullptr || _buffer->length() == 0)
                return  0;
            assert(_cur + len <= _buffer->length());
            if (_cur + len > _buffer->length()) 
                return  0;
            memcpy(data,_buffer->data() + _cur,len);
            _cur    +=  len;
            return  len;
        }
        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const override
        {
            return  _cur;
        }
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        virtual uint64   seek(uint64 pos) override
        {
            auto    old =   tell();
            _cur    =   pos;
            return  old;
        }
        /// <summary>
        /// 跳过
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        virtual uint64  skip(int64_t pos) override
        {
           return   seek(_cur + pos);
        }
        /// <summary>
        /// 获取长度
        /// </summary>
        /// <returns></returns>
        virtual uint64  length() const override
        {
            if (_buffer == nullptr)   
                return  0;
            else
                return  _buffer->length();
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        virtual bool    isValid() const override
        {
            return  _buffer != nullptr;
        }
        /// <summary>
        /// 关闭
        /// </summary>
        virtual void    close() override
        {
            _buffer   =   nullptr;
        }
    };

    using   MemoryReader    =   SharedPtr<FEReaderMemory>;
}


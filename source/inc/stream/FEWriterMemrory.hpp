#pragma     once

#include    "../FEBuffer.hpp"
#include    "FEWriter.hpp"

namespace   FE
{
    class   FEWriterMemory :public FEWriter
    {
    protected:
        uint64      _cur    =   0;
        Buffer      _buffer =   nullptr;
    public:
        FEWriterMemory(FEContext& ctx,Buffer ptr)
            :FEWriter(ctx)
        {
            _buffer =   ptr;
        }
        /// <summary>
        /// 写缓冲区
        /// </summary>
        /// <param name="data">数据地址 </param>
        /// <param name="len">数据长度</param>
        /// <param name="tag">数据标签</param>
        /// <returns>写入大小,0失败</returns>
        virtual uint64  writeBuffer(const void* data,uint64 len,const Tag& tag = {}) override
        {
            if (data == nullptr || len == 0)
                return  0;
            if (_cur + len <= _buffer->length())
            {
                memcpy(_buffer->data() + _cur,data,len);
                _cur    +=  len;
                return  len;
            }
            return  0;
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
        virtual uint64  seek(uint64_t pos) override
        {
            auto    old =   _cur;
            _cur    =   pos;
            return  old;
        }
    };
}


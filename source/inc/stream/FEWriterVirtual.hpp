#pragma     once
#include    "FEWriter.hpp"

namespace   FE
{
    class   FEWriterVirtual :public FEWriter
    {
    protected:
        uint64    _cur    =   0;
    public:
        FEWriterVirtual(FEContext& ctx)
            :FEWriter(ctx)
        {
        }
        /// <summary>
        /// 写缓冲区
        /// </summary>
        /// <param name="data">数据地址 </param>
        /// <param name="len">数据长度</param>
        /// <param name="tag">数据标签</param>
        /// <returns>写入大小,0失败</returns>
        virtual uint64  writeBuffer(const void*,uint64 len,const Tag& tag = {}) override
        {
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
        virtual uint64  seek(uint64 pos) override
        {
            auto    old =   _cur;
            _cur    =   pos;
            return  old;
        }
    };
}


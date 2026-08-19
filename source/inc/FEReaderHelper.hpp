#pragma     once

#include    "FEBuffer.hpp"
#include    "stream/FEReader.hpp"
#include    "stream/FEReaderFile.hpp"
#include    "stream/FEReaderMemory.hpp"

namespace   FE
{
    class   FEReaderHelper
    {
    protected:
        FEReader&   _reader;
        uint64_t    _start;
    public:
        FEReaderHelper(FEReader& reader)
            :_reader(reader)
        {
            _start  =   _reader.tell();
        }
        ~FEReaderHelper()
        {}
        inline  Buffer  read(FEContext& ctx) 
        {
            Buffer  buffer  =   new FEBuffer(ctx);
            auto    length  =   _reader.length();
            buffer->cInfo()._buffer.resize(length);
            _reader.readBuffer(buffer->data(),length);
            return  buffer;
        }
        /// <summary>
        /// 获取写的字节数
        /// </summary>
        /// <returns></returns>
        uint64_t    size() const
        {
            return  _reader.tell() - _start;
        }
    };
}


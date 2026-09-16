
#include    "../inc/FEBuffer.hpp"
#include    "../inc/stream/FEReader.hpp"
#include    "../inc/stream/FEReaderFile.hpp"

namespace   FE
{
    Buffer     FEBuffer::loadFile(FEContext& ctx,const char* fileName,const char* mode)
    {
        FEReaderFile    reader(ctx,fileName,mode);
        if (!reader.isValid())
            return  nullptr;
        auto        len     =   reader.length();
        Buffer      buf     =   new FEBuffer(ctx);
        auto&       cInf    =   buf->cInfo();
        cInf._buffer.resize(len);
        reader.readBuffer(cInf._buffer.data(),len);
        reader.close();
        return      buf;
    }
}

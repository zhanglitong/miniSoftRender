
#include    "../inc/FEBuffer.hpp"
#include    "../inc/FEAssetsMgr.h"
#include    "../inc/stream/FEReader.hpp"
#include    "../inc/stream/FEReaderFile.hpp"

namespace   FE
{
    Buffer     FEBuffer::loadFile(FEContext& ctx,const char* fileName,const char* mode)
    {
        if (fileName == nullptr)
            return  nullptr;
        /// 资源内嵌加载:":/data/xxx" 走 FEAssetsMgr。
        /// 资源 key 相对于资源根目录(bin/data),不含 "data/" 前缀,
        /// 因此先按原相对路径查询,再尝试去掉 "data/" 后查询
        if (fileName[0] == ':' && fileName[1] == '/')
        {
            const   char*   rel =   fileName + 2;
            Buffer          buf =   FEAssetsMgr::instance().queryBuffer(rel);
            if (buf == nullptr && strncmp(rel,"data/",5) == 0)
                buf =   FEAssetsMgr::instance().queryBuffer(rel + 5);
            return  buf;
        }
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

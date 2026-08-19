#pragma     once
#include    "../FEChunkInf.hpp"
#include    "../FEObject.h"
#include    "../FEDefine.h"
namespace   FE
{
    class   FEReaderFile :public FEReader
    {
    protected:
        FILE*   _file;
    public:
        FEReaderFile(FEContext& ctx,const char* filePathName,const char* mode = "rb")
            :FEReader(ctx)
        {
            _file   =   fopen(filePathName,mode);
        }
        ~FEReaderFile()
        {
            close();
        }
    public:
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        virtual bool    isValid() const override
        {
            return  _file != nullptr;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="data"></param>
        /// <param name="len"></param>
        /// <returns></returns>
        virtual uint64  readBuffer(void* data,uint64 len) override
        {
            assert(_file!=nullptr && data != nullptr && len != 0);
            if (_file == nullptr || data == nullptr || len == 0)   
                return  0;
            auto    result  =   fread(data,1,len,_file);
            assert(result == len);
            return  result;
        }

        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const override
        {
            assert(_file!=nullptr);
            if (_file == nullptr)   
                return  0;
            else
                return  _ftelli64(_file);
        }
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns>上一次的位置</returns>
        virtual uint64  seek(uint64 pos) override
        {
            uint64  old =   tell();
            assert(_file!=nullptr);
            if (_file == nullptr)   
                return  old;
            _fseeki64(_file,pos,SEEK_SET);
            return  old;
        }
        /// <summary>
        /// 跳过
        /// </summary>
        /// <param name="pos"></param>
        /// <returns>上一次的位置</returns>
        virtual uint64  skip(int64 pos) override
        {
           return   seek(tell() + pos);
        }
        /// <summary>
        /// 获取长度
        /// </summary>
        /// <returns></returns>
        virtual uint64  length() const override
        {
            if (_file == nullptr)   
                return  0;
            _fseeki64(_file,0,SEEK_END);
            auto    len =   _ftelli64(_file);
            _fseeki64(_file,0,SEEK_SET);
            return  len;
        }
        
        virtual void    close() override
        {
            if (_file == nullptr)
                return;
            fclose(_file);
            _file   =   nullptr;
        }
    };

    using   FEReaderPtr     =   SharedPtr<FEReader>;
}


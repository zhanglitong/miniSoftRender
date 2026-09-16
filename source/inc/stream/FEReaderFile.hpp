#pragma     once
#include    "../FEChunkInf.hpp"
#include    "../FEObject.h"
#include    "../FEDefine.h"
#include    "../FEAssetsMgr.h"
namespace   FE
{
    class   FEReaderFile :public FEReader
    {
    protected:
        char*       _buffer =   nullptr;
        FILE*       _file   =   nullptr;
        Buffer      _memory =   nullptr;
        size_t      _cur    =   0;
        size_t      _size   =   0;
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
        /// 通过资源Id构造文件对象
        /// </summary>
        /// <param name="relPath">资源名称，相对路径</param>
        /// <returns></returns>
        inline  bool    open(const char* relPath)
        {
            /// 说明不是系统目录，按照常规文件处理
            if (relPath != nullptr)
            {
                /// 查询相对目录，如果有缓冲区，则使用缓冲区
                auto    buffer  =   FEAssetsMgr::instance().queryBuffer(relPath);
                _file   =   nullptr;
                _cur    =   0;
                _memory =   buffer;
                _size   =   buffer ? buffer->length() : 0;
                return  buffer ? true : false;
            }
            else
            {
                _file   =   nullptr;
                _memory =   nullptr;
                _cur    =   0;
                _size   =   0;
                return  false;
            }
        }
        /// <summary>
        /// 是否是资产对象
        /// </summary>
        /// <returns></returns>
        inline  bool    isAssets() const
        {
            return  _memory != nullptr;
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        virtual bool    isValid() const override
        {
            return  _file != nullptr || _memory != nullptr;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="data"></param>
        /// <param name="len"></param>
        /// <returns></returns>
        virtual uint64  readBuffer(void* dst,uint64 len) override
        {
            if (_file)
                return  fread(dst,1,len,_file);
            else if(_memory == nullptr)
                return  0;
            assert(_cur + len <= _size);
            if (_cur + len <= _size)
            {
                memcpy(dst,(char*)(_memory->data())+ _cur,len);
                _cur    +=  len;
                return  len;
            }
            else
            {
                memcpy(dst,(char*)(_memory->data())+ _cur,(_cur + len) - _size);
                _cur    =  _size;
                return  (_cur + len) - _size;
            }
        }
        
        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const override
        {
            if (_file != nullptr)
                return  _ftelli64(_file);
            else
                return  _cur;
        }
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns>上一次的位置</returns>
        virtual uint64  seek(uint64 pos) override
        {
            assert(_file!=nullptr);
            if (_file )   
            {
                uint64  old =   tell();
                _fseeki64(_file,pos,SEEK_SET);
                return  old;
            }
            else
            {
                uint64  old =   _cur;
                _cur        =   pos;
                return  old;
            }
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
            if (_file)
            {
                fclose(_file);
                _file   =   nullptr;
            }
            _memory =   nullptr;
            _cur    =   0;
            _size   =   0;
        }
        virtual bool    end()
        {
            if (_file)
                return  feof(_file) != 0;
            else
                return  _cur >= _size;
        }
    };

    using   FEReaderPtr     =   SharedPtr<FEReader>;
}


#pragma     once
#include    "FEWriter.hpp"
namespace   FE
{
    class   FEWriterFile :public FEWriter
    {
    protected:
        FILE*   _file   =   nullptr;
    public:
        FEWriterFile(FEContext& ctx,const char* fileName)
            :FEWriter(ctx)
        {
            _file   =   fopen(fileName,"wb");
        }
        ~FEWriterFile()
        {
            if (_file)
            {
                fclose(_file);
            }
        }
        inline  void    detach()
        {
            _file   =   nullptr;
        }
        
        inline  bool    isValid() const
        {
            return  _file != nullptr;
        }
        /// <summary>
        /// 写缓冲区
        /// </summary>
        /// <param name="data">数据地址 </param>
        /// <param name="len">数据长度</param>
        /// <param name="tag">数据标签</param>
        /// <returns>写入大小,0失败</returns>
        virtual uint64  writeBuffer(const void*data,uint64 len ,const Tag& tag = {}) override
        {
            (void)tag;
            assert(_file != nullptr);
            if (_file == nullptr || data == nullptr || len == 0)
                return  0;
            else
                return  fwrite(data,1,len,_file);
        }
        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const    override
        {
            assert(_file != nullptr);
            if (_file == nullptr)
                return  0;
            else
                return  _ftelli64(_file);
        }
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        virtual uint64  seek(uint64_t pos) override
        {
            assert(_file != nullptr);
            if (_file == nullptr)
                return  0;
            auto    old =   tell();
            _fseeki64(_file,pos,SEEK_SET);
            return  old;
        }
        /// <summary>
        /// 关闭
        /// </summary>
        virtual  void    close()
        {
            if (_file)
            {
                fclose(_file);
                _file   =   nullptr;
            }
        }
    };
}


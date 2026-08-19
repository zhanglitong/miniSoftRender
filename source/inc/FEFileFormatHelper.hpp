#pragma     once
#include    "FEStreamReader.hpp"
#include    "FEStreamWriter.hpp"
#include    "FEObjectHelper.hpp"

namespace   FE
{
    class   FEFileFormatHelper 
    {
    public:
        using   Reader  =   SharedPtr<FEStreamReader>;
        using   Writer  =   SharedPtr<FEStreamWriter>;
    public:
        /// <summary>
        /// 创建任务
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="files"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Reader  make(FEContext& ctx,const Strings& files,const Format& fmt = {})
        {
            namespace fs = std::filesystem;

            if (files.empty())
                return  nullptr;
            /// 1. 使用参数的格式
            Format  fmtIn   =   fmt;
            /// 2. 如果不可用，则根据扩展名获取
            if (!fmtIn.isValid())
            {
                fs::path    stdFile(files.front());
                auto        ext     =   stdFile.extension().string();
                strcpy(fmtIn.ext(),ext.c_str());
            }
            /// 3. 根据格式获取加载器
            auto        reader   =   queryReader(ctx,fmtIn);
            /// 4. 没有获取到
            if (reader == nullptr)
                return  nullptr;
            /// 5. 获取到了
            reader->setParam(TBReadFiles,files);
            return  reader.get();
        }
        /// <summary>
        /// 创建任务
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="buffers"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Reader  make(FEContext& ctx,const Buffers& buffers,const Format& fmt)
        {
            if (buffers.empty())
                return  nullptr;
            /// 1. 根据格式获取加载器
            auto    reader  =   queryReader(ctx,fmt);
            /// 2. 没有获取到
            if (reader == nullptr)
                return  nullptr;
            /// 3. 获取到了
            reader->setParam(TBReadBuffers,buffers);
            return  reader;
        }
        /// <summary>
        /// 获取加载器
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Reader  queryReader(FEContext& _ctx,const Format& fmt)
        {
            const auto  key =   fmt.toString();
            Format  fFmt;
            if (!_ctx.readers().query(key,fFmt))
            {
                LOG_INF("ctx.readers().query(%s) return false",key.c_str());
                return  nullptr;
            }
            /// 检测模式是否兼容,例如插件支持文件模式，不支持内存模式
            /// 但输入的是内存模式
            if (!fFmt._mode.containFlag(fmt._mode.data()))
            {
                LOG_INF("fFmt._mode.containFlag(%d) return false",fmt._mode.data());
                return  nullptr;
            }
            auto    result  =   FEObjectHelper::createObject(_ctx,fFmt.loaderId());
            if (result == nullptr)
                return  nullptr;
            auto    reader  =   result->cast<FEStreamReader>();
            if (reader == nullptr)
            {
                LOG_INF("result->cast<FEStreamReader>() return nullptr");
                return  nullptr;
            }
            return  reader;
        }
    public:
        /// <summary>
        /// 创建任务
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="files"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Writer  make(FEContext& ctx,const String& file,const Format& fmt = {})
        {
            namespace fs = std::filesystem;

            if (file.empty())
                return  nullptr;
            /// 1. 使用参数的格式
            Format  fmtIn   =   fmt;
            /// 2. 如果不可用，则根据扩展名获取
            if (!fmtIn.isValid())
            {
                fs::path    stdFile(file);
                auto        ext     =   stdFile.extension().string();
                strcpy(fmtIn.ext(),ext.c_str());
            }
            /// 3. 根据格式获取加载器
            auto    writer   =   queryWriter(ctx,fmtIn);
            /// 4. 没有获取到
            if (writer == nullptr)
                return  nullptr;
            /// 5. 获取到了
            writer->setParam(TBWriteFile,file);
            return  writer;
        }
        /// <summary>
        /// 创建任务
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="buffers"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Writer  make(FEContext& ctx,const Buffer& buffer,const Format& fmt)
        {
            if (buffer == nullptr)
                return  nullptr;
            /// 1. 根据格式获取加载器
            auto    writer   =   queryWriter(ctx,fmt);
            /// 2. 没有获取到
            if (writer == nullptr)
                return  nullptr;
            /// 3. 获取到了
            writer->setParam(TBWriteBuffer,buffer);
            return  writer;
        }
        /// <summary>
        /// 获取加载器
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  Writer  queryWriter(FEContext& _ctx,const Format& fmt)
        {
            const auto  key =   fmt.toString();
            Format      fFmt;
            if (!_ctx.writers().query(key,fFmt))
            {
                LOG_INF("ctx.writers().query(%s) return false",key.c_str());
                return  nullptr;
            }
            /// 检测模式是否兼容,例如插件支持文件模式，不支持内存模式
            /// 但输入的是内存模式
            if (!fFmt._mode.containFlag(fmt._mode.data()))
            {
                LOG_INF("fFmt._mode.containFlag(%d) return false",fmt._mode.data());
                return  nullptr;
            }
            auto    result  =   FEObjectHelper::createObject(_ctx,fFmt.loaderId());
            if (result == nullptr)
                return  nullptr;
            auto    writer  =   result->cast<FEStreamWriter>();
            if (writer == nullptr)
            {
                LOG_INF("result->cast<FEStreamWriter>() return nullptr");
                return  nullptr;
            }
            return  writer;
        }
    };
}

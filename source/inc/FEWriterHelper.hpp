#pragma     once
#include    "stream/FEWriter.hpp"
#include    "stream/FEWriterFile.hpp"
#include    "stream/FEWriterMemrory.hpp"
#include    "stream/FEWriterVirtual.hpp"

namespace   FE
{
    class   FEWriterHelper;
    using   NotifyFlush     =   std::function<void(FEWriterHelper& helper)>;
    class   FEWriterHelper
    {
    public:
        FEWriter&       _writer;
    protected:
        uint64_t        _start;
        NotifyFlush     _flushNotify;
    public:
        /// <summary>
        /// flushNotify 当写完数据后调用，目的是回填数据头，需要不外自定义实现
        /// infor 是数据头
        /// auto    notify  =   [](FEWriterHelper& helper)
        /// {   
        ///     uint64  nCur    =   helper._writer.tell();
        ///     helper._writer.seek(helper.start());
        ///     helper._writer.write(infor);
        ///     helper._writer.seek(nCur);
        /// });
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="flushNotify"></param>
        FEWriterHelper(FEWriter&writer,const NotifyFlush& flushNotify = {})
            :_writer(writer)
        {
            _flushNotify    =   flushNotify;
            _start          =   _writer.tell();
        }
        ~FEWriterHelper()
        {
            if (_flushNotify)
            {
                _flushNotify(*this);
            }
        }
        uint64_t    start() const
        {
            return  _start;
        }
        /// <summary>
        /// 获取写的字节数
        /// </summary>
        /// <returns></returns>
        uint64_t    size() const
        {
            return  _writer.tell() - _start;
        }
    };
}


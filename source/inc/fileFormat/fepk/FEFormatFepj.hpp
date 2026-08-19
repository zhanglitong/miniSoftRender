#pragma     once
#include    "../../FEFileFormat.hpp"
#include    "../../FEWriterHelper.hpp"
#include    "../../FEReaderHelper.hpp"

namespace   FE
{
    /// #define CLASS_INDEX
    struct  FEFepjHeader
    {
        struct  Header
        {
            char    _magic[4]   =   {'f','e','p','j'};
            uint    _version    =   MAKE_VERSION(1,0,0,0);
            uint64  _datetime   =   0;
            uint64  _nodeCnt    =   0;
            uint64  _objectCnt  =   0;
        };
        FEFepjHeader(bool cacheFromSystem )
        {

            (void)cacheFromSystem;
#ifdef  CLASS_INDEX
            /// 遍历获取类型id
            /// 缓存下来
            /// 获取系统所有创建器
            if (cacheFromSystem)
            {
                auto&   creators    =   FEContext::creators();
                for (auto& var : creators.data())
                {
                    _classIds.push_back(var.first);
                }
                std::sort(_classIds.begin(),_classIds.end());
            }
#endif
        }
        bool    isValid() const
        {
            return  strncmp(_header._magic,"fepj",4) == 0;
        }
        size_t    write(FEWriter& writer)
        {
            auto    result  =   writer.write(_header);
#ifdef  CLASS_INDEX
            uint    cnt =   (uint)_classIds.size();
            result      +=  writer.write(cnt);
            for (uint i = 0; i < cnt; i++)
            {
                result  +=  writer.write(_classIds[i]);
            }
#endif
            return  result;
        }
        size_t  read(FEReader& reader)
        {
            auto    result  =  reader.read(_header);
#ifdef  CLASS_INDEX
            uint    cnt =   0;
            reader.read(cnt);
            _classIds.resize(cnt);
            for (uint i = 0; i < cnt; i++)
            {
                result  +=  reader.read(_classIds[i]);
            }
#endif
            return  result;
        }

        /// <summary>
        /// 根据 id获取索引
        /// </summary>
        /// <param name="classId"></param>
        /// <returns>result >= 0 ,找到了，否则没有找到</returns>
        int16   index(const FEUuid& classId)
        {
            (void)classId;
#ifdef  CLASS_INDEX
            auto it = std::lower_bound(_classIds.begin(), _classIds.end(), classId);
            if (it != _classIds.end() && *it == classId) 
            {
                size_t  index    = std::distance(_classIds.begin(), it);
                return  int16(index);
            }
            else
            {
                return  -1;
            }
#else
            return  -1;
#endif
        }
    public:
        Header  _header;
#ifdef  CLASS_INDE
        /// <summary>
        /// 写入当当前文件中的类型id,系统会根据类型进行排序;
        /// 重复的对象如果写入类型id比较消耗磁盘空间
        /// 所以使用一个ushort做映射,把一个类型映衬成一个0~SHORT_MAX的值，减少空间占用
        /// </summary>
        Uuids   _classIds;
#endif
    };

    class   FEFepjFormat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
            fmt._loaderId   =   id;
            fmt._type       =   FEFileFormat::DT_Model;
            fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
            return  {fmt};
        }
    };
}

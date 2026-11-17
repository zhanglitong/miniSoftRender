#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEDateTime.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../FEEntryList.hpp"
#include    "../../FEFileInfor.hpp"
#include    "FEFormatFepj.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatFepjReader,"{D66C720A-3FB9-4794-B139-D360231E46E5}");

    class   FEFormatFepjReader :public FEStreamReader
    {
    public:
        using   Header  =   FEFepjHeader;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatFepjReader)
    public:
        FEFormatFepjReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatFepjReader(const FEFormatFepjReader& other)
            :FEStreamReader(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        /// <returns></returns>
        virtual Formats formats() const override
        {
            return  formatList();
        }
        /// <summary>
        /// 接口调用加载文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        virtual Objects readFiles(const Strings& files) override
        {
            FESerializeCtx  xsCtx;
            ObjectUMap      objectMap;
            Objects         result;
            FEReader*       pReader =   nullptr;
            uint64          offset  =   0;
            xsCtx.query     =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                switch(option)
                {
                case FESerializeCtx::O_Query:
                    {
                        auto    itr =   objectMap.find(id);
                        if (itr != objectMap.end())
                            return  {itr->second.get(),0};
                        else
                            return  {};
                    }
                    break;
                case FESerializeCtx::O_Cache:
                    objectMap[id]   =   object;
                    return  {object,0};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)this,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        FEReader*   reader  =   object ? object->as<FEReader>() : nullptr;
                        if (reader != nullptr && xsCtx.change)
                        {
                            offset          +=  pReader ? pReader->length() : 0;
                            pReader         =   reader;
                            xsCtx.change    =   false;
                        }
                        if (reader)
                            xsCtx.doByte    =  offset   +   reader->tell();
                        auto    per     =   double(xsCtx.doByte)/(std::max)(1.0,double(xsCtx.allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
            xsCtx.reserve   =   [&](size_t count)
            {
                objectMap.reserve(objectMap.size() + count);
            };
            /// 计算所有文件大小
            for (auto& var : files)
            {
                xsCtx.allByte +=  FEFileInfor::fileSize(var);
            }
            setParam(TBCXSerialize,&xsCtx);
            for (auto& file : files)
            {
                FEReaderFile    reader(_ctx,file.c_str());
                if (!reader.isValid())
                {
                    LOG_ERR("fepj: cannot open %s",file.c_str());
                    continue;
                }
                FEReaderHelper  helper(reader);
                Buffer  buffer  =   helper.read(_ctx);
                reader.close();
                auto    temp  =   readBuffers({buffer});
                result.insert(result.end(),temp.begin(),temp.end());
            }
            /// 多文件时跨文件引用可能在读后面的文件时才齐,统一在全部读完后告警
            const   size_t  unresolved  =   xsCtx.resolvePending();
            if (unresolved > 0)
                LOG_WAR("fepj: %zu pending reference(s) unresolved",unresolved);
            /// 参数里存的是栈上 xsCtx 的地址,离开作用域前必须清掉,
            /// 否则 reader 之后直接调用 readBuffers 会取到悬垂指针
            setParam(TBCXSerialize,TaskParam{});
            return  result;
        }
        /// <summary>
        /// 输入多个buffer,从多个buffer中读取对象数据
        /// </summary>
        /// <param name="buffers"></param>
        /// <param name="params"></param>
        /// <returns></returns>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            FESerializeCtx  newCtx;
            FESerializeCtx* pCtx    =   &newCtx;
            ObjectUMap      objectMap;
            FEReader*       pReader =   nullptr;
            uint64          offset  =   0;
            for (auto var : buffers)
            {
                newCtx.allByte   +=  var->length();
            }
            /// 查询函数
            newCtx.query        =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                switch(option)
                {
                case FESerializeCtx::O_Query:
                    {
                        auto    itr =   objectMap.find(id);
                        if (itr != objectMap.end())
                            return  {itr->second.get(),0};
                        else
                            return  {};
                    }
                    break;
                case FESerializeCtx::O_Cache:
                    objectMap[id]   =   object;
                    return  {object,0};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)this,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        FEReader*   reader  =   object ? object->as<FEReader>() : nullptr;
                        if (reader != nullptr && pCtx->change)
                        {
                            offset          +=  pReader ? pReader->length() : 0;
                            pReader         =   reader;
                            pCtx->change    =   false;
                        }
                        if (reader)
                            pCtx->doByte    =   offset   +   reader->tell();
                        auto    per     =   double(pCtx->doByte)/(std::max)(1.0,double(pCtx->allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
            newCtx.reserve      =   [&](size_t count)
            {
                objectMap.reserve(objectMap.size() + count);
            };
            /// 先查找是否已经设置了
            auto        parCtx  =   param(TBCXSerialize);
            /// 如果已经设置，则使用设置的
            if (!std::holds_alternative<std::monostate>(parCtx))
            {
                pCtx    =   std::get<FESerializeCtx*>(parCtx);
            }
            Objects         results;
            for (auto var : buffers)
            {
                FEReaderMemory  reader(_ctx,var);
                Header          header(false);
                auto            res     =   header.read(reader);
                /// 头部长度和 magic 必须匹配,否则不进入对象流
                if (res < sizeof(FEFepjHeader::Header) || !header.isValid())
                {
                    LOG_ERR("fepj: invalid header (read %llu of %zu bytes)",(uint64)res,sizeof(FEFepjHeader::Header));
                    continue;
                }
                /// 主版本不一致时无法保证布局兼容,直接拒绝
                if (((header._header._version >> 24) & 0xFF) != ((FEFepjHeader::Header()._version >> 24) & 0xFF))
                {
                    LOG_ERR("fepj: unsupported version 0x%08X",header._header._version);
                    continue;
                }
                /// 用头部计数预分配对象表;计数异常(超过总字节数)时忽略,避免损坏文件触发超大分配
                if (pCtx->reserve && header._header._objectCnt > 0 && header._header._objectCnt < pCtx->allByte)
                    pCtx->reserve(size_t(header._header._objectCnt));
                /// 入口对象列表按 buffer 独立收集,避免多 buffer 时重复返回
                Objects         entryList;
                pCtx->preCLSId          =   CLSId();
                pCtx->preCreator        =   {};
                pCtx->change            =   true;
                do
                {
                    const   uint64  pos     =   reader.tell();
                    uint            nVer    =   0;
                    Object          object  =   FEObjectHelper::readObject(_ctx,reader,nVer,*pCtx);
                    if (object == nullptr)
                    {
                        /// 读指针没有前进说明剩余字节不足以组成一个对象块,
                        /// 再 continue 会原地死循环
                        if (reader.tell() == pos)
                            break;
                        continue;
                    }
                    pCtx->query(object->objectId(),object.get(),FESerializeCtx::O_Cache);
                    /// 检测是不是FEEntryList 
                    /// 该对象定义了要返回哪些对象
                    if (object->cast<FEEntryList>() != nullptr)
                    {
                        entryList.push_back(object);
                    }
                    
                } while (!reader.end());
                
                for (auto& var1 : entryList)
                {
                    auto        pList   =   var1->cast<FEEntryList>();
                    if (pList == nullptr)
                        continue;
                    Objects&    eList   =   pList->entryList();
                    results.insert(results.end(),eList.begin(),eList.end());
                }
            }
            /// 所有对象读完、对象表完整后,统一回填反序列化期间登记的前向引用
            /// 使用外部 ctx(多文件场景)时由 readFiles 在全部读完后统一告警
            const   bool    ownCtx      =   pCtx == &newCtx;
            const   size_t  unresolved  =   pCtx->resolvePending(ownCtx);
            if (unresolved > 0 && ownCtx)
                LOG_WAR("fepj: %zu pending reference(s) unresolved",unresolved);
            return  results;
        }
        /// <summary>
        /// 进度通知
        /// </summary>
        /// <param name="val"></param>
        virtual void    onProgress(real val)
        {
            printf("progress:%lf\n", val * 100.0);
        }
    public:
        static  Formats formatList()
        {
            return  FEFepjFormat::formats(UUIDOF(FEFormatFepjReader));
        }
    };
}

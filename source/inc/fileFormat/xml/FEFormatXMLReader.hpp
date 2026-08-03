#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEDateTime.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../FEEntryList.hpp"
#include    "../../FEFileInfor.hpp"
#include    "FEFormatXML.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatXMLReader,"{B7A88898-CC51-4280-B109-8B41E831AFA4}");

    class   FEFormatXMLReader :public FEStreamReader
    {
    public:
        using   Header  =   FEFepjHeader;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatXMLReader)
    public:
        FEFormatXMLReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatXMLReader(const FEFormatXMLReader& other)
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
            FESerializeCtx    xsCtx;
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
                    continue;
                FEReaderHelper  helper(reader);
                Buffer  buffer  =   helper.read(_ctx);
                reader.close();
                auto    temp  =   readBuffers({buffer});
                result.insert(result.end(),temp.begin(),temp.end());
            }
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
            FESerializeCtx    newCtx;
            FESerializeCtx*   pCtx    =   &newCtx;
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
                        auto    per     =   double(pCtx->doByte)/(std::max)(1.0,double(pCtx->allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
            /// 先查找是否已经设置了
            auto        parCtx  =   param(TBCXSerialize);
            /// 如果已经设置，则使用设置的
            if (!std::holds_alternative<std::monostate>(parCtx))
            {
                pCtx    =   std::get<FESerializeCtx*>(parCtx);
            }
            /// 存储入口对象列表
            Objects         entryList;
            Objects         results;
            for (auto var : buffers)
            {
                FEReaderMemory  reader(_ctx,var);
                Header          header(false);
                auto            res     =   header.read(reader);
                assert(res == sizeof(header));
                if (res != sizeof(header))
                    continue;
                FEChunkInf      chk     =   {};
                pCtx->preCLSId          =   CLSId();
                pCtx->preCreator        =   {};
                pCtx->change            =   true;
                do
                {
                    uint    nVer    =   0;
                    Object  object  =   FEObjectHelper::readObject(_ctx,reader,nVer,*pCtx);
                    if (object == nullptr)
                        continue;
                    pCtx->query(object->objectId(),object.get(),FESerializeCtx::O_Cache);
                    /// 检测是不是FEEntryList 
                    /// 该对象定义了要返回哪些对象
                    if (object->cast<FEEntryList>())
                    {
                        entryList.push_back(object);
                    }
                    
                } while (!reader.end());
                
                for (auto& var : entryList)
                {
                    auto        pList   =   var->cast<FEEntryList>();
                    if (pList == nullptr)
                        continue;
                    Objects&    eList   =   pList->entryList();
                    results.insert(results.begin(),eList.begin(),eList.end());
                }
                
            }
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
            return  FEFepjFormat::formats(UUIDOF(FEFormatXMLReader));
        }
    };
}
#pragma     once
#include    "../../FEStreamWriter.hpp"
#include    "../../FEDateTime.hpp"
#include    "../../FEEntryList.hpp"
#include    "../../FEWriterHelper.hpp"
#include    "../../FEXML.hpp"
#include    "FEFormatXML.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatXMLWriter,"{0EAF693E-09CA-4591-8DF8-3ADDA1A30097}");

    class   FEFormatXMLWriter :public FEStreamWriter
    {
    public:
        using   Header  =   FEFepjHeader;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatXMLWriter)
    public:
        FEFormatXMLWriter(FEContext& ctx)
            :FEStreamWriter(ctx)
        {}
        FEFormatXMLWriter(const FEFormatXMLWriter& other)
            :FEStreamWriter(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        /// <returns></returns>
        virtual Formats formats() const override
        {
            return  FEFormatXML::formats(classId());
        }
        /// <summary>
        /// 写文件
        /// </summary>
        /// <param name="ojects"></param>
        /// <param name="filePathName"></param>
        /// <returns></returns>
        virtual bool    writeFile(const Objects&ojects,const String& filePathName) override
        {
            FESerializeCtx    xsCtx;
            xsCtx.query =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                return  {nullptr,0};
            };
            FEWriterVirtual vFile(_ctx);
            uint64          nLen    =   write(vFile,ojects,xsCtx);
            if (nLen == 0)
                return  false;
            Buffer          buffer  =   new FEBuffer(_ctx);
            buffer->cInfo()._buffer.resize(nLen);
            /// 计算总大小
            xsCtx.allByte   =   vFile.tell();
            xsCtx.doByte    =   0;
            setParam(TBCXSerialize,&xsCtx);

            FEWriterMemory  mFile(_ctx,buffer);
            bool            resullt =    writeBuffer(ojects,mFile);
            assert(mFile.tell() == nLen);
            if (!resullt || mFile.tell() != nLen)
                return  false;
            std::string     tmp     =   filePathName;
                            tmp     +=  ".tmp";

            FEWriterFile    fFile(_ctx,tmp.c_str());
            if (fFile.isValid())
            {
                auto    nWrite  =   fFile.writeBuffer(buffer->data(),buffer->length());
                if (nWrite != buffer->length())
                    return  false;
                fFile.close();
            }
            std::filesystem::path   tmpFile     =   tmp;
            /// 删除原来的文件
            std::filesystem::path   filePath    =   filePathName;
            if (std::filesystem::exists(filePath))
                std::filesystem::remove(filePath);
            /// 重命名tmp-> filePath
            std::filesystem::rename(tmpFile,filePath);
            return  true;
        }
        /// <summary>
        /// 写Writer
        /// </summary>
        /// <param name="ojects"></param>
        /// <param name="writer"></param>
        /// <returns></returns>
        virtual bool    writeBuffer(const Objects&ojects,FEWriter& writer)  override
        {
            FESerializeCtx    xsCtx;
            FESerializeCtx*   pCtx    =   &xsCtx;
            ObjectUMap      objectMap;
            auto            paramCtx    =   param(TBCXSerialize);
            if (!std::holds_alternative<std::monostate>(paramCtx))
            {
                pCtx   =   std::get<FESerializeCtx*>(paramCtx);
            }
            xsCtx.query =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
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
                    return  {};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)this,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        pCtx->doByte    =   writer.tell();
                        auto    per     =   double(pCtx->doByte)/(std::max)(1.0,double(pCtx->allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
            auto    result  =   write(writer,ojects,xsCtx);
            return  result > 0;
        }

        virtual void    onProgress(real val)
        {
            printf("progress:%lf\n", val * 100.0);
        }
    public:
        static  Formats formatList()
        {
            return  FEFormatXML::formats(UUIDOF(FEFormatXMLWriter));
        }
    protected:
        /// <summary>
        /// 查询一个对象的依赖对象
        /// </summary>
        /// <param name="object">被查询对象</param>
        /// <param name="objectSet">输出依赖对象集合</param>
        /// <param name="results">与objectSet一致，是有序的，按照依赖关系</param>
        static  void    collectDepends(Object object,ObjectUSet& objectSet,Objects& results)
        {
            if (!object->supportSerial())
                return;
            ObjectUSet  depends;
            /// 查询object依赖的对象
            object->queryDepends(depends);
            /// 转存到线性表
            /// 准备查询依赖的依赖
            for (auto& var : depends)
            {
                if (!var->supportSerial())
                    continue;
                if (objectSet.find(var) != objectSet.end())
                    continue;
                results.push_back(var);
                objectSet.emplace(var);
            }
            /// 遍历子孙节点,优先按照层级递归
            if (object->cast<FENode>())
            {
                auto&   childs  =   object->cast<FENode>()->children();
                for (auto& var : childs)
                {
                    if (!var->supportSerial())
                        return;
                    collectDepends(var.get(),objectSet,results);
                }
            }
            
            /// 询依赖的依赖(递归) 深度递归
            for (auto& var : depends)
            {
                collectDepends(var,objectSet,results);
            }
        }
        inline  size_t  write(FEWriter& writer,const Objects&ojects,FESerializeCtx& xCTX)
        {
            XMLDoc      doc;
            XMLNode*    ver     =   doc.allocate_node(rapidxml::node_pi, "xml version='1.0' encoding='utf-8'", 0);
            doc.append_node(ver);

            /// 统计依赖
            ObjectUSet  objectSet;
            Objects     results;
            uint64      nStart  =   writer.tell();
            for (auto& var : ojects)
            {
                collectDepends(var,objectSet,results);
            }
            Header  header(true);
            header.write(writer);
            /// 依赖写入
            for (auto& var : results)
            {
                uint    version =   0;
                var->serialize(writer,version,xCTX);
            }
            /// 对象写入
            FEEntryList entryList(_ctx);
            /// 给入口数据赋值
            uint    nVersion =   0;
            for (auto& var : ojects)
            {
                if (!var->supportSerial())
                    continue;
                entryList.entryList().push_back(var);
                var->serialize(writer,nVersion,xCTX);
            }
            entryList.serialize(writer,nVersion,xCTX);
            return  writer.tell() - nStart;
        }
    };
}
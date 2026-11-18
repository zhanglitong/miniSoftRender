#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../FEProperty.hpp"
#include    "../../node/FENode.hpp"
#include    "FEFormatATT.hpp"
#include    <unordered_map>
#include    <cstring>
#if FE_PLATFORM == FE_PLATFORM_WIN32
    #include    <windows.h>
#endif

namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatATTReader,"{6F1C4A2D-8B3E-4D7A-9C21-5E8F0B7D4A61}");

    /// <summary>
    /// AVEVA/CADC RVM 属性文件(.att)读取器。
    /// 文本格式,每条记录以 "NEW 名称" 开始、以 "END" 结束,记录体内为 "键:= 值";
    /// 缩进只是排版,记录之间通过 NEW/END 描述层级。
    /// 解析结果为一组 FEProperty(code 为记录名),调用 attach 按 FENode 名称
    /// 挂接为节点组件。文件通常以本地 ANSI 编码(中文为 GBK)保存,读取时转 UTF-8。
    /// </summary>
    class   FEFormatATTReader :public FEStreamReader
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatATTReader)
    public:
        FEFormatATTReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatATTReader(const FEFormatATTReader& other)
            :FEStreamReader(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        virtual Formats formats() const override
        {
            return  formatList();
        }
        /// <summary>
        /// 从文件加载属性记录
        /// </summary>
        virtual Objects readFiles(const Strings& files) override
        {
            Objects result;
            for (auto& file : files)
            {
                FEReaderFile    reader(_ctx,file.c_str());
                if (!reader.isValid())
                {
                    LOG_ERR("ATT: open file failed: %s",file.c_str());
                    continue;
                }
                FEReaderHelper  helper(reader);
                Buffer          buffer  =   helper.read(_ctx);
                reader.close();
                if (buffer == nullptr || buffer->length() == 0)
                    continue;
                parse(buffer->data(),buffer->length(),result);
                if (isCanceled())
                    break;
            }
            setProgress(real(1));
            return  result;
        }
        /// <summary>
        /// 从内存加载属性记录
        /// </summary>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            Objects result;
            for (auto& buffer : buffers)
            {
                if (buffer == nullptr || buffer->length() == 0)
                    continue;
                parse(buffer->data(),buffer->length(),result);
                if (isCanceled())
                    break;
            }
            setProgress(real(1));
            return  result;
        }
    public:
        static  Formats formatList()
        {
            return  FEATTFomat::formats(UUIDOF(FEFormatATTReader));
        }
        /// <summary>
        /// 把属性记录按 code 与节点名匹配,作为组件挂接到节点上
        /// </summary>
        /// <param name="root">节点树根(含导入的模型树)</param>
        /// <param name="properties">readFiles/readBuffers 返回的属性记录</param>
        /// <returns>成功挂接的属性个数</returns>
        static  size_t  attach(Node root,const Objects& properties)
        {
            if (root == nullptr || properties.empty())
                return  0;
            std::unordered_map<String,FENode*>  nodes;
            collectNodes(root.get(),nodes);
            if (nodes.empty())
                return  0;

            size_t  count   =   0;
            for (auto object : properties)
            {
                if (object == nullptr)
                    continue;
                auto*   property    =   object->cast<FEProperty>();
                if (property == nullptr || property->code().empty())
                    continue;
                auto    itr =   nodes.find(property->code());
                if (itr == nodes.end() || itr->second == nullptr)
                    continue;
                if (itr->second->addComponent(Component(property)))
                    ++  count;
            }
            return  count;
        }
    protected:
        /// <summary>
        /// 收集节点树中的名称 -> 节点映射(同名节点后出现的覆盖先出现的)
        /// </summary>
        static  void    collectNodes(FENode* node,std::unordered_map<String,FENode*>& nodes)
        {
            if (node == nullptr)
                return;
            if (node->nameIsValid())
                nodes[node->name()] =   node;
            for (auto& child : node->children())
                collectNodes(child.get(),nodes);
        }
        /// <summary>
        /// 是否以 tag 开头且后面是分隔符(避免把 NEWTYPE 之类的键当成 NEW)
        /// </summary>
        static  bool    startsWith(const char* begin,const char* end,const char* tag)
        {
            const   size_t  len =   std::strlen(tag);
            if (size_t(end - begin) < len || std::memcmp(begin,tag,len) != 0)
                return  false;
            return  size_t(end - begin) == len || begin[len] == ' ' || begin[len] == '\t';
        }
        /// <summary>
        /// 在行内查找标记
        /// </summary>
        static  const char* findToken(const char* begin,const char* end,const char* token)
        {
            const   size_t  len =   std::strlen(token);
            for (const char* p = begin; size_t(end - p) >= len; ++p)
            {
                if (std::memcmp(p,token,len) == 0)
                    return  p;
            }
            return  nullptr;
        }
        /// <summary>
        /// 是否是合法 UTF-8(用于区分 UTF-8 与本地 ANSI/GBK 文本)
        /// </summary>
        static  bool    isUtf8(const char* data,size_t size)
        {
            size_t  i   =   0;
            while (i < size)
            {
                const   uint8   c   =   uint8(data[i]);
                if (c < 0x80)
                {
                    ++  i;
                    continue;
                }
                size_t  extra   =   0;
                if ((c & 0xE0) == 0xC0)         extra   =   1;
                else if ((c & 0xF0) == 0xE0)    extra   =   2;
                else if ((c & 0xF8) == 0xF0)    extra   =   3;
                else                            return  false;
                if (i + extra >= size)
                    return  false;
                for (size_t k = 1; k <= extra; ++k)
                {
                    if ((uint8(data[i + k]) & 0xC0) != 0x80)
                        return  false;
                }
                i   +=  extra + 1;
            }
            return  true;
        }
        /// <summary>
        /// 局部文本转 UTF-8:合法 UTF-8 原样返回,否则按 GBK(中文 ANSI)转换
        /// </summary>
        static  String  toUtf8(const char* data,size_t size)
        {
            if (data == nullptr || size == 0)
                return  {};
            bool    ascii   =   true;
            for (size_t i = 0; i < size; ++i)
            {
                if (uint8(data[i]) >= 0x80)
                {
                    ascii   =   false;
                    break;
                }
            }
            if (ascii || isUtf8(data,size))
                return  String(data,size);
#if FE_PLATFORM == FE_PLATFORM_WIN32
            const   int     srcLen  =   int(size);
            UINT    codePage        =   936;    ///< 中文 ANSI
            int     wideLen         =   MultiByteToWideChar(codePage,MB_ERR_INVALID_CHARS,data,srcLen,nullptr,0);
            if (wideLen <= 0)
            {
                /// 非法 GBK 时退回系统 ANSI 代码页
                codePage    =   CP_ACP;
                wideLen     =   MultiByteToWideChar(codePage,0,data,srcLen,nullptr,0);
            }
            if (wideLen > 0)
            {
                std::wstring    wide(size_t(wideLen),L'\0');
                MultiByteToWideChar(codePage,0,data,srcLen,&wide[0],wideLen);
                int         utf8Len =   WideCharToMultiByte(CP_UTF8,0,wide.c_str(),wideLen,nullptr,0,nullptr,nullptr);
                if (utf8Len > 0)
                {
                    String  utf8(size_t(utf8Len),'\0');
                    WideCharToMultiByte(CP_UTF8,0,wide.c_str(),wideLen,&utf8[0],utf8Len,nullptr,nullptr);
                    return  utf8;
                }
            }
#endif
            return  String(data,size);
        }
        /// <summary>
        /// 解析整块文本,逐行处理 NEW/END/键值
        /// </summary>
        void    parse(const uint8* data,uint64 size,Objects& result)
        {
            if (data == nullptr || size == 0)
                return;
            setProgress(real(0));

            const   char*   begin   =   reinterpret_cast<const char*>(data);
            const   char*   end     =   begin + size;
            /// UTF-8 BOM
            if (size >= 3 && uint8(begin[0]) == 0xEF && uint8(begin[1]) == 0xBB && uint8(begin[2]) == 0xBF)
                begin   +=  3;

            Property    current     =   nullptr;
            bool        skipBlock   =   false;      ///< Header Information 不生成属性记录
            const   char* p         =   begin;
            uint32      lines       =   0;
            while (p < end)
            {
                if (isCanceled())
                    break;
                const   char*   lineEnd =   p;
                while (lineEnd < end && *lineEnd != '\n')
                    ++  lineEnd;
                const   char*   lineBegin   =   p;
                const   char*   lineTail    =   lineEnd;
                if (lineTail > lineBegin && lineTail[-1] == '\r')
                    --  lineTail;
                while (lineBegin < lineTail && (*lineBegin == ' ' || *lineBegin == '\t'))
                    ++  lineBegin;
                while (lineTail > lineBegin && (lineTail[-1] == ' ' || lineTail[-1] == '\t'))
                    --  lineTail;

                parseLine(lineBegin,lineTail,current,skipBlock,result);

                p   =   lineEnd < end ? lineEnd + 1 : end;
                if ((++lines & 0x3FF) == 0)
                    setProgress(real(uint64(p - begin)) / real(size));
            }
            setProgress(real(1));
        }
        /// <summary>
        /// 处理一行:NEW 名称 / END / 键:=值
        /// </summary>
        void    parseLine(const char* begin,const char* end,Property& current,bool& skipBlock,Objects& result)
        {
            if (begin >= end)
                return;
            if (startsWith(begin,end,"NEW"))
            {
                const   char*   nameBegin   =   begin + 3;
                while (nameBegin < end && (*nameBegin == ' ' || *nameBegin == '\t'))
                    ++  nameBegin;
                /// 上一条记录缺 END 时直接丢弃,避免属性串到下一条记录
                current     =   nullptr;
                String  name    =   toUtf8(nameBegin,size_t(end - nameBegin));
                if (name == "Header Information")
                {
                    skipBlock   =   true;
                    return;
                }
                skipBlock   =   false;
                if (name.empty())
                    return;
                current     =   new FEProperty(_ctx);
                current->setCode(std::move(name));
                result.emplace_back(current.get());
                return;
            }
            if (startsWith(begin,end,"END"))
            {
                current     =   nullptr;
                skipBlock   =   false;
                return;
            }
            /// OPTION 等控制行不是属性
            if (startsWith(begin,end,"OPTION"))
                return;
            if (skipBlock || current == nullptr)
                return;
            const   char*   sep =   findToken(begin,end,":=");
            if (sep == nullptr)
                return;
            const   char*   keyEnd  =   sep;
            while (keyEnd > begin && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t'))
                --  keyEnd;
            const   char*   valBegin    =   sep + 2;
            while (valBegin < end && (*valBegin == ' ' || *valBegin == '\t'))
                ++  valBegin;
            if (keyEnd == begin)
                return;
            String  key =   toUtf8(begin,size_t(keyEnd - begin));
            String  val =   toUtf8(valBegin,size_t(end - valBegin));
            current->setValue(std::move(key),std::move(val));
        }
    };
}

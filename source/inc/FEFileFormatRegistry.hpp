#pragma     once
#include    <cassert>
#include    <cctype>
#include    "FEContext.hpp"

namespace   FE
{
    /// <summary>
    /// 文件格式注册表:
    ///   1. 数据存放在 FEContext 内,本类只是注册与查询的入口,按需在栈上构造
    ///   2. 扩展名不区分大小写,带不带 '.' 都可以
    ///   3. 用于"这个文件能不能打开"这类判断,避免调用方写死格式列表
    /// </summary>
    class   FE_API  FEFileFormatRegistry
    {
    public:
        using   FormatMap   =   FEKeyValues<String,FEFileFormat>;
    public:
        explicit FEFileFormatRegistry(FEContext& ctx)
            : _ctx(ctx)
        {}
    public:
        /// <summary>
        /// 注册读取器,重复注册在 Debug 下断言
        /// </summary>
        void    addReader(const FEFileFormat& fmt)
        {
            assert(_ctx._readers.isExist(fmt.toString()) == nullptr);
            _ctx._readers.add(fmt.toString(),fmt);
        }
        void    addWriter(const FEFileFormat& fmt)
        {
            assert(_ctx._writers.isExist(fmt.toString()) == nullptr);
            _ctx._writers.add(fmt.toString(),fmt);
        }
        /// <summary>
        /// 按注册键(fmt.toString())查询读取器格式
        /// </summary>
        bool    queryReader(const String& key,FEFileFormat& fmt) const
        {
            const   FEFileFormat*   result  =   _ctx._readers.isExist(key);
            if (result == nullptr)
                return  false;
            fmt =   *result;
            return  true;
        }
        bool    queryWriter(const String& key,FEFileFormat& fmt) const
        {
            const   FEFileFormat*   result  =   _ctx._writers.isExist(key);
            if (result == nullptr)
                return  false;
            fmt =   *result;
            return  true;
        }
        /// <summary>
        /// 已注册的读取器/写入器格式表,例如用于组装打开文件对话框的过滤器
        /// </summary>
        inline  const   FormatMap&  readerFormats() const
        {
            return  _ctx._readers;
        }
        inline  const   FormatMap&  writerFormats() const
        {
            return  _ctx._writers;
        }
        /// <summary>
        /// 是否注册了可读取该扩展名的读取器
        /// </summary>
        inline  bool    supportReader(const String& ext) const
        {
            return  reader(ext) != nullptr;
        }
        /// <summary>
        /// 是否注册了可写出该扩展名的写入器
        /// </summary>
        inline  bool    supportWriter(const String& ext) const
        {
            return  writer(ext) != nullptr;
        }
        /// <summary>
        /// 按扩展名取读取器格式,未注册返回 nullptr
        /// </summary>
        inline  const   FEFileFormat*  reader(const String& ext) const
        {
            return  find(_ctx._readers,ext);
        }
        /// <summary>
        /// 按扩展名取写入器格式,未注册返回 nullptr
        /// </summary>
        inline  const   FEFileFormat*  writer(const String& ext) const
        {
            return  find(_ctx._writers,ext);
        }
        /// <summary>
        /// 已注册的可读扩展名,例如 {".fepj",".gltf",".glb"}
        /// </summary>
        inline  Strings readerExtensions() const
        {
            return  extensions(_ctx._readers);
        }
        inline  Strings writerExtensions() const
        {
            return  extensions(_ctx._writers);
        }
        /// <summary>
        /// 已注册的可读扩展名拼成的提示文本,例如 ".fepj .gltf .glb"
        /// </summary>
        inline  String  readerExtensionsText(const String& separator = " ") const
        {
            return  join(readerExtensions(),separator);
        }
        inline  String  writerExtensionsText(const String& separator = " ") const
        {
            return  join(writerExtensions(),separator);
        }
    protected:
        /// <summary>
        /// 扩展名归一化:去掉空白,补 '.' 前缀
        /// </summary>
        static  String  normalize(const String& ext)
        {
            String  target;
            for (char c : ext)
            {
                if (!isspace((unsigned char)c))
                    target.push_back(c);
            }
            if (!target.empty() && target.front() != '.')
                target.insert(target.begin(),'.');
            return  target;
        }
        static  const   FEFileFormat*  find(const FormatMap& store,const String& ext)
        {
            const   String  target  =   normalize(ext);
            if (target.empty())
                return  nullptr;
            for (const auto& it : store.data())
            {
                if (_stricmp(it.second.ext(),target.c_str()) == 0)
                    return  &it.second;
            }
            return  nullptr;
        }
        static  Strings extensions(const FormatMap& store)
        {
            Strings result;
            for (const auto& it : store.data())
                result.push_back(it.second.ext());
            return  result;
        }
        static  String  join(const Strings& values,const String& separator)
        {
            String  text;
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (i != 0)
                    text  +=  separator;
                text  +=  values[i];
            }
            return  text;
        }
    protected:
        FEContext&  _ctx;
    };
}

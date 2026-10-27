
#include    <algorithm>
#include    <cctype>
#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <iostream>
#include    <set>
#include    <string>
#include    <vector>
#include    <filesystem>

/// std::codecvt / std::wstring_convert 在 C++17 中已弃用，
/// 本工具仅用它们做 GBK 与 UTF-8 互转，这里屏蔽 MSVC 的弃用提醒
#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#include    <codecvt>

namespace   fs  =   std::filesystem;
#include    "../../inc/FEMd5Key.hpp"
#include    "../../inc/FEXML.hpp"
#include    "../../inc/FEDateTime.hpp"
#include    "../../inc/FEConst.h"
#include    "cmdline.h"

using   namespace   FE;

using   WString     =   std::wstring;
using   String      =   std::string;
using   ArrayString =   std::vector<std::string>;
/// <summary>
/// 文件内容缓冲区，末尾额外保留一个 '\0'，便于按字符串解析
/// </summary>
using   FileBuffer  =   std::vector<char>;

const char g_map[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

/// <summary>
/// 将名称中不合法的 C++ 标识符字符替换为合法字符
/// </summary>
/// <param name="str">待处理的名称</param>
/// <returns>处理后的名称</returns>
static  String& replaceDot(String& str)
{
    const   size_t  nMap    =   sizeof(g_map) - 1;
    for (char& ch : str)
    {
        const   unsigned char   value   =   static_cast<unsigned char>(ch);
        if (   (value >= 'A' && value <= 'Z')
            || (value >= 'a' && value <= 'z')
            || (value >= '0' && value <= '9')
            || value == '_')
            continue;
        ch  =   g_map[value % nMap];
    }
    /// 标识符不能以数字开头
    if (!str.empty() && str[0] >= '0' && str[0] <= '9')
        str.insert(str.begin(),'_');
    return  str;
}

/// <summary>
/// 将路径中的反斜杠统一替换为正斜杠
/// </summary>
/// <param name="str">待处理的路径</param>
/// <returns>处理后的路径</returns>
static  String& replacePath(String& str)
{
    for (char& ch : str)
    {
        if (ch == '\\')
            ch = '/';
    }
    return  str;
}

/// <summary>
/// 去掉字符串首尾的空白字符
/// </summary>
/// <param name="str">待处理的字符串</param>
/// <returns>去掉首尾空白后的字符串</returns>
static  String  trim(const String& str)
{
    size_t  nBegin  =   0;
    size_t  nEnd    =   str.size();
    while (nBegin < nEnd && std::isspace(static_cast<unsigned char>(str[nBegin])))
        ++nBegin;
    while (nEnd > nBegin && std::isspace(static_cast<unsigned char>(str[nEnd - 1])))
        --nEnd;
    return  str.substr(nBegin,nEnd - nBegin);
}

/// <summary>
/// 归一化资源路径:去掉首尾空白、反斜杠转正斜杠、合并重复斜杠、去掉尾部斜杠
/// </summary>
/// <param name="path">待处理的路径</param>
/// <returns>归一化后的路径</returns>
static  String  normalizePath(const String& path)
{
    String  result;
    result.reserve(path.size());
    for (char ch : trim(path))
    {
        if (ch == '\\')
            ch = '/';
        if (ch == '/' && !result.empty() && result.back() == '/')
            continue;
        result.push_back(ch);
    }
    while (result.size() > 1 && result.back() == '/')
        result.pop_back();
    return  result;
}

/// <summary>
/// 生成 .qrc 资源 key，规范为 :/prefix/name
/// </summary>
/// <param name="prefix">qresource 的 prefix 属性</param>
/// <param name="name">file 的 alias 或相对路径</param>
/// <returns>资源 key，例如 :/EditorRes/res/ico_Add.png</returns>
static  String  qrcKey(const String& prefix,const String& name)
{
    String  folder  =   normalizePath(prefix);
    String  leaf    =   normalizePath(name);
    /// 去掉 alias/path 的前导 "./" 与前导 '/',它们相对于 prefix
    while (leaf.size() >= 2 && leaf[0] == '.' && leaf[1] == '/')
        leaf.erase(0,2);
    while (!leaf.empty() && leaf[0] == '/')
        leaf.erase(0,1);
    if (folder.empty() || folder == "/")
        return  ":/" + leaf;
    if (folder[0] != '/')
        folder.insert(folder.begin(),'/');
    return  ":/" + folder.substr(1) + "/" + leaf;
}

/// <summary>
/// 获取相对于给定跟目录的相对目录，即 rootPath - path
/// </summary>
/// <param name="rootPath">根目录</param>
/// <param name="path">完整路径</param>
/// <returns>相对路径；不匹配时返回空字符串</returns>
const char* getRelPath(const std::string& rootPath,const char* path)
{
    if (path == nullptr || rootPath.empty())
        return  "";

    size_t  nLen    =   strlen(path);
    if (nLen < rootPath.size())
        return  "";
    for (size_t i = 0; i < rootPath.size(); i++)
    {
        const   unsigned char   ch1 =   static_cast<unsigned char>(path[i]);
        const   unsigned char   ch2 =   static_cast<unsigned char>(rootPath[i]);
        if (std::toupper(ch1) == std::toupper(ch2))
            continue;
        if (   (ch1 == '/' || ch1 == '\\')
            && (ch2 == '/' || ch2 == '\\'))
            continue;
        return  "";
    }
    /// 前缀必须落在目录分隔处，避免 d:/work 误匹配 d:/work2
    const char* relName =   path + rootPath.size();
    if (*relName != '\0' && *relName != '/' && *relName != '\\')
        return  "";
    while (*relName == '/' || *relName == '\\')
        ++relName;
    return  relName;
}


static  String  fileExtension(const char* fileName)
{
    if (fileName == nullptr || *fileName == '\0')
        return  "";
    if (*fileName == '.')
        return  fileName;
    fs::path filePath(fileName);
    return  filePath.extension().string();
}
static  String  getFileName(const String& fullPath)
{
    fs::path filePath(fullPath);
    return  filePath.filename().string();
}

static  String  filePath(const String& fileName)
{
    fs::path filePath(fileName);
    return  filePath.parent_path().string();
}

/// <summary>
/// 忽略大小写比较两个字符串
/// </summary>
/// <param name="left">左字符串</param>
/// <param name="right">右字符串</param>
/// <returns>相等返回 true</returns>
static  bool    equalIgnoreCase(const char* left,const char* right)
{
    if (left == nullptr || right == nullptr)
        return  left == right;
    for ( ; *left != '\0' && *right != '\0'; ++left,++right)
    {
        const   unsigned char   ch1 =   static_cast<unsigned char>(*left);
        const   unsigned char   ch2 =   static_cast<unsigned char>(*right);
        if (std::toupper(ch1) != std::toupper(ch2))
            return  false;
    }
    return  *left == *right;
}

static  bool    makeDir(const String& dir)
{
    if (dir.empty())
        return  false;
    try
    {
        /// 目录不存在则逐级创建；已存在返回 false
        return  fs::create_directories(dir);
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << e.what() << std::endl;
        return  false;
    }
}

// linux下为"zh_CN.GBK"
#ifdef WIN32
    #define GBK_NAME ".936"
#else
    #define GBK_NAME "zh_CN.GBK"
#endif

class   WCHAR_GBK : public std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
public:
    WCHAR_GBK(const char* name) : codecvt_byname(name){}
};

inline  String  utf8_2_gbk(const char* utf8)
{
    try
    {
        using   WCHAR_UTF8  =   std::codecvt_utf8<wchar_t>;
        /// gbk与unicode之间的转换器
        std::wstring_convert<WCHAR_GBK>  cvtGBK(new WCHAR_GBK(GBK_NAME));
        /// utf8与unicode之间的转换器
        std::wstring_convert<WCHAR_UTF8> cvtUTF8;
        /// 从utf8转换为unicode
        WString    ustr = cvtUTF8.from_bytes(utf8);
        /// 从unicode转换为gbk
        String     str = cvtGBK.to_bytes(ustr);
        return str;
    }
    catch (const std::exception& exc)
    {
        /// linux 下如果报该异常，则需要安装 zh_CN.GBK
        printf("异常(%s)，则需要安装 zh_CN.GBK\n",exc.what());
    }
    return utf8;
}

inline  String  gbk_2_utf8(const char* gbk)
{
    using   WCHAR_UTF8  =   std::codecvt_utf8<wchar_t>;
    try
    {
        /// gbk与unicode之间的转换器
        std::wstring_convert<WCHAR_GBK>  cvtGBK(new WCHAR_GBK(GBK_NAME));
        /// utf8与unicode之间的转换器
        std::wstring_convert<WCHAR_UTF8> cvtUTF8;
        /// 从utf8转换为unicode
        WString    ustr = cvtGBK.from_bytes(gbk);
        /// 从unicode转换为gbk
        String     str = cvtUTF8.to_bytes(ustr);
        return  str;
    }
    catch (const std::exception& exc)
    {
        /// linux 下如果报该异常，则需要安装 zh_CN.GBK
        printf("异常(%s)，则需要安装 zh_CN.GBK\n",exc.what());
        return gbk;
    }
}

/// <summary>
/// 读取文件全部内容到缓冲区，并在末尾补 '\0'
/// </summary>
/// <param name="fileName">文件路径</param>
/// <param name="buffer">输出的文件内容</param>
/// <returns>成功返回 true</returns>
static  bool    readFile(const char* fileName,FileBuffer& buffer)
{
    if (fileName == nullptr)
        return  false;

    FILE*   pFile   =   fopen(fileName,"rb");
    if (pFile == nullptr)
    {
        /// 兼容路径编码差异:先尝试 UTF-8 -> GBK，再尝试 GBK -> UTF-8
        String  gbkFile =   utf8_2_gbk(fileName);
        pFile   =   fopen(gbkFile.c_str(),"rb");
    }
    if (pFile == nullptr)
    {
        String  utfFile =   gbk_2_utf8(fileName);
        pFile   =   fopen(utfFile.c_str(),"rb");
    }
    if (pFile == nullptr)
    {
        printf("open file(%s) error !\n",fileName);
        return  false;
    }

    fseek(pFile,0,SEEK_END);
    long    nLen    =   ftell(pFile);
    if (nLen < 0)
    {
        fclose(pFile);
        printf("get file(%s) size error !\n",fileName);
        return  false;
    }
    fseek(pFile,0,SEEK_SET);
    buffer.resize(size_t(nLen) + 1);
    if (nLen > 0)
    {
        size_t  nRead   =   fread(buffer.data(),1,size_t(nLen),pFile);
        if (nRead != size_t(nLen))
        {
            fclose(pFile);
            printf("read file(%s) error !\n",fileName);
            return  false;
        }
    }
    fclose(pFile);
    buffer[size_t(nLen)]    =   '\0';
    return  true;
}

/// <summary>
/// 获取名称
/// </summary>
/// <param name="workPath">工作目录，用于提取资源相对名称</param>
/// <param name="rootPath">当前遍历的文件或目录</param>
/// <param name="doc">xml doc 对象</param>
/// <param name="parent">根节点</param>
void    findFileToXML(const char* workPath,const char* rootPath,XMLDoc& doc,XMLNode* parent)
{
    String      path    =   rootPath;
    if (path.empty() || !fs::exists(path))
        return;

    if(fs::is_directory(path))
    {
        XMLNode*    nodePath        =   doc.allocate_node(rapidxml::node_element, "folder", 0);
        String      name            =   fs::path(path).stem().string();
        XMLAttr*    aPath           =   doc.allocate_attribute("path", doc.allocate_string(name.c_str()));
        nodePath->append_attribute(aPath);
        parent->append_node(nodePath);

        /// 目录内容排序后再遍历，保证生成的资源文件稳定可复现
        ArrayString children;
        for (const auto& entry : fs::directory_iterator(path))
            children.push_back(entry.path().generic_string());
        std::sort(children.begin(),children.end());
        for (const auto& file : children)
        {
            findFileToXML(workPath,file.c_str(),doc,nodePath);
        }
    }
    else
    {
        String  ext =   fileExtension(rootPath);
        if (   equalIgnoreCase(ext.c_str(),".fers")
            || equalIgnoreCase(ext.c_str(),".prrs")
            || equalIgnoreCase(ext.c_str(),".cpp"))
        {
            return  ;
        }
        XMLNode*    nodeFile    =   doc.allocate_node(rapidxml::node_element,"file", 0);
        String      pName       =   getFileName(rootPath);
        XMLAttr*    aFileName   =   doc.allocate_attribute("input",doc.allocate_string(pName.c_str()));

        String      relPath     =   getRelPath(workPath,rootPath);
        if (relPath.empty())
            relPath =   rootPath;
        XMLAttr*    aName       =   doc.allocate_attribute("name",doc.allocate_string(relPath.c_str()));
        nodeFile->append_attribute(aName);
        nodeFile->append_attribute(aFileName);
        parent->append_node(nodeFile);
    }
}

struct  Header
{
    std::string     _srcFileName;
    std::string     _declareDataLenVar;
    std::string     _declareDataVar;

    std::string     _declareDataName;
    std::string     _declareDataLenName;
};

using   Headers =   std::vector<Header>;

void    outFileBody(const char* className,FILE* file,const Headers& headers)
{
    if (file == nullptr)
        return;

    const   String  clsName =   std::string("CLASS_") + className;

    std::string     res;
    res +=  "\n";
    res +=  "/// helper function !\n";
    res +=  "/// \n";
    res +=  "/// comments\n";
    res +=  "namespace FE\n";
    res +=  "{\n";
    res +=  "   class   " + clsName + "\n";
    res +=  "   {\n";
    res +=  "   public:\n";
    res +=  "       " + clsName + "()\n";
    res +=  "       {\n";
    for (const auto& h : headers)
    {
        res +=  "            FE::FEAssetsMgr::instance().addBuffer(\"";
        res +=  h._srcFileName;
        res +=  "\",";
        res +=  h._declareDataName;
        res +=  ",";
        res +=  h._declareDataLenName;
        res +=  ",false);\n\n";
    }
    res +=  "       }\n";
    res +=  "   };\n";
    res +=  "   static  "   +   clsName + "    _" + className + ";\n";
    res +=  "}\n";
    fputs(res.c_str(),file);
}
void    outFileHeader(FILE* file)
{
    if (file == nullptr)
        return;

    FEDateTime      dateTime;
    char            szTimeFormat[256] = { 0 };
    dateTime.toDateTimeString(szTimeFormat);
    fputs("/// tool.resource.cc Compiler V1.0.0.0\n", file);
    fputs("/// tool.resource.cc Copyright 2018-2026 FE Inc All rights reserved !\n", file);
    fputs("/// Warning! All changes made in this file will be lost!\n", file);
    fputs("/// Build Time ", file);
    fputs(szTimeFormat, file);

    fputs("\n\n", file);
    fputs("#include \"../inc/FEAssetsMgr.h\"\n", file);
#ifdef _MSC_VER
    fputs("#pragma warning(push)\n", file);
    fputs("#pragma warning(disable:4828)\n", file);
#endif


    fputs("\n\n", file);
    
}

void    outFileTrail(FILE* file)
{
    if (file == nullptr)
        return;
#ifdef _MSC_VER
    fputs("#pragma warning(pop)\n", file);
#endif
}

/// <summary>
/// 编译单个资源文件，生成字节数组源码并登记到 header
/// </summary>
/// <param name="srcFile">源文件路径</param>
/// <param name="dstFile">输出文件路径，仅 pDstFile 为空时使用</param>
/// <param name="dstName">数据变量名，会自动转换为合法标识符</param>
/// <param name="sName">资源名称，注册到 FEAssetsMgr 的 key</param>
/// <param name="header">输出的登记信息</param>
/// <param name="pDstFile">汇总输出文件，为空时单独输出到 dstFile</param>
/// <returns>成功返回 true</returns>
bool    compile( const char* srcFile
                ,const char* dstFile
                ,const char* dstName
                ,const char* sName
                ,Header&     header
                ,FILE*       pDstFile = nullptr)
{
    FileBuffer  buffer;
    if (!readFile(srcFile,buffer))
        return  false;
    printf("start compile file(%s) !\n",srcFile);

    const   size_t  nLen    =   buffer.size() - 1;

    String  legalName   =   dstName;
    replaceDot(legalName);

    const   String  outDataName     =   std::string("_") + legalName;
    const   String  outDataLenName  =   outDataName + "_len";

    std::string     result;
    /// 每个字节输出 "0x00," 共 5 个字符，另预留换行与缩进
    result.reserve(nLen * 6 + 1024);
    result  +=  "namespace  FE\n";
    result  +=  "{\n";
    result  +=  "   unsigned int    " + outDataLenName + "      =   " + std::to_string(nLen) + ";\n";
    result  +=  "   unsigned char   " + outDataName + "[]    =   \n";
    result  +=  "   { \n";
    result  +=  "       ";
    for (size_t i = 0 ;i < nLen + 1; ++ i)
    {
        char            szBuf[8]    =   {0};
        const unsigned  value       =   static_cast<unsigned char>(buffer[i]);
        snprintf(szBuf,sizeof(szBuf),"0x%02x,",value);
        if (i && i%64 == 0)
        {
            result  +=  "\n       ";
        }
        result  +=  szBuf;
    }
    result  +=  "\n";
    result  +=  "   };\n";
    result  +=  "};\n";

    header._declareDataLenVar   =   "   unsigned int    " + outDataLenName + ";";
    header._declareDataVar      =   "   unsigned char   " + outDataName + "[];";
    header._declareDataName     =   outDataName;
    header._declareDataLenName  =   outDataLenName;
    header._srcFileName         =   sName;

    /// 如果有输入文件，则使用
    FILE*   pOutFile    =   pDstFile;
    const   bool    bOwnFile    =   (pOutFile == nullptr);
    if (bOwnFile)
    {
        String  outFileName =   String(srcFile) + std::string(".cpp");
        if (dstFile != nullptr && strlen(dstFile) >= 3)
            outFileName =   dstFile;
        pOutFile    =   fopen(outFileName.c_str(), "wt+");
        if (pOutFile == nullptr)
        {
            printf("open file(%s) error !\n",outFileName.c_str());
            return  false;
        }
        outFileHeader(pOutFile);
    }
    fwrite(result.c_str(),1,result.size(),pOutFile);
    /// pDstFile == nullptr 则说明 pOutFile 是创建的，不是自建的
    /// 需要关闭，否则不关闭
    if (bOwnFile)
    {
        Headers     headers;
        headers.push_back(header);
        outFileBody(legalName.c_str(),pOutFile,headers);
        outFileTrail(pOutFile);
        fclose(pOutFile);
    }
    printf("end compile file(%s) !\n",srcFile);
    return  true;
}


std::string     toClassBody(const Headers& headers)
{
    size_t          index   =   0;
    std::string     clsStr  =   "";
    clsStr += "#include \"../inc/FEAssetsMgr.h\"\n";
    clsStr += "namespace FE\n";
    clsStr += "{\n";
    for (size_t i = 0; i < headers.size(); ++i)
    {
        clsStr += "    extern  ";
        clsStr += headers[i]._declareDataLenVar;
        clsStr += "\n";
        clsStr += "    extern  ";
        clsStr += headers[i]._declareDataVar;
        clsStr += "\n";
        clsStr += "\n";
    }
 
    clsStr += "    FEAssetsMgr::FEAssetsMgr()\n";
    clsStr += "    {\n";
    for (auto itr = headers.begin(); itr != headers.end(); ++itr)
    {
        if (index == 0)
        {
            clsStr += "        Resource  res;\n";
            ++index;
        }
        clsStr += "        res._data                =   ";
        clsStr += itr->_declareDataName;
        clsStr += ";\n";
        clsStr += "        res._dataLen             =   ";
        clsStr += itr->_declareDataLenName;
        clsStr += ";\n";
        clsStr += "        res._name                =   ";
        clsStr += "\"";
        clsStr += itr->_srcFileName;
        clsStr += "\";";
        clsStr += "\n";
        clsStr += "        _resource[res._name]     =   res;\n";
        clsStr += "\n";
    }
    clsStr += "    }\n";
    clsStr += "    FEAssetsMgr&    FEAssetsMgr::instance()\n";
    clsStr += "    {\n";
    clsStr += "         static   FEAssetsMgr _resource;\n";
    clsStr += "         return   _resource;\n";
    clsStr += "     }\n";
    clsStr += "     const   FEAssetsMgr::Resource*  FEAssetsMgr::getResource(const char* fileName)\n";
    clsStr += "     {\n";
    clsStr += "         auto    itr = _resource.find(fileName);\n";
    clsStr += "         if (itr != _resource.end())\n";
    clsStr += "             return  &itr->second;\n";
    clsStr += "         else\n";
    clsStr += "             return  nullptr;\n";
    clsStr += "       }\n";
    clsStr += "};\n";
    return  clsStr;

}

std::string     toClass()
{
    std::string     clsStr  =   "";
    clsStr  +=  "#include   <map>\n";
    clsStr  +=  "#include   <string>\n";
    clsStr  +=  "namespace  FE\n";
    clsStr  +=  "{\n";
    clsStr  +=  "   class   FEAssetsMgr\n";
    clsStr  +=  "   {\n";
    clsStr  +=  "   public:\n";
    clsStr  +=  "       struct  Resource\n";
    clsStr  +=  "       {\n";
    clsStr  +=  "           size_t      _dataLen;\n";
    clsStr  +=  "           const void* _data;\n";
    clsStr  +=  "           std::string _name;\n";
    clsStr  +=  "       };\n";
    clsStr  +=  "       using   MapResource = std::map<std::string, Resource>;\n";
    clsStr  +=  "   protected:\n";
    clsStr  +=  "       MapResource _resource;\n";
    clsStr  +=  "   public:\n";
    clsStr  +=  "       static   FEAssetsMgr&    instance();\n";
    clsStr  +=  "   public:\n";
    clsStr  +=  "       FEAssetsMgr();\n\n";  
    clsStr  +=  "       /// query interface for resource \n";
    clsStr  +=  "       const   Resource*   getResource(const char* fileName);\n";
    clsStr  +=  "   };\n";
    clsStr  +=  "};\n";
    return  clsStr;
}

void    procNode(
                XMLNode*            node
                ,const std::string& rootDir
                ,const std::string& workPath
                ,const std::string& resFile
                ,Headers&           headers
                ,FILE*              pOutFile)
{
    XMLNode* child  =   node->first_node();
    if (child)
    {
        /// path 属性属于当前节点，循环外只取一次
        XMLAttr*    pPath   =   node->first_attribute("path");
        std::string path    =   workPath;
        if (pPath)
            path    +=  std::string("/") + pPath->value();

        for ( ; child ; child = child->next_sibling())
        {
            procNode(child, rootDir,path, resFile, headers, pOutFile);
        }
    }
    else
    {
        XMLAttr*    aInput  =   node->first_attribute("input");
        if (aInput == nullptr)
            return;
        const   std::string srcFile =   workPath + "/" + aInput->value();
        const   std::string sName   =   (srcFile.size() > rootDir.size() + 1)
                                        ? srcFile.substr(rootDir.size() + 1)
                                        : getFileName(srcFile);

        std::string dstNames    =   srcFile;

        replacePath(dstNames);
        replaceDot(dstNames);

        Header      header;
        if (pOutFile)
        {
            fputs("\n\n", pOutFile);
            fputs("/// ", pOutFile);
            fputs(srcFile.c_str(), pOutFile);
            fputs("\n", pOutFile);

            if(compile( srcFile.c_str()
                    ,resFile.c_str()
                    ,dstNames.c_str()
                    ,sName.c_str()
                    ,header
                    ,pOutFile
                ))
            {
                headers.push_back(header);
            }
        }
        else
        {
            const   std::string     dstFile =   srcFile + std::string(".cpp");
            if(compile( srcFile.c_str()
                    ,dstFile.c_str()
                    ,dstNames.c_str()
                    ,sName.c_str()
                    ,header
                    ,nullptr
                    ))
            {
                headers.push_back(header);
            }
        }
    }
}

bool    processXML(
     const char* workPath
    ,const char* ccPath
    ,const char* srcFile
    ,const char* outFile
    ,Headers& headers)
{
    FileBuffer  buffer;
    if (!readFile(srcFile,buffer))
        return  false;

    std::string     className   =   getFileName(srcFile);
    replaceDot(className);

    FILE*       pOutFile    =   nullptr;
    try
    {
        XMLDoc      doc;
        doc.parse<0>(buffer.data());
        XMLNode*    rootNode =   doc.first_node();
        if (rootNode == 0)
        {
            printf("parse file(%s) failed!\n",srcFile);
            return false;
        }
        /// $Work,$This
        XMLAttr*        pDataPath   =   rootNode->first_attribute("dataPath");
        XMLAttr*        pSingleOut  =   rootNode->first_attribute("singleOut");

        std::string     dataPath;
        if (pDataPath == 0 || strcmp(pDataPath->value(),"$Work") == 0)
            dataPath    =   workPath;
        else if (strcmp(pDataPath->value(),"$This") == 0)
            dataPath    =   ccPath;
        else
            dataPath    =   pDataPath->value();

        if (pSingleOut && atoi(pSingleOut->value()) != 0)
        {
            pOutFile    =   fopen(outFile,"w+");
            if (pOutFile == nullptr)
            {
                printf("open file(%s) error !\n",outFile);
                return  false;
            }
            outFileHeader(pOutFile);
        }

        XMLNode*    nodeFile    =   rootNode->first_node();

        for ( ; nodeFile; nodeFile = nodeFile->next_sibling())
        {
            procNode(nodeFile,dataPath,dataPath,outFile,headers,pOutFile);
        }
        if (pOutFile)
        {
            outFileBody(className.c_str(),pOutFile,headers);
            outFileTrail(pOutFile);
            fclose(pOutFile);
            pOutFile    =   nullptr;
        }

        printf("compile finished !\n");
    }
    catch (const std::exception& exc)
    {
        printf("XML Parse Exception(%s): %s\n",srcFile,exc.what());
        if (pOutFile)
            fclose(pOutFile);
        return  false;
    }
    return  true;
}


bool    processQRC(
     const char* workPath
    ,const char* ccPath
    ,const char* srcFileName
    ,const char* dstFile
    ,Headers& headers)
{
    (void)ccPath;
    (void)workPath;

    FileBuffer  buffer;
    if (!readFile(srcFileName,buffer))
        return  false;

    std::string     className   =   srcFileName;
    replaceDot(className);

    /// 已注册的资源 key,用于检测重复(重复会导致同名符号与重复注册)
    std::set<std::string>   usedKeys;
    bool    bSuccess    =   true;

    FILE*       pOutFile    =   nullptr;
    try
    {
        XMLDoc      doc;
        doc.parse<0>(buffer.data());
        XMLNode*    rootNode =   doc.first_node();
        if (rootNode == 0)
        {
            printf("parse file(%s) failed!\n",srcFileName);
            return false;
        }
        if (strcmp(rootNode->name(),"RCC") != 0)
        {
            printf("parse file(%s) failed! root element must be <RCC>, actual is <%s>\n",
                    srcFileName,rootNode->name());
            return  false;
        }
        std::string     path        =   filePath(srcFileName);

        pOutFile    =   fopen(dstFile,"w+");
        if (pOutFile == nullptr)
        {
            printf("open file(%s) error !\n",dstFile);
            return  false;
        }
        outFileHeader(pOutFile);

        for (XMLNode* nodeQRC  =   rootNode->first_node("qresource")
                    ; nodeQRC  ; nodeQRC  =   nodeQRC->next_sibling("qresource"))
        {
            /// prefix 缺省为 "/"
            XMLAttr*    aPrefix =   nodeQRC->first_attribute("prefix");
            std::string prefix  =   aPrefix ? aPrefix->value() : "/";
            /// qresource 级别的语言
            XMLAttr*    aRCLang =   nodeQRC->first_attribute("lang");
            std::string rcLang  =   aRCLang ? trim(aRCLang->value()) : "";

            for (XMLNode* nodeFile  =   nodeQRC->first_node("file")
                        ; nodeFile  ; nodeFile  =   nodeFile->next_sibling("file"))
            {
                /// <file> 的文本内容就是磁盘相对路径(可能跨行,需 trim)
                std::string fileText    =   trim(nodeFile->value());
                if (fileText.empty())
                {
                    printf("qrc file(%s): <file> without path, skipped!\n",srcFileName);
                    bSuccess    =   false;
                    continue;
                }
                /// alias 优先作为资源名
                XMLAttr*    aAlias  =   nodeFile->first_attribute("alias");
                std::string alias   =   aAlias ? trim(aAlias->value()) : "";
                /// file 级别的 lang 优先于 qresource 级别
                XMLAttr*    aFileLang =   nodeFile->first_attribute("lang");
                std::string lang    =   aFileLang ? trim(aFileLang->value()) : rcLang;

                std::string key     =   qrcKey(prefix, alias.empty() ? fileText : alias);
                /// <summary>
                /// FEAssetsMgr 以字符串为唯一 key,无法表达同一资源的多语言版本,
                /// 这里在 key 末尾追加 "@lang" 加以区分
                /// </summary>
                if (!lang.empty())
                    key += "@" + lang;

                if (usedKeys.count(key) > 0)
                {
                    printf("qrc file(%s): duplicate resource key(%s), skipped!\n",srcFileName,key.c_str());
                    bSuccess    =   false;
                    continue;
                }
                usedKeys.insert(key);

                std::string srcFile =   path + "/" + fileText;

                replacePath(srcFile);

                /// 符号名基于完整 key,保证不同 prefix/目录下的同名文件不冲突
                std::string dstNames =   key + std::string(".cpp");
                replacePath(dstNames);
                replaceDot(dstNames);

                fputs("\n\n", pOutFile);
                fputs("/// ", pOutFile);
                fputs(srcFile.c_str(), pOutFile);
                fputs("\n", pOutFile);

                Header      header;
                if(compile(srcFile.c_str(),dstFile,dstNames.c_str(),key.c_str(),header,pOutFile))
                {
                    headers.push_back(header);
                }
                else
                {
                    bSuccess    =   false;
                }
            }
        }

        outFileBody(className.c_str(),pOutFile,headers);
        outFileTrail(pOutFile);
        fclose(pOutFile);
        pOutFile    =   nullptr;
        printf("compile finished !\n");
    }
    catch (const std::exception& exc)
    {
        printf("XML Parse Exception(%s): %s\n",srcFileName,exc.what());
        if (pOutFile)
            fclose(pOutFile);
        return  false;
    }
    return  bSuccess;
}

/// <summary>
/// 程序入口
/// </summary>
/// <param name="argc">参数个数</param>
/// <param name="argv">参数列表</param>
/// <returns>0 成功，非 0 失败</returns>
int     main(int argc,char** argv)
{
    CMDLine::Parser a;
    /// 工作目录，用来提取资源名称使用
    /// 例如工作目录是: c:/xx/yy/zz/ww,如果 文件如果是 c:/xx/yy/zz/ww/xyz/tt.xx
    /// 提取结果是: xyz/tt.xx
    a.add<std::string>("work",  'w',    "work dir",         true,       "");
    /// 模式: 文件夹，fers文件,qrc
    a.add<std::string>("mode",  'm',    "dir,fers,qrc",     true,       "");
    /// 输入，文件夹，或者文件
    a.add<std::string>("in",    'i',    "src file",         true,       "");
    /// 输出fers资源文件(xml)格式描述,如果不指定则使用默认值
    a.add<std::string>("fers",  'f',    "fers file",        false,      "");
    /// 输出,cpp文件
    a.add<std::string>("out",   'o',    "cpp file",         false,      "");
    
    bool ok = a.parse(argc, argv);

    if (!ok)
    {
        std::cerr << a.error_full() << a.usage();
        return 1;
    }

    /// 1:是工作目录，重要，定位资源的位置
    /// 2:输入文件信息
    /// 3 -out
    /// 4 输出的文件
    
    Headers         headers;
    /// 编译器的路径
    std::string     ccPath  =   (argc > 0 && argv[0] != nullptr) ? argv[0] : "";
                    ccPath  =   filePath(ccPath);
    
    /// 工作目录
    std::string     workPath    =   a.get<std::string>("work");
    std::string     packPath    =   a.get<std::string>("in");
    std::string     mode        =   a.get<std::string>("mode");
    std::string     fersFile    =   a.get<std::string>("fers");
    std::string     outDefine   =   a.get<std::string>("out");
    
    std::string     srcFile     =   packPath;
    std::string     outFile     =   outDefine;

    if (fersFile.empty())
        fersFile =   srcFile + ".fers";

    if (outFile.empty())
        outFile =   fersFile + ".cpp";

    replacePath(ccPath);
    replacePath(workPath);
    replacePath(packPath);
    replacePath(srcFile);
    replacePath(outFile);

    /// 文件夹
    if (mode == "dir")
    {
        /// 遍历文件生成资源文件
        printf("Gen Resource File\n");
        XMLDoc      doc;
        XMLNode*    xmlinfo =   doc.allocate_node(rapidxml::node_pi, "xml version='1.0' encoding='gb2312'");

        XMLNode*    root    =   doc.allocate_node(rapidxml::node_element, "root", 0);
      
        doc.append_node(xmlinfo);
        doc.append_node(root);
        
        XMLAttr*    aDataPath   =   doc.allocate_attribute("dataPath",  doc.allocate_string(workPath.c_str()));
        XMLAttr*    aOutPath    =   doc.allocate_attribute("outPath",   "");
        XMLAttr*    aSingleOut  =   doc.allocate_attribute("singleOut", "1");

        root->append_attribute(aDataPath);
        root->append_attribute(aOutPath);
        root->append_attribute(aSingleOut);

        findFileToXML(workPath.c_str(),packPath.c_str(),doc,root);

        std::string xmlResult;
        rapidxml::print(std::back_inserter(xmlResult), doc);
        printf("Save... %s \n",fersFile.c_str());

        
        makeDir(filePath(fersFile));

        FILE*       pFile       =   fopen(fersFile.c_str(),"w+");
        if (pFile == nullptr)
        {
            printf("Save %s error !\n",fersFile.c_str());
            return 1;
        }
        fputs(xmlResult.c_str(),pFile);
        fclose(pFile);

        if (!outDefine.empty())
        {
            printf("parse %s \n",fersFile.c_str());
            if (!processXML(workPath.c_str(), ccPath.c_str(), fersFile.c_str(), outFile.c_str(),headers))
                return 1;
        }
        return  0;
    }

    if (mode =="qrc")
    {
        printf("parse %s \n",srcFile.c_str());
        return  processQRC(workPath.c_str(), ccPath.c_str(), srcFile.c_str(), outFile.c_str(),headers) ? 0 : 1;
    }

    if (mode =="fers")
    {
        printf("parse %s \n",srcFile.c_str());
        return  processXML(workPath.c_str(), ccPath.c_str(), srcFile.c_str(), outFile.c_str(),headers) ? 0 : 1;
    }

    printf("unknown mode(%s), support: dir,fers,qrc\n",mode.c_str());
    std::cerr << a.usage();
    return  1;
}

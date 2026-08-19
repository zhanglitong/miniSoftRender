
#include    <iostream>
#include    <vector>
#include    <map>
#include    <filesystem>
#include    <codecvt>

namespace   fs  =   std::filesystem;
#include    "../../inc/FEMd5Key.hpp"
#include    "../../inc/FEXML.hpp"
#include    "../../inc/FEDateTime.hpp"
#include    "../../inc/FEConst.h"
#include    "cmdline.h"

using   namespace   FE;

using   ArrayString =   std::vector<std::string>;

const char g_map[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
char*   replaceDot(char* pStr)
{
    for ( int i = 0; pStr[i] != '\0' ; ++i)
    {
        if (pStr[i] >= 'A' && pStr[i] <= 'Z')
            continue;
        if (pStr[i] >= 'a' && pStr[i] <= 'z')
            continue;
        if (pStr[i] >= '0' && pStr[i] <= '9')
            continue;
        if (pStr[i]  == '_')
            continue;
        pStr[i]     =   g_map[int(pStr[i]) % sizeof(g_map)];
    }
    return  pStr;
}
/// <summary>
/// 获取相对于给定跟目录的相对目录，即 rootPath - path
/// </summary>
/// <param name="rootPath"></param>
/// <param name="path"></param>
/// <returns></returns>
const char* getRelPath(const std::string& rootPath,const char* path)
{
    if (path == nullptr)
        return  "";

    size_t  nLen    =   strlen(path);
    if (nLen < rootPath.size())
        return  "";
    for (size_t i = 0; i < rootPath.size(); i++)
    {
        auto    ch1 =   toupper(path[i]);
        auto    ch2 =   toupper(rootPath[i]);
        if (ch1 == ch2)
            continue;
        else if(ch1 == '/'  && ch2 == '\\')
            continue;
        else if(ch1 == '\\' && ch2 == '/')
            continue;
        return  "";
    }
    auto    relName =   path + rootPath.size();
    if (relName && relName[0] == '/')
        return  relName + 1;
    else
        return  relName;
}


static  String  fileExtension(const char* fileName)
{
    if (*fileName == '.')
        return  fileName;
    fs::path filePath(fileName);
    return  filePath.extension().string();
}
static  String  getFileName(const char* fullPath)
{
    fs::path filePath(fullPath);
    return  filePath.filename().string();
}

static  String  filePath(const char* fileName)
{
    fs::path filePath(fileName);
    return  filePath.parent_path().string();
}
static  bool    makeDir(const char* dir)
{
    namespace   fs  =   std::filesystem;
    try
    {
        /// 成功, 失败，则是已经存在
        if (fs::create_directory(dir))
            return  true;
        else
            return  false;
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

using   WString =   std::wstring;
using   String  =   std::string;
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
/// 获取名称
/// </summary>
/// <param name="rootPath"> 根目录</param>
/// <param name="doc">xml doc 对象</param>
/// <param name="parent">根节点</param>
void    findFileToXML(const char* workPath,const char* rootPath,XMLDoc& doc,XMLNode* parent)
{
    std::string     rootDir =   rootPath;
    std::string     path    =   rootPath;
    if (!fs::exists(path))
        return;
    
    if(fs::is_directory(path))
    {
        XMLNode*    nodePath        =   doc.allocate_node(rapidxml::node_element, "folder", 0);
        fs::path    filePath(path);
        auto        name            =   filePath.stem().string();
        XMLAttr*    aPath           =   doc.allocate_attribute("path", doc.allocate_string(name.c_str()));
        nodePath->append_attribute(aPath);
        parent->append_node(nodePath);
        for ( auto entry : fs::directory_iterator(path))
        {
            String  file    =   entry.path().generic_string();
            findFileToXML(workPath,file.c_str(),doc,nodePath);
        }
    }
    else
    {
        auto    ext =   fileExtension(rootPath);
        if (   _stricmp(ext.c_str(),".fers") == 0 
            || _stricmp(ext.c_str(),".prrs") == 0 
            || _stricmp(ext.c_str(),".cpp")  == 0)
        {
            return  ;
        }
        XMLNode*    nodeFile    =   doc.allocate_node(rapidxml::node_element,"file", 0);
        auto        pName       =   getFileName(rootPath);
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


char*   replacePath(char* pStr)
{
    for (int i = 0; pStr[i] != '\0'; ++i)
    {
        if (pStr[i] == '\\')
            pStr[i] = '/';
    }
    return  pStr;
}

struct  Header
{
    std::string     _srcFileName;
    std::string     _declareDataLenVar;
    std::string     _declareDataVar;

    std::string     _declareDataName;
    std::string     _declareDataLenName;
    size_t          _dataLen;
    const void*     _data;
};

using   Headers =   std::vector<Header>;


void    outFileBody(const char* className,FILE* file,const Headers& headers)
{
    std::string     clsName =   std::string("CLASS_") + className;

    std::string     res;
    res +=  "\n";
    res +=  "/// helper function !\n";
    res +=  "/// \n";
    res +=  "/// comments\n";
    res +=  "namespace CELL\n";
    res +=  "{\n";
    res +=  "   class   " + clsName + "\n";
    res +=  "   {\n";
    res +=  "   public:\n";
    res +=  "       "+ clsName +"()\n";
    res +=  "       {\n";
    for (auto h : headers)
    {
        char     szBuf[1024] = { 0 };
        sprintf(szBuf, "            CELL::CELLAssetsMgr::instance().addBuffer(\"%s\",%s,%s,false);\n", h._srcFileName.c_str(), h._declareDataName.c_str(), h._declareDataLenName.c_str());
        res += szBuf;
        res +=  "\n";
    }
    res +=  "       }\n";
    res +=  "   };\n";
    res +=  "   static  "   +   clsName + "    _" + className + ";\n";
    res +=  "}\n";
    fputs(res.c_str(),file);
    

}
void    outFileHeader(FILE* file)
{
    FEDateTime      dateTime;
    char            szTimeFormat[256] = { 0 };
    dateTime.toDateTimeString(szTimeFormat);
    fputs("/// tool.resource.cc Compiler V1.0.0.0\n", file);
    fputs("/// tool.resource.cc Copyright 2018-2024 PR Inc All rights reserved !\n", file);
    fputs("/// Warning! All changes made in this file will be lost!\n", file);
    fputs("/// Build Time ", file);
    fputs(szTimeFormat, file);

    fputs("\n\n", file);
    fputs("#include \"engine/CELLAssetsMgr.h\"\n", file);
    fputs("#pragma warning(push)\n", file);
    fputs("#pragma warning(disable:4828)\n", file);


    fputs("\n\n", file);
    
}

void    outFileTrail(FILE* file)
{
    fputs("#pragma warning(pop)\n", file);
}
bool    compile( const char* srcFile
                ,const char* dstFile
                ,const char* dstName
                ,const char* sName
                ,Header&     header
                ,FILE*       pDstFile = nullptr)
{
    FILE*   pFile   =   fopen(srcFile,"rb");
    if (pFile == nullptr)
    {
        std::string     gbkFile =   utf8_2_gbk(srcFile);
        pFile   =   fopen(gbkFile.c_str(),"rb");       
    }
    if (pFile == nullptr)
    {
        std::string     utfFile =   gbk_2_utf8(srcFile);
        pFile   =   fopen(utfFile.c_str(),"rb");       
    }
    if (pFile == nullptr)
    {
        printf("open file(%s) error !\n",srcFile);
        return  false;
    }
    else
    {
        printf("start compile file(%s) !\n",srcFile);
    }
    fseek(pFile,0,SEEK_END);
    size_t  nLen    =   ftell(pFile);
    fseek(pFile,0,SEEK_SET);
    unsigned char*   pBuf   =   new unsigned char[nLen + 1];
    fread(pBuf,1,nLen,pFile);
    pBuf[nLen]  =   '\0';
    fclose(pFile);
    /// 使用文件内容生成一个唯一MD5
    FEMd5Key    classId         =   FEMD5::fromBuffer(pBuf,(uint)nLen);


    std::string     legalName       =   dstName;
    replaceDot(legalName.data());
    
    std::string     outDataName     =   std::string("_") + legalName;
    std::string     outDataLenName  =   outDataName;
                    outDataLenName  +=  "_len";
    std::string     result;
    result.reserve(nLen * 4 + 1024);

    char    defDataVar[1024]        =   {0};
    char    defDataLenVar[1024]     =   {0};

    
    char    declareDataVar[1024]    =   {0};
    char    declareDataLenVar[1024] =   {0};

    char    dataLen[16]             =   {0};
    sprintf(dataLen, "%d;\n", int(nLen));

    replaceDot(outDataLenName.data());
    replaceDot(outDataName.data());
   
    sprintf(defDataLenVar,      "   unsigned int    %s      =   %d;\n",outDataLenName.c_str()      ,int(nLen));
    sprintf(defDataVar,         "   unsigned char   %s[]    =   \n",   outDataName.c_str());

    sprintf(declareDataLenVar,  "   unsigned int    %s;",              outDataLenName.c_str());
    sprintf(declareDataVar,     "   unsigned char   %s[];",            outDataName.c_str());


    header._declareDataLenVar   =   declareDataLenVar;
    header._declareDataVar      =   declareDataVar;

    header._declareDataName     =   outDataName;
    header._declareDataLenName  =   outDataLenName;
    header._dataLen             =   nLen;
    header._srcFileName         =   sName;

    result  =   "namespace  CELL\n";
    result  +=  "{\n";
    result  +=  defDataLenVar;
    result  +=  defDataVar;
    result  +=  "   { \n";
    result  +=  "       ";
    for (size_t i = 0 ;i < nLen + 1; ++ i)
    {
        char    szBuf[8]    =   {0};
        sprintf(szBuf,"0x%02x,",pBuf[i]);
        if (i && i%64 == 0)
        {
            result  +=  "\n       ";
        }
        result  +=  szBuf;
    }
    result  +=  "\n";
    result  +=  "   };\n";
    result  +=  "};\n";

    /// 如果有输入文件，则使用
    FILE*   pOutFile    =   pDstFile;
    if (pOutFile == nullptr)
    {
        if (dstFile == nullptr || strlen(dstFile) < 3)
        {
            char    szFile[1024] = { 0 };
            sprintf(szFile, "%s.cpp", srcFile);
            pOutFile    =   fopen(szFile, "wt+");
        }
        else
        {
            pOutFile = fopen(dstFile, "wt+");
        }
        outFileHeader(pOutFile);
    }
    fwrite(result.c_str(),1,result.size(),pOutFile);
    /// pDstFile == nullptr 则说明 pOutFile 是创建的，不是自建的
    /// 需要关闭，否则不关闭
    if (pDstFile == nullptr)
    {
        std::string     className   =   dstName;
        replaceDot(className.data());
        Headers         headers;
        headers.push_back(header);
        outFileBody(className.c_str(),pOutFile,headers);
        outFileTrail(pOutFile);
        fclose(pOutFile);
    }
    printf("end compile file(%s) !\n",srcFile);
    delete  []pBuf;
    return  true;
}


std::string     toClassBody(const Headers& headers)
{
    size_t          index   =   0;
    std::string     clsStr  =   "";
    clsStr += "#include \"engine/CELLAssetsMgr.hpp\"\n";
    clsStr += "namespace CELL\n";
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
 
    clsStr += "    CELLAssetsMgr::CELLAssetsMgr()\n";
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
    clsStr += "    CELLAssetsMgr&    CELLAssetsMgr::instance()\n";
    clsStr += "    {\n";
    clsStr += "         static   CELLAssetsMgr _resource;\n";
    clsStr += "         return   _resource;\n";
    clsStr += "     }\n";
    clsStr += "     const   CELLAssetsMgr::Resource*  CELLAssetsMgr::getResource(const char* fileName)\n";
    clsStr += "     {\n";
    clsStr += "         auto    itr = _resource.find(fileName);\n";
    clsStr += "         if (itr == _resource.end())\n";
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
    clsStr  +=  "namespace  CELL\n";
    clsStr  +=  "{\n";
    clsStr  +=  "   class   CELLAssetsMgr\n";
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
    clsStr  +=  "       static   CELLAssetsMgr&    instance();\n";
    clsStr  +=  "   public:\n";
    clsStr  +=  "       CELLAssetsMgr();\n\n";  
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
        for ( ; child ; child = child->next_sibling())
        {
            XMLAttr*    pPath   =   node->first_attribute("path");
            std::string path    =   workPath;
            if (pPath)
                path    =   workPath + std::string("/") + pPath->value();

            procNode(child, rootDir,path.c_str(), resFile, headers, pOutFile);
        }
    }
    else
    {
        Header      header;
        XMLAttr*    aInput  =   node->first_attribute("input");
        XMLAttr*    aName   =   node->first_attribute("name");
        XMLAttr*    aOutPut =   node->first_attribute("output");
        if (aInput == nullptr)
            return;
        std::string srcFile     =   workPath + "/" + aInput->value();
        std::string dstFile     =   resFile;
        std::string sName       =   "";
        if (aName)
            sName   =   aName->value();
        else if(aInput)
            sName   =   aInput->value();
        else
            sName   =   srcFile.substr(rootDir.size() + 1);

        std::string dstNames    =   sName;
        /// 如果指定了输出文件名称，则使用指定的名称最为变量名称
        if (aOutPut)
            dstNames            =   aOutPut->value();

        replacePath((char*)dstNames.c_str());
        replaceDot((char*)dstNames.c_str());
            
        if (pOutFile)
        {
            fputs("\n\n", pOutFile);
            fputs("/// ", pOutFile);
            fputs(srcFile.c_str(), pOutFile);
            fputs("\n", pOutFile);


            if(compile( srcFile.c_str()
                    ,dstFile.c_str()
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
            dstFile =   std::string(srcFile) + std::string(".cpp");
            if(compile( srcFile.c_str()
                    ,dstFile.c_str()
                    ,dstNames.c_str()
                    ,sName.c_str()
                    ,header
                    ,pOutFile
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
    FILE*   pFile   =   fopen(srcFile,"rb");
    if (pFile == nullptr)
    {
        printf("open file(%s) failed!\n",srcFile);
        return  false;
    }
    fseek(pFile,0,SEEK_END);
    size_t  nLen    =   ftell(pFile);
    fseek(pFile,0,SEEK_SET);
    char*   pData   =   new char[nLen + 1];
    pData[nLen]     =   '\0';
    fread(pData,1,nLen,pFile);
    fclose(pFile);

    std::string     className   =   getFileName(srcFile);
    replaceDot(className.data());
    try
    {
        XMLDoc      doc;
        XMLNode*    rootNode = 0;
        doc.parse<0>(pData);
        rootNode = doc.first_node();
        if (rootNode == 0)
        {
            return false;
        }
        std::string     dataPath;
        std::string     outPath;
        /// $Work,$This
        XMLAttr*        pDataPath   =   rootNode->first_attribute("dataPath");
        XMLAttr*        pOutPath    =   rootNode->first_attribute("outPath");
        XMLAttr*        pSingleOut  =   rootNode->first_attribute("singleOut");
        
        if (pDataPath == 0)
            dataPath    =   workPath;
        else if(strcmp(pDataPath->value(),"$Work") == 0)
            dataPath    =   workPath;
        else if(strcmp(pDataPath->value(),"$This") == 0)
            dataPath    =   ccPath;
        else
            dataPath    =   pDataPath->value();
        if (pOutPath)   
            outPath     =   pOutPath->value();

        FILE*       pOutFile    =   nullptr;
        if (pSingleOut && atoi(pSingleOut->value()) != 0)
            pOutFile    =   fopen(outFile,"w+");
        else
            pOutFile    =   nullptr;

        if (pOutFile)
        {
            outFileHeader(pOutFile);
        }
        replacePath((char*)outFile);

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
        }

        printf("compile finished !\n");
    }
    catch (...)
    {
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
    FILE*   pFile   =   fopen(srcFileName,"rb");
    if (pFile == nullptr)
    {
        printf("open file(%s) failed!\n",srcFileName);
        return  false;
    }
    fseek(pFile,0,SEEK_END);
    size_t  nLen    =   ftell(pFile);
    fseek(pFile,0,SEEK_SET);
    char*   pData   =   new char[nLen + 1];
    pData[nLen]     =   '\0';
    fread(pData,1,nLen,pFile);
    fclose(pFile);


    std::string     className   =   srcFileName;
    replaceDot(className.data());
    try
    {
        XMLDoc      doc;
        XMLNode*    rootNode = 0;
        doc.parse<0>(pData);
        rootNode = doc.first_node();
        if (rootNode == 0)
        {
            return false;
        }
        std::string     path        =   filePath(srcFileName);
        std::string     outPath     =   path;

        FILE*           pOutFile    =   fopen(dstFile,"w+");
        if (pOutFile)
        {
            outFileHeader(pOutFile);
        }
        XMLNode*    nodeQRC     =   rootNode->first_node("qresource");
        for ( ; nodeQRC ; nodeQRC   =   nodeQRC->next_sibling("qresource"))
        {
            XMLNode*    nodeFile    =   nodeQRC->first_node();
            for ( ; nodeFile ; nodeFile = nodeFile->next_sibling())
            {
                Header      header;
                std::string aInput  =   nodeFile->value();
                if (aInput.empty())
                    continue;
                std::string srcFile =   path + "/" + aInput;
            
                replacePath((char*)srcFile.c_str());

                std::string dstNames =   aInput + std::string(".cpp");
                replacePath((char*)dstNames.c_str());
                replaceDot((char*)dstNames.c_str());
            
                if (pOutFile)
                {
                    fputs("\n\n", pOutFile);
                    fputs("/// ", pOutFile);
                    fputs(srcFile.c_str(), pOutFile);
                    fputs("\n", pOutFile);
                }
                compile(srcFile.c_str(),dstFile,dstNames.c_str(),aInput.c_str(),header,pOutFile);
                headers.push_back(header);
            }
        }
        
        if (pOutFile)
        {
            outFileBody(className.c_str(),pOutFile,headers);
            outFileTrail(pOutFile);
            fclose(pOutFile);
        }
    }
    catch (...)
    {
        printf("XML Parse Exception!\n");
    }
    return  true;
}

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

    if (!ok || argc < 6 ) 
    {
        std::cout << a.usage() ;
        return 0;
    }

    /// 1:是工作目录，重要，定位资源的位置
    /// 2:输入文件信息
    /// 3 -out
    /// 4 输出的文件
    
    ArrayString     files;
    Headers         headers;
    /// 编译器的路径
    char            szCCPath[FE_PATH_LENGTH]  =   {0};
    strcpy(szCCPath,argv[0]);

    std::string     ccPath  =   filePath(szCCPath);
    
    /// 工作目录
    std::string     workPath    =   a.get<std::string>("work");
    std::string     packPath    =   a.get<std::string>("in");
    std::string     mode        =   a.get<std::string>("mode");
    std::string     fersFile    =   a.get<std::string>("fers");
    std::string     outDefine   =   a.get<std::string>("out");
    
    std::string     outPath     =   workPath;
    
    std::string     srcFile     =   std::string(packPath.data());
    std::string     outFile     =   a.get<std::string>("out");

    if (fersFile.empty())
        fersFile =   srcFile + ".fers";

    if (outFile.empty())
        outFile =   fersFile + ".cpp";
    
    std::string     outName =   srcFile;
   
    replacePath((char*)ccPath.c_str());
    replacePath((char*)outPath.c_str());
    replacePath((char*)workPath.c_str());
    replacePath((char*)srcFile.c_str());
    replacePath((char*)outFile.c_str());

    replacePath((char*)outName.c_str());
    replaceDot((char*)outName.c_str());

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
        
        XMLAttr*    aDataPath   =   doc.allocate_attribute("dataPath",  packPath.c_str());
        XMLAttr*    aOutPath    =   doc.allocate_attribute("outPath",   "");
        XMLAttr*    aSingleOut  =   doc.allocate_attribute("singleOut", "1");

        root->append_attribute(aDataPath);
        root->append_attribute(aOutPath);
        root->append_attribute(aSingleOut);

        findFileToXML(workPath.c_str(),packPath.c_str(),doc,root);

        std::string xmlResult;
        rapidxml::print(std::back_inserter(xmlResult), doc);
        printf("Save... %s \n",fersFile.c_str());

        
        makeDir(filePath(fersFile.c_str()).c_str());

        FILE*       pFile       =   fopen(fersFile.c_str(),"w+");
        if (pFile)
        {
            fputs(xmlResult.c_str(),pFile);
            fclose(pFile);
        }
        else
        {
            printf("Save %s error !\n",fersFile.c_str());
        }

        if (!outDefine.empty())
        {
            printf("parse %s \n",fersFile.c_str());
            processXML(outPath.c_str(), ccPath.c_str(), fersFile.c_str(), outFile.c_str(),headers);
        }
    }
    else if (mode =="qrc")
    {
        printf("parse %s \n",srcFile.c_str());
        processQRC(outPath.c_str(), ccPath.c_str(), srcFile.c_str(), outFile.c_str(),headers);
    }
    else if (mode =="fers")
    {
        printf("parse %s \n",srcFile.c_str());
        processXML(outPath.c_str(), ccPath.c_str(), srcFile.c_str(), outFile.c_str(),headers);
    }
    return  0;
}

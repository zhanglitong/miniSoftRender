#pragma     once
#include    "FEMath.hpp"
#include    "FEDefine.h"
#include    <filesystem>
#include    <iostream>
namespace   fs  =   std::filesystem;

namespace   FE
{
    class   FEFileSystem
    {
    public:
        enum        FileFlag
        {
            FF_Dir      =   0x001,
            FF_File     =   0x002,
        };
    public:
        /// <summary>
        /// 获取文件名称，不带扩展名
        /// c:/xx/yy/tt.x
        /// return tt
        /// </summary>
        /// <param name="fullPath">全路径</param>
        /// <returns>返回文件名称，不带扩展名部分</returns>
        static  String  fileStem(const char* fullPath)
        {
            fs::path filePath(fullPath);
            return  filePath.stem().string();
        }
        /// <summary>
        /// 获取文件名称
        /// c:/xx/yy/tt.x
        /// return tt.x
        /// </summary>
        /// <param name="fullPath">全路径</param>
        /// <returns>返回文件扩展名</returns>
        static  String  fileName(const char* fullPath)
        {
            fs::path filePath(fullPath);
            return  filePath.filename().string();
        }
        /// <summary>
        /// 获取文件夹下的文件或者文件夹，不做递归处理
        /// </summary>
        /// <param name="dir">被检索文件夹</param>
        /// <param name="filter">过滤器</param>
        /// <returns>返回名称，相对于 dir</returns>
        static  Strings entryList(const String& dir,const String& ext = "",Flags filter = FF_File)
        {
            Strings files;
            if (!std::filesystem::exists(dir))
            {
                return  {};
            }
            for(const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if(filter.hasFlag(FF_File) && std::filesystem::is_regular_file(entry))
                {
                    if (ext.empty())
                        files.push_back(entry.path().filename().string());
                    else if(_stricmp(entry.path().extension().string().c_str(),ext.c_str()) == 0)
                        files.push_back(entry.path().filename().string());
                }
                else if(filter.hasFlag(FF_Dir) && std::filesystem::is_directory(entry))
                {
                    files.push_back(entry.path().filename().string());
                }
            }
            return  files;
        }

        /// <summary>
        /// 文件/文件夹是否存在
        /// </summary>
        static  bool    fileExists(const std::string& file)
        {
            return  std::filesystem::exists(file);
        }
        static  bool    fileExists(const char* file)
        {
            return  std::filesystem::exists(file);
        }
        /// <summary>
        /// 判断是否是文件夹
        /// </summary>
        static  bool    isDirectory(const std::string& path)
        {
            return  std::filesystem::exists(path) && std::filesystem::is_directory(path);
        }
        static  bool    isDirectory(const char* path)
        {
            return  std::filesystem::exists(path) && std::filesystem::is_directory(path);
        }
        /// <summary>
        /// 判断是否是文件
        /// </summary>
        static  bool    isFile(const std::string& path)
        {
            return  std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
        }
        static  bool    isFile(const char* path)
        {
            return  std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
        }
        /// <summary>
        /// 创建文件夹
        /// </summary>
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
        /// <summary>
        /// 拷贝文件
        /// </summary>
        static  bool    copyFile(const char* existFileName, const char* newFileName, bool bFailIfExist)
        {
#if 1
            /// 源文件路径
            fs::path source         =   existFileName;
            /// 目标文件路径
            fs::path destination    =   newFileName; 
            try
            {
                /// 确保源文件存在
                if (!fs::exists(source))
                {
                    std::cerr << "source file does not exist!" << std::endl;
                    return false;
                }
                if (bFailIfExist)
                {
                    if ( fs::exists(destination))
                    {
                        std::cerr << "destination file exist!" << std::endl;
                        return  false;
                    }
                }
                /// 复制文件
                fs::copy(source, destination, fs::copy_options::overwrite_existing);
                return  true;
            }
            catch (const fs::filesystem_error& e)
            {
                std::cerr << e.what() << std::endl;
                return false;
            }
#else
            FILE*   pSrc    =   nullptr;
            FILE*   pDes    =   nullptr;
            if((pDes = fopen(newFileName, "r")) != nullptr )
            {
                if(bFailIfExist)
                    return false;
                fclose(pDes);
            }
            bool    hasCopy =   false;
            do
            {
                pSrc    =   fopen(existFileName,"rb");
                pDes    =   fopen(newFileName,  "wb+");
                if (pSrc == nullptr || pDes == nullptr)
                {
                    break;
                }
                size_t  nlen        =   0;
                char    buf[4096]   =   {0};
                while (nlen = fread(buf, 1, sizeof(buf), pSrc) > 0)
                {
                    fwrite(buf, 1, nlen, pDes);
                }
                hasCopy =   true;

            } while (false);

            if(pSrc != nullptr)
            {
                fclose(pSrc);
                pSrc    =   nullptr;
            }
            if(pDes != nullptr)
            {
                fclose(pDes);
                pDes    =   nullptr;
            }
            return hasCopy;
#endif
        }
        /// <summary>
        /// 删除文件，内部判断是否是文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        static  bool    removeFile(const char* file)
        {
            namespace fs = std::filesystem;
            if (isFile(file))
            {
                return  fs::remove(file);
            }
            else
            {
                return  false;
            }
        }
        /// <summary>
        /// 删除目录以及其中的所有文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        static  bool    removeDir(const char* dir)
        {
            namespace fs = std::filesystem;
            if (isDirectory(dir))
            {
                fs::remove_all(dir);
                return  true;
            }
            else
            {
                return  false;
            }
        }
        /// <summary>
        /// c:/xx/yy/tt.x
        /// return c:/xx/yy/
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns>文件路径</returns>
        static  String  filePath(const char* fileName)
        {
            fs::path filePath(fileName);
            return  filePath.parent_path().string();
        }
        /// <summary>
        /// 获取文件扩展名
        /// c:/xx/yy/tt.x
        /// return .x
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns>返回文件扩展名</returns>
        static  String  fileExtension(const char* fileName)
        {
            if (*fileName == '.')
                return  fileName;
            fs::path filePath(fileName);
            return  filePath.extension().string();
        }
    };

}
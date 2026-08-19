#pragma     once
#include    "FEMath.hpp"
#include    "FEDefine.h"
#include    <filesystem>
namespace   fs  =   std::filesystem;

namespace   FE
{
    class   FEFileInfor
    {
    public:
        /// <summary>
        /// 获取文件大小
        /// </summary>
        /// <param name="filePathName"></param>
        /// <returns></returns>
        static  uint64  fileSize(const String& filePathName)
        {
            fs::path    file    =   filePathName;
            try 
            {
                return  fs::file_size(file);
            } 
            catch (const fs::filesystem_error& e) 
            {
                printf("%s\n",e.what());
                return  0;
            }
        }
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
    };

}

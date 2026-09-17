#pragma     once

#include    "FEMath.hpp"
#include    <array>
#include    "FEBuffer.hpp"
#include    "FEMutex.hpp"
#include    "FEObject.h"

namespace FE
{
    /// <summary>
    /// 数据集合管理，单例对象，用来管理系统加载的所有资源数据
    /// </summary>
    class   FE_API  FEAssetsMgr: public FEReference<Flags>
    {
    public:
        friend  class   FEContext;
    public:
        using   BufferMap   =   std::map<std::string,BufferPtr>;
        using   String      =   std::string;
    protected:
        BufferMap   _buffers;
        FEMutex     _mutex;
        String      _rootPath;
   protected:
       FEAssetsMgr();
    public:
        /// <summary>
        /// 设置资源的根路径，用来提取相对路径使用
        /// </summary>
        /// <param name="path"></param>
        void        setRootPath(const String& path);
        /// <summary>
        /// 获取相对于_rootPath 相对路径，如果不是相对于_rootPath 的路径，则返回"";
        /// case 0:
        /// _rootPath = C:/xxx/asss/
        /// path      = C:/xxx/asss/data/xxxxads.jpg
        /// return  data/xxxxads.jpg
        /// case 1:
        /// _rootPath = C:/xxx/tttt/
        /// path      = C:/xxx/asss/data/xxxxads.jpg
        /// return  nullptr
        /// case 2:
        /// _rootPath = C:\xxx\tttt\
        /// path      = C:/xxx/tttt/data/xxxxads.jpg
        /// return  data/xxxxads.jpg
        /// </summary>
        PCSTR       getRelPath(const char* path) const;
        /// <summary>
        /// 添加缓冲区对象到系统中
        /// </summary>
        /// <param name="name">名称，唯一标志，系统会查询，如果存在则返回false</param>
        /// <param name="buffe">内存地址</param>
        /// <param name="length">长度</param>
        /// <returns>如果已经存在同名对象，失败，否则true</returns>
        BufferPtr   addBuffer(const char* name,const void* buffer,size_t length,bool bFree) ;
        /// <summary>
        /// 添加缓冲区对象到系统中
        /// </summary>
        /// <param name="ptr"></param>
        /// <returns></returns>
        BufferPtr   addBuffer(const char* name,BufferPtr ptr) ;
        /// <summary>
        /// 替换缓冲区
        /// </summary>
        /// <param name="name"></param>
        /// <param name="ptr"></param>
        /// <returns></returns>
        BufferPtr   replaceBuffer(const char* name,BufferPtr ptr) ;      
        /// <summary>
        /// 根据给定名称移除缓冲区对象
        /// </summary>
        /// <param name="name"></param>
        bool        removeBuffer(const char* name) ;
        /// <summary>
        /// 获取缓冲区对象
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        BufferPtr   queryBuffer(const char* name) ;
        /// <summary>
        /// 清除所有缓存区对象
        /// </summary>
        void        clearBuffer() ;
        /// <summary>
        /// 返回所有buffer
        /// </summary>
        /// <returns></returns>
        inline auto&    bufferMap()
        {
            return  _buffers;
        }
        /// <summary>
        /// 返回所有buffer
        /// </summary>
        /// <returns></returns>
        inline auto&    bufferMap() const
        {
            return  _buffers;
        }
    public:
        /// <summary>
        /// 获取单实例对象
        /// </summary>
        /// <returns></returns>
        static  FEAssetsMgr&    instance();
    };


    using   AssetsMgr =   SharedPtr<FEAssetsMgr>;
}



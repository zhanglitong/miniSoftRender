#include    "../inc/FEAssetsMgr.h"

namespace FE
{
    FEAssetsMgr*    sInstance   =   nullptr;
    FEAssetsMgr::FEAssetsMgr(FEContext& ctx)    
        :FEObject(ctx)
    {
        assert(sInstance == nullptr);
        sInstance   =   this; 
    }
    void    FEAssetsMgr::setRootPath(const String& path)
    {
        _rootPath   =   path;
        for (auto& var : _rootPath)
        {
            if (var == '\\')    var =   '/';
        }
    }
        
    PCSTR   FEAssetsMgr::getRelPath(const char* path) const
    {
        if (path == nullptr)
            return  nullptr;

        size_t  nLen    =   strlen(path);
        if (nLen < _rootPath.size())
            return  nullptr;
        for (size_t i = 0; i < _rootPath.size(); i++)
        {
            auto    ch1 =   toupper(path[i]);
            auto    ch2 =   toupper(_rootPath[i]);
            if (ch1 == ch2)
                continue;
            else if(ch1 == '/'  && ch2 == '\\')
                continue;
            else if(ch1 == '\\' && ch2 == '/')
                continue;
            return  nullptr;
        }
        auto    result  =   path + _rootPath.size();
        if (result && result[0] == '/')
            return  result + 1;
        else if (result && result[0] == '\\')
            return  result + 1;
        else
            return  result;
    }
    /// <summary>
    /// 添加缓冲区对象到系统中
    /// </summary>
    /// <param name="name">名称，唯一标志，系统会查询，如果存在则返回false</param>
    /// <param name="buffe">内存地址</param>
    /// <param name="length">长度</param>
    /// <returns>如果已经存在同名对象，失败，否则true</returns>
    BufferPtr   FEAssetsMgr::addBuffer(const char* pName,const void* buffer,size_t length,bool bFree) 
    {
        FEMutex::ScopeLock lk(_mutex);
        String  name(pName);
        auto    itr =   _buffers.find(name);
        if (itr != _buffers.end())
            return  nullptr;
        BufferPtr   buf =   new FEBuffer(_ctx);
        FEBuffer::CreateInfo    infor   =   {};
        infor._offset   =   0;
        infor._range    =   length;
        infor._ref      =   (uint8*)buffer;
        infor._free     =   bFree;
        buf->create(infor);
        _buffers[name]  =   buf;
        return  buf;
    }
        
    BufferPtr   FEAssetsMgr::addBuffer(const char* pName,BufferPtr ptr) 
    {
        FEMutex::ScopeLock lk(_mutex);
        String  name(pName);
        auto    itr =   _buffers.find(name);
        if (itr != _buffers.end())
            return  nullptr;
        else
            _buffers[name]  =   ptr;
        return  ptr;
    }
        
    BufferPtr   FEAssetsMgr::replaceBuffer(const char* name,BufferPtr ptr)
    {
        FEMutex::ScopeLock lk(_mutex);
        auto    itr =   _buffers.find(name);
        assert(itr != _buffers.end());
        if (itr == _buffers.end())
            return  nullptr;

        BufferPtr   old =   itr->second;
        itr->second     =   ptr;
        return  old;
    }
        
    bool        FEAssetsMgr::removeBuffer(const char* pName) 
    {
        FEMutex::ScopeLock lk(_mutex);
        String  name(pName);
        auto    itr =   _buffers.find(name);
        if (itr == _buffers.end())
            return  false;
        _buffers.erase(itr);
        return  true;
    }
        
    BufferPtr   FEAssetsMgr::queryBuffer(const char* pName)
    {
        FEMutex::ScopeLock lk(_mutex);
        String  name(pName);
        auto    itr =   _buffers.find(name);
        if (itr == _buffers.end())
            return  nullptr;
        else
            return  itr->second;
    }
        
    void        FEAssetsMgr::clearBuffer() 
    {
        FEMutex::ScopeLock lk(_mutex);
        _buffers.clear();
    }
    /// <summary>
    /// 获取单实例对象
    /// </summary>
    /// <returns></returns>
    FEAssetsMgr&FEAssetsMgr::instance()
    {
        return  *instancePtr();
    }
    FEAssetsMgr*FEAssetsMgr::instancePtr()
    {
        assert(sInstance!=nullptr);
        return  sInstance;
    }
}



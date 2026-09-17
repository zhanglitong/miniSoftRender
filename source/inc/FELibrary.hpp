#pragma     once

#include    "FEPlatform.h"

#if      FE_PLATFORM == FE_PLATFORM_WIN32
namespace   FE
{
    /// <summary>
    /// 注意析构函数不会卸载，必须手动调用 unload 进行释放
    /// </summary>
    class   FELibrary 
    {
    public:
        std::string _path;
        HMODULE     _location   =   nullptr;  
    public:
        FELibrary()
        {}

        virtual ~FELibrary()
        {}
        /// <summary>
        /// 获取名称
        /// </summary>
        inline  auto    path() const
        {
            return  _path.c_str();
        }
        /// <summary>
        /// 获取对象
        /// </summary>
        inline  auto    handle() const
        {
            return  _location;
        }
        /// <summary>
        /// 判断是否已经加载
        /// </summary>
        inline  bool    isLoad() const
        {
            return  _location != nullptr;
        }
        /// <summary>
        /// 加载库
        /// </summary>
        virtual bool    load(const char* path)
        {
            unload();
            _path       =   path;
            _location   =   LoadLibraryA(path);
            return  _location != nullptr;
        }
        /// <summary>
        /// 卸载库
        /// </summary>
        virtual bool    unload()
        {
            if (_location)
            {
                BOOL    res =   FreeLibrary(_location);
                _location   =   nullptr;
                _path       =   "";
                return  res ? true : false;
            }
            return  false;
        }
        /// <summary>
        /// 获取函数地址
        /// </summary>
        virtual void*   getFunction(const char* funName) const
        {
            if (_location)
                return  GetProcAddress(_location,funName);
            else
                return  nullptr;
        }
    };
}

#elif FE_PLATFORM == FE_PLATFORM_LINUX

#include    <dlfcn.h>
namespace   FE
{
    class   FELibrary
    {
    public:
        std::string _name;
        void*       _location   =   nullptr;
    public:
        FELibrary()
        {}
        virtual ~FELibrary()
        {}
        /// <summary>
        /// 获取名称
        /// </summary>
        inline  auto    path() const
        {
            return  _name.c_str();
        }
        /// <summary>
        /// 获取对象
        /// </summary>
        inline  auto    handle() const
        {
            return  _location;
        }
        /// <summary>
        /// 判断是否已经加载
        /// </summary>
        inline  bool    isLoad() const
        {
            return  _location != nullptr;
        }
        /// <summary>
        /// 加载库
        /// </summary>
        virtual bool    load(const char* path)
        {
            unload();
            if((_location = dlopen(path, RTLD_LAZY)) == NULL)
            {
                printf("dloprn - %s\n",dlerror());
            }
            _name   =   path;
            return  _location != nullptr;
        }
        /// <summary>
        /// 卸载库
        /// </summary>
        virtual bool    unload()
        {
            if (_location)
            {
                auto    result  =   dlclose(_location) ;
                _location       =   nullptr;
                _name           =   "";
                return  0 == result;
            }
            return  false;
        }
        /// <summary>
        /// 获取函数地址
        /// </summary>
        virtual void*   getFunction(const char* funName) const
        {
            return  dlsym(_location, funName);
        }
    };
}
#elif FE_PLATFORM == FE_PLATFORM_ANDROID
#include    <dlfcn.h>
namespace   FE
{
    class   FELibrary
    {
    public:
        std::string _name;
        void*       _location   =   nullptr;
    public:
        FELibrary()
        {
        }

        virtual ~FELibrary()
        {}
        /// <summary>
        /// 获取名称
        /// </summary>
        inline  auto    path() const
        {
            return  _name.c_str();
        }
        /// <summary>
        /// 获取对象
        /// </summary>
        inline  auto    handle() const
        {
            return  _location;
        }
        /// <summary>
        /// 判断是否已经加载
        /// </summary>
        inline  bool    isLoad() const
        {
            return  _location != nullptr;
        }
        /// <summary>
        /// 加载库
        /// </summary>
        virtual bool    load(const char* path)
        {
            unload();
            if((_location = dlopen(path, RTLD_LAZY | RTLD_GLOBAL)) == NULL)
            {
                printf("dloprn - %s\n",dlerror());
            }
            _name   =   path;
            return  _location != nullptr;
        }
        /// <summary>
        /// 卸载库
        /// </summary>
        virtual bool    unload()
        {
            if (_location)
            {
                auto    result  =   dlclose(_location) ;
                _location       =   nullptr;
                _name           =   "";
                return  0 == result;
            }
            return  false;
        }
        /// <summary>
        /// 获取函数地址
        /// </summary>
        virtual void*   getFunction(const char* funName) const
        {
            if (funName && _location)
                return  dlsym(_location, funName);
            else
                return  nullptr;
        }
    };
}
#endif

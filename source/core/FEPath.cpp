/// <summary>
/// FEPath 实现:按平台取可执行文件路径,供配置等需要"exe 同目录"的功能使用。
/// </summary>

#include    "FEPath.hpp"

#if FE_PLATFORM == FE_PLATFORM_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include    <windows.h>
#else
    #include    <unistd.h>
#endif

namespace   FE
{
    std::filesystem::path   exePath()
    {
#if FE_PLATFORM == FE_PLATFORM_WIN32
        wchar_t         buffer[1024]    =   {};
        const   DWORD   length  =   GetModuleFileNameW(nullptr,buffer,
                                  (DWORD)(sizeof(buffer) / sizeof(buffer[0])));
        if (length == 0 || length >= sizeof(buffer) / sizeof(buffer[0]))
            return  std::filesystem::path();
        return  std::filesystem::path(buffer);
#else
        char            buffer[4096]    =   {};
        const   ssize_t length  =   readlink("/proc/self/exe",buffer,sizeof(buffer) - 1);
        if (length <= 0)
            return  std::filesystem::path();
        buffer[length]  =   '\0';
        return  std::filesystem::path(buffer);
#endif
    }

    std::filesystem::path   exeDir()
    {
        const   std::filesystem::path   path    =   exePath();
        return  path.empty() ? path : path.parent_path();
    }
}

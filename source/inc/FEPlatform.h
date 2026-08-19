#pragma     once
#include    <stdlib.h>
#include    <stdio.h>

#define     FE_PLATFORM_WIN32     1
#define     FE_PLATFORM_LINUX     2
#define     FE_PLATFORM_ANDROID   3

#if defined _WIN32 || defined _WIN64 || defined WIN32
    #define FE_PLATFORM     FE_PLATFORM_WIN32
#elif defined (_ANDROID_)
    #define FE_PLATFORM     FE_PLATFORM_ANDROID
#elif defined _LINUX_ || defined LINUX

    #   define      FE_PLATFORM     FE_PLATFORM_LINUX

    #   define      _fseeki64       fseeko64
    #   define      _ftelli64       ftello64

    #   define      sprintf_s       sprintf
    #   define      _stricmp        strcasecmp
    #   define      stricmp         strcasecmp
    #   define      strnicmp        strncasecmp
    #   define      _strnicmp       strncasecmp
    #   define      _strcmpi        strcmp
#else

    #   define      _fseeki64       fseeko64
    #   define      _ftelli64       ftello64

    #   define      sprintf_s       sprintf
    #   define      _stricmp        strcasecmp
    #   define      stricmp         strcasecmp
    #   define      strnicmp        strncasecmp
    #   define      _strnicmp       strncasecmp
    #   define      _strcmpi        strcmp
#endif

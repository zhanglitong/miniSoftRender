#pragma     once

#include    "FEDefine.h"

#include    <filesystem>

namespace   FE
{
    /// <summary>
    /// 可执行文件的完整路径(按平台实现,取不到时返回空 path):
    ///   Windows: GetModuleFileNameW(宽字符,避免中文路径被代码页截断)
    ///   Linux:   /proc/self/exe
    /// </summary>
    FE_API  std::filesystem::path   exePath();
    /// <summary>
    /// 可执行文件所在目录(exePath 为空时同样返回空)
    /// </summary>
    FE_API  std::filesystem::path   exeDir();
}

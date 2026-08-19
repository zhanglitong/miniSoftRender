#include    "demo.h"

#ifdef _WIN32
#include    <windows.h>
#endif

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc,char** argv)
{
#ifdef _WIN32
    // 分配控制台，让 printf 能看到 WebGPU validation 层的输出
    AllocConsole();
    freopen_s((FILE**)stdout,"CONOUT$","w",stdout);
    freopen_s((FILE**)stderr,"CONOUT$","w",stderr);
    setvbuf(stdout,nullptr,_IONBF,0);
    setvbuf(stderr,nullptr,_IONBF,0);
#endif

    FE::Demo    demo;
    demo.main();

    return  0;
}
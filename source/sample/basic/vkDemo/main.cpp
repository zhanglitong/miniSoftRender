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
    // 鍒嗛厤鎺у埗鍙帮紝璁?printf 鑳界湅鍒?WebGPU validation 灞傜殑杈撳嚭
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

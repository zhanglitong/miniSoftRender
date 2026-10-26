#include    "demo.h"
#include    <cstdlib>


#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);


int     main(int argc,char** argv)
{
    /// 解析命令行参数选择渲染后端,默认 Vulkan
    /// 用法: vkDemo.exe [--vulkan|--webgpu] [--msaa N]
    FEUuid  rendererId  =   RS_VULKAN;
    uint    msaa        =   1;   ///< 多采样数(MSAA),默认 1x
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0 ||
            _stricmp(arg,"/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0 ||
                 _stricmp(arg,"/vulkan") == 0)
        {
            rendererId  =   RS_VULKAN;
        }
        else if (_stricmp(arg,"--msaa") == 0 || _stricmp(arg,"-msaa") == 0 ||
                 _stricmp(arg,"/msaa") == 0)
        {
            if (i + 1 < argc)
                msaa    =   (uint)atoi(argv[++i]);
        }
    }

    FE::Demo    demo(rendererId, msaa);
    demo.main();

    return  0;
}

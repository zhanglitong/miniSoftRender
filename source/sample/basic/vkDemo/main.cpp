#include    "demo.h"


#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);


int     main(int argc,char** argv)
{
    /// 解析命令行参数选择渲染后端,默认 Vulkan
    /// 用法: vkDemo.exe [--vulkan|--webgpu]
    FEUuid  rendererId  =   RS_VULKAN;
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
    }

    FE::Demo    demo(rendererId);
    demo.main();

    return  0;
}

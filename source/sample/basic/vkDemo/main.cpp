#include    "demo.h"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc,char** argv)
{
    FE::Demo    demo;
    demo.main();

    return  0;
}
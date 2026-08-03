#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FERenderSystem.h"
#include    "VKRenderSystem.h"
#include    <vulkan/vulkan.h>

namespace  FE
{
    uint    loadPlugin_rs_vulkan(FEContext&)
    {
        //DEFINE_CLASS_CREATOR(VKRenderSystem);
        return  0;
    }
}
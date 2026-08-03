#pragma     once
#include    "graphic/FEDevice.h"
#include    "FEVulkan.h"
namespace   FE
{
    inline  static  PFN_vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT    { nullptr };
    inline  static  PFN_vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT      { nullptr };
    inline  static  PFN_vkCmdInsertDebugUtilsLabelEXT   vkCmdInsertDebugUtilsLabelEXT   { nullptr };

    class   VKDebugUtils
    {
    public:
        static  void    setup(VkInstance _instance)
        {
            vkCmdBeginDebugUtilsLabelEXT    =    reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance,    "vkCmdBeginDebugUtilsLabelEXT"));
            vkCmdEndDebugUtilsLabelEXT      =    reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance,      "vkCmdEndDebugUtilsLabelEXT"));
            vkCmdInsertDebugUtilsLabelEXT   =    reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance,   "vkCmdInsertDebugUtilsLabelEXT"));
        }
        static  void    cmdBeginLabel(VkCommandBuffer cmdbuffer, const String& caption, const float4& color)
        {
            if (!vkCmdBeginDebugUtilsLabelEXT) 
                return;
            VkDebugUtilsLabelEXT labelInfo{};
            labelInfo.sType         =    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            labelInfo.pLabelName    =    caption.c_str();
            memcpy(labelInfo.color, &color[0], sizeof(color));
            vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);
        }

        static  void    cmdEndLabel(VkCommandBuffer cmdbuffer)
        {
            if (!vkCmdEndDebugUtilsLabelEXT) 
                return;
            vkCmdEndDebugUtilsLabelEXT(cmdbuffer);
        }
    };

}
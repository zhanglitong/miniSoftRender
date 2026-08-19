#pragma     once

#include    "graphic/RSObject.h"
#include    "FEVulkan.h"
namespace   FE
{
    using   Externs         =   std::vector<VkExtensionProperties>;
    class   VKInstanceExtensions
    {
    public:
        Externs     _externs;
    public:
        VKInstanceExtensions()
        {
            uint        extCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
            if (extCount > 0)
            {
                _externs.resize(extCount);
                vkEnumerateInstanceExtensionProperties(nullptr, &extCount, _externs.data());
            }
        }
        ~VKInstanceExtensions()
        {}

        bool        hasExtern(const char* name) const
        {
            for (auto& var :_externs)
            {
                if (0 == strcmp(var.extensionName, name))
                    return  true;
            }
            return  false; 
        }
        PCSTRs      defaults()
        {
            PCSTRs  instanceExts;

            if (hasExtern(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
                instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            if (hasExtern(VK_KHR_SURFACE_EXTENSION_NAME))
            {
                instanceExts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            }
            if (hasExtern(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            {
                instanceExts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
#if defined(_WIN32)
            if(hasExtern(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
                instanceExts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
            if(hasExtern(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
            if(hasExtern(VK_KHR_DISPLAY_EXTENSION_NAME))
            	instanceExts.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
            if(hasExtern(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            if(hasExtern(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            if(hasExtern(VK_KHR_XCB_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            if(hasExtern(VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);               
#elif defined(VK_USE_PLATFORM_IOS_MVK)
            if(hasExtern(VK_MVK_IOS_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            if(hasExtern(VK_MVK_MACOS_SURFACE_EXTENSION_NAME))
            	instanceExts.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
            if(hasExtern(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
	            instanceExts.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
            if(hasExtern(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME))
	            instanceExts.push_back(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME);
#endif
            return  instanceExts;
        }
    };
}

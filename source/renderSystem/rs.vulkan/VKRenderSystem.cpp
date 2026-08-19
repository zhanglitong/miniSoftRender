
#include    "VKRenderSystem.h"

namespace   FE
{

    VKRenderSystem::~VKRenderSystem()
    {
        destroy();
    }
    void        VKRenderSystem::destroy() 
    {
        if (vkDestroyDebugUtilsMessenger && _debuger)
        {
            vkDestroyDebugUtilsMessenger(_native,_debuger,nullptr);
        }
        if (_native)
        {
            vkDestroyInstance(_native, nullptr);
        }
    }

    FEResult    VKRenderSystem::createInstanceImpl()
    {
        uint32      instVersion =   0;
        vkEnumerateInstanceVersion(&instVersion);

        uint32_t    major       =   VK_VERSION_MAJOR(instVersion);
        uint32_t    minor       =   VK_VERSION_MINOR(instVersion);
        uint32_t    patch       =   VK_VERSION_PATCH(instVersion);

        (void)major;
        (void)minor;
        (void)patch;    

        VkApplicationInfo   appInfo{};
        appInfo.sType                   =   VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName        =   "PR Engine";
        appInfo.applicationVersion      =   VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName             =   "PR Engine";
        appInfo.engineVersion           =   0;
        appInfo.apiVersion              =   instVersion;

        VKInstanceLayers        layers;
        VKInstanceExtensions    extensions;
        PCSTRs                  instExts    =   extensions.defaults();
        PCSTRs                  instLayers  =   layers.defaults();
        VkInstanceCreateInfo    cInfo{};
        cInfo.sType                     =   VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        cInfo.pApplicationInfo          =   &appInfo;

        cInfo.enabledExtensionCount     =   static_cast<uint32_t>(instExts.size());
        cInfo.ppEnabledExtensionNames   =   instExts.data();


        if (_enableValidationLayers)
        {
            cInfo.enabledLayerCount     =   static_cast<uint>(instLayers.size());
            cInfo.ppEnabledLayerNames   =   instLayers.size() > 0 ? instLayers.data() : NULL;
            cInfo.pNext                 =   nullptr;
        }
        else
        {
            cInfo.enabledLayerCount     =   0;
            cInfo.pNext                 =   nullptr;
        }
        auto    result  =   vkCreateInstance(&cInfo, nullptr, &_native);
        if ( result != VK_SUCCESS)
        {
            assert(0 != 0 && "failed to create instance!");
            return  FEResult::ER_FAILED;
        }

        vkCreateDebugUtilsMessenger     =   reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_native,   "vkCreateDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessenger    =   reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_native,  "vkDestroyDebugUtilsMessengerEXT"));


        if (_enableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            debugCreateInfo.sType           =   VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

            debugCreateInfo.messageSeverity =   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            debugCreateInfo.messageType     =   VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

            debugCreateInfo.pfnUserCallback =   VKRenderSystem::debugCallback;
            debugCreateInfo.pUserData       =   this;


            vkCreateDebugUtilsMessenger(_native,&debugCreateInfo,nullptr,&_debuger);
        }
        return  FEResult::ER_SUCCESS;
    }
    GPUs        VKRenderSystem::gpuListImpl() const 
    {
        uint32_t        gpuCount    =   0;
        PhysicalDevices physicalDevices;
        GPUs            result;
        do
        {
            if (_native == nullptr)
                break;
            if (vkEnumeratePhysicalDevices(_native, &gpuCount, nullptr) != VK_SUCCESS)
                break;
            physicalDevices.resize(gpuCount);
            if (vkEnumeratePhysicalDevices(_native, &gpuCount, physicalDevices.data()) != VK_SUCCESS)
                break;
            for (auto card: physicalDevices) 
            {
                VkPhysicalDeviceProperties      prop;
                vkGetPhysicalDeviceProperties(card, &prop);

                VkPhysicalDeviceIDProperties    deviceIDProps   =   {};

                deviceIDProps.sType             =   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
                deviceIDProps.pNext             =   nullptr;
                VkPhysicalDeviceProperties2 props2 =   {};
                props2.sType                    =   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                props2.pNext                    =   &deviceIDProps;
                vkGetPhysicalDeviceProperties2(card, &props2);

                GPU     gpu;
                gpu.gpuId   =   FEUuid::fromBuffer(deviceIDProps.deviceUUID);
                gpu.name    =   prop.deviceName;

                uint32  major   =   VK_VERSION_MAJOR(prop.apiVersion);  
                uint32  minor   =   VK_VERSION_MINOR(prop.apiVersion);  
                uint32  patch   =   VK_VERSION_PATCH(prop.apiVersion);
                gpu.apiVersion  =   VK_MAKE_API_VERSION(0, major, minor, patch);
                gpu.gpu         =   card;
                switch(prop.deviceType)
                {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER           :  gpu.type    =   DEV_TYPE_OTHER         ; break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU  :  gpu.type    =   DEV_TYPE_INTEGRATED_GPU; break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU    :  gpu.type    =   DEV_TYPE_DISCRETE_GPU  ; break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU     :  gpu.type    =   DEV_TYPE_VIRTUAL_GPU   ; break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU             :  gpu.type    =   DEV_TYPE_CPU           ; break;
                }
                result.emplace_back(gpu);
            }
        } while (false);

        return  result;
    }

    void        VKRenderSystem::debug(VkDebugUtilsMessageSeverityFlagBitsEXT flg,const char* msg)
    {
        switch(flg)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT    :
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT       :
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT    :
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT      :
            break;
        }
        LOG_DBG("GPU:%s",msg);
    }


    VkBool32 VKRenderSystem::debugCallback(   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
                                            , VkDebugUtilsMessageTypeFlagsEXT messageType
                                            , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
                                            , void* pUserData)
    {
        (void)messageType;
        VKRenderSystem*  pThis   =   (VKRenderSystem*)pUserData;
        if (pThis == nullptr)
            return  VK_FALSE;
        pThis->debug(messageSeverity,pCallbackData->pMessage);
        static      bool    bOut    =   false;
        static      bool    bFile   =   false;
        if (bFile)
        {
            FILE*   pFile   =   fopen("error.txt","wt");
            fputs(pCallbackData->pMessage,pFile);
            fclose(pFile);
        }
        return VK_FALSE;
    }
}

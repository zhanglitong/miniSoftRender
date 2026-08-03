#pragma     once

#include    "VKSwapchain.h"
#include    "VKDevice.h"
#include    "VKRenderSystem.h"
#include    "VKGImageView.h"

namespace   FE
{
    VKSwapchain::~VKSwapchain()
    {
        cleanup();
    }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    void    VKSwapchain::initSurface(void* platformHandle, void* platformWindow)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    void    VKSwapchain::initSurface(ANativeWindow* window)
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
    void    VKSwapchain::initSurface(IDirectFB* dfb, IDirectFBSurface* window)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    void    VKSwapchain::initSurface(wl_display *display, wl_surface *window)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    void    VKSwapchain::initSurface(xcb_connection_t* connection, xcb_window_t window)
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    void    VKSwapchain::initSurface(void* view)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    void    VKSwapchain::initSurface(CAMetalLayer* metalLayer)
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
    void    VKSwapchain::initSurface(uint32_t width, uint32_t height)
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
    void    VKSwapchain::initSurface(screen_context_t screen_context, screen_window_t screen_window)
#endif
    {
        auto&       vkDevice        =   (VKDevice&)(_ctx.device());
        auto        physicalDevice  =   vkDevice.physicalDevice();
        auto        device          =   vkDevice.logicalDevice();
        auto        instance        =   (VkInstance)vkDevice.renderSystem().native();
        VkResult    error           =   VK_SUCCESS;
        assert(physicalDevice);
        assert(device);
        assert(instance);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance =   (HINSTANCE)platformHandle;
        surfaceCreateInfo.hwnd      =   (HWND)platformWindow;
        error                       =   vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.window    =   window;
        error                       =   vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
        VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
        surfaceCreateInfo.pNext     =   NULL;
        surfaceCreateInfo.flags     =   0;
        surfaceCreateInfo.pView     =   view;
        error                       =   vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        surfaceCreateInfo.pNext     =   NULL;
        surfaceCreateInfo.flags     =   0;
        surfaceCreateInfo.pView     =   view;
        error                       =   vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfaceCreateInfo.pNext     =   NULL;
        surfaceCreateInfo.flags     =   0;
        surfaceCreateInfo.pLayer    =   metalLayer;
        error                       =   vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(_DIRECT2DISPLAY)
        createDirect2DisplaySurface(width, height);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
        VkDirectFBSurfaceCreateInfoEXT surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
        surfaceCreateInfo.dfb       =   dfb;
        surfaceCreateInfo.surface   =   window;
        error                       =   vkCreateDirectFBSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.display   =   display;
        surfaceCreateInfo.surface   =   window;
        error                       =   vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection=   connection;
        surfaceCreateInfo.window    =   window;
        error                       =   vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
        VkHeadlessSurfaceCreateInfoEXT surfaceCreateInfo = {};
        surfaceCreateInfo.sType             =   VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
        auto    fpCreateHeadlessSurfaceEXT  =   (PFN_vkCreateHeadlessSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");
        if (!fpCreateHeadlessSurfaceEXT)
        {
            LOG_ERR("Could not fetch function pointer for the headless extension!");
            return  false;
        }
        error                               =   fpCreateHeadlessSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
        VkScreenSurfaceCreateInfoQNX surfaceCreateInfo = {};
        surfaceCreateInfo.sType     =   VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
        surfaceCreateInfo.pNext     =   NULL;
        surfaceCreateInfo.flags     =   0;
        surfaceCreateInfo.context   =   screen_context;
        surfaceCreateInfo.window    =   screen_window;
        error                       =   vkCreateScreenSurfaceQNX(instance, &surfaceCreateInfo, NULL, &surface);
#endif
        if (error   !=  VK_SUCCESS) 
            return  ;

        // Get available queue family properties
        uint32_t    queueCount  =   0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
        assert(queueCount >= 1);
        std::vector<VkQueueFamilyProperties> queueProps(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

        // Iterate over each queue to learn whether it supports presenting:
        // Find a queue with present support
        // Will be used to present the swap chain images to the windowing system
        std::vector<VkBool32> supportsPresent(queueCount);
        for (uint32_t i = 0; i < queueCount; i++) 
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
        }

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        uint32_t graphicsQueueNodeIndex =   UINT32_MAX;
        uint32_t presentQueueNodeIndex  =   UINT32_MAX;
        for (uint32_t i = 0; i < queueCount; i++) 
        {
            if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)  
            {
                if (graphicsQueueNodeIndex == UINT32_MAX) 
                {
                    graphicsQueueNodeIndex  =   i;
                }
                if (supportsPresent[i] == VK_TRUE) 
                {
                    graphicsQueueNodeIndex  =   i;
                    presentQueueNodeIndex   =   i;
                    break;
                }
            }
        }
        if (presentQueueNodeIndex == UINT32_MAX) 
        {	
            // If there's no queue that supports both present and graphics
            // try to find a separate present queue
            for (uint32_t i = 0; i < queueCount; ++i) 
            {
                if (supportsPresent[i] == VK_TRUE) 
                {
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }

        // Exit if either a graphics or a presenting queue hasn't been found
        if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)  
        {
            LOG_ERR("Could not find a graphics and/or presenting queue!");
        }
        if (graphicsQueueNodeIndex != presentQueueNodeIndex) 
        {
            LOG_ERR("Separate graphics and presenting queues are not supported yet!");
        }
        _cInfo._queueFamilyIndex    =   graphicsQueueNodeIndex;
        // Get list of supported surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
        assert(formatCount > 0);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));
        // We want to get a format that best suits our needs, so we try to get one from a set of preferred formats
        // Initialize the format to the first one returned by the implementation in case we can't find one of the preffered formats
        VkSurfaceFormatKHR  selectedFormat          =   surfaceFormats[0];
        /// <summary>
        /// �����ĸ�ʽ����,����˳�����ȼ�����ƥ��
        /// </summary>
        VkFormats           preferredImageFormats   = 
        { 
            VK_FORMAT_R8G8B8A8_UNORM, 
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_A8B8G8R8_UNORM_PACK32 
        };
        for (auto& fmt : preferredImageFormats) 
        {
            auto    itr =   std::find_if(surfaceFormats.begin(), surfaceFormats.end(),[&](const VkSurfaceFormatKHR& val)->bool
            {
                return  val.format == fmt;
            }); 
            if (itr != surfaceFormats.end())
            {
                _colorFormat    =   fmt;
                _colorSpace     =   (*itr).colorSpace;
                break;
            }
        }
    }
    
    bool        VKSwapchain::acquireNextImage(uint64 timeout, Semaphore sem, Fence fence,uint& imageIndex)
    {
        if (!isValid())
            return  false;
        auto&   vkDevice        =   (VKDevice&)(_ctx.device());
        //auto    physicalDevice  =   vkDevice.physicalDevice();
        auto    device          =   vkDevice.logicalDevice();
        //auto    instance        =   (VkInstance)vkDevice.renderSystem().native();
        auto    nativeSem       =   sem ? (VkSemaphore)sem->native() : nullptr;
        auto    nativeFence     =   fence ? (VkFence)fence->native() : nullptr;
        auto    result          =   vkAcquireNextImageKHR(device, _native, timeout, nativeSem, nativeFence, &imageIndex);
        assert (result == VK_SUCCESS);
        if (result == VK_SUCCESS)
            return  true;
        else
            return    false;
    }
    GImgViews   VKSwapchain::imageViews() const
    {
        return  _imageViews;
    }
    bool        VKSwapchain::create(const VKSwapchain::CreateInfo& info)
    {
        _cInfo =   info;

        RECT    rect;
        GetClientRect((HWND)info._window,&rect);
        initSurface(info._appInst,info._window);
        create(_cInfo._width,_cInfo._height,true,false);
        return  true;
    }

    bool        VKSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        //auto        physicalDevice  =   vkDevice.physicalDevice();

        VkSemaphore         nativeSems[1]   =   {};
                            nativeSems[0]   =   pInfo._sem   ? (VkSemaphore)(pInfo._sem->native()) : nullptr;
        VkQueue             nativeQueue     =   pInfo._queue ? (VkQueue)(pInfo._queue->native()) : nullptr;
        VkPresentInfoKHR    presentInfo{};
        presentInfo.sType               =   VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount  =   1;
        presentInfo.pWaitSemaphores     =   nativeSems;
        presentInfo.swapchainCount      =   1;
        presentInfo.pSwapchains         =   &_native;
        presentInfo.pImageIndices       =   &pInfo._imageIndex;
        auto    result  =   vkQueuePresentKHR(nativeQueue, &presentInfo);
        return  result == VK_SUCCESS;
    }
    void        VKSwapchain::create(uint32_t& width, uint32_t& height, bool vsync, bool /*fullscreen*/)
    {
        auto&       vkDevice        =   (VKDevice&)(_ctx.device());
        auto        physicalDevice  =   vkDevice.physicalDevice();
        auto        device          =   vkDevice.logicalDevice();
        auto        instance        =   (VkInstance)vkDevice.renderSystem().native();
        assert(physicalDevice);
        assert(device);
        assert(instance);

        /// Store the current swap chain handle so we can use it later on to ease up recreation
        VkSwapchainKHR              oldSwapchain            =   _native;
        VkSurfaceCapabilitiesKHR    surfaceCaps             =   {};
        VkExtent2D                  swapchainExtent         =   {};
        uint32_t                    presentModeCount        =   {};
        VkPresentModeKHR            swapchainPresentMode    =    VK_PRESENT_MODE_FIFO_KHR;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

        if (surfaceCaps.currentExtent.width == (uint32_t)-1) 
        {
            swapchainExtent.width   =    width;
            swapchainExtent.height  =    height;
        } 
        else 
        {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent =    surfaceCaps.currentExtent;
            width           =    surfaceCaps.currentExtent.width;
            height          =    surfaceCaps.currentExtent.height;
        }
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
        assert(presentModeCount > 0);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

        ///    The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
        ///    This mode waits for the vertical blank ("v-sync")
        if (!vsync)
        {
            for (size_t i = 0; i < presentModeCount; i++)
            {
                if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }
        }
        // Determine the number of images
        uint32_t desiredNumberOfSwapchainImages = surfaceCaps.minImageCount + 1;
        if ((surfaceCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCaps.maxImageCount)) 
        {
            desiredNumberOfSwapchainImages = surfaceCaps.maxImageCount;
        }
        // Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            // We prefer a non-rotated transform
            preTransform    =  VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } 
        else 
        {
            preTransform    =   surfaceCaps.currentTransform;
        }
        // Find a supported composite alpha format (not all devices support alpha opaque)
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // Simply select the first composite alpha format available
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = 
        {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR           ,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR   ,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR  ,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR          ,
        };
        for (auto& compositeAlphaFlag : compositeAlphaFlags) 
        {
            if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) 
            {
                compositeAlpha = compositeAlphaFlag;
                break;
            };
        }

        VkSwapchainCreateInfoKHR swapchainCI    =    {};
        swapchainCI.sType                   =   VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface                 =   surface;
        swapchainCI.minImageCount           =   desiredNumberOfSwapchainImages;
        swapchainCI.imageFormat             =   _colorFormat;
        swapchainCI.imageColorSpace         =   _colorSpace,
        swapchainCI.imageExtent             =   { swapchainExtent.width, swapchainExtent.height };
        swapchainCI.imageArrayLayers        =   1;
        swapchainCI.imageUsage              =   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.imageSharingMode        =   VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount   =   0;
        swapchainCI.preTransform            =   (VkSurfaceTransformFlagBitsKHR)preTransform;
        swapchainCI.compositeAlpha          =   compositeAlpha;
        swapchainCI.presentMode             =   swapchainPresentMode;
        // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
        swapchainCI.clipped                =   VK_TRUE;
        // Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
        swapchainCI.oldSwapchain            =   oldSwapchain;
            
        // Enable transfer source on swap chain images if supported
        if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) 
        {
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        // Enable transfer destination on swap chain images if supported
        if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) 
        {
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &_native));

        // If an existing swap chain is re-created, destroy the old swap chain and the ressources owned by the application (image views, images are owned by the swap chain)
        if (oldSwapchain != VK_NULL_HANDLE) 
        { 
            _imageViews.clear();
            vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }
        // Get the (new) swap chain images
        VkImages    images;
        uint        imageCount;
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, _native, &imageCount, nullptr));
        images.resize(imageCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, _native, &imageCount, images.data()));

        _images.clear();
        _images.resize(imageCount);
        _imageViews.resize(imageCount);
        for (uint i = 0; i < imageCount; i++)
        {
            _images[i]  =   new VKGImage(_ctx,images[i],true);
        }
        for (size_t i = 0; i < images.size(); i++)
        {
            VkImageViewCreateInfo colorAttachmentView   =   {};
            colorAttachmentView.sType                           =   VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            colorAttachmentView.image                           =   images[i],
            colorAttachmentView.viewType                        =   VK_IMAGE_VIEW_TYPE_2D,
            colorAttachmentView.format                          =   _colorFormat,
            colorAttachmentView.components                      =   { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            colorAttachmentView.subresourceRange                =   {};
                    
            colorAttachmentView.subresourceRange.aspectMask     =   VK_IMAGE_ASPECT_COLOR_BIT,
            colorAttachmentView.subresourceRange.baseMipLevel   =   0;
            colorAttachmentView.subresourceRange.levelCount     =   1;
            colorAttachmentView.subresourceRange.baseArrayLayer =   0;
            colorAttachmentView.subresourceRange.layerCount     =   1;
            VkImageView     vkImgView                           =   nullptr;
            VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &vkImgView));
            if (vkImgView)
            {
                auto    vkView  =   new VKGImageView(_ctx,vkImgView,false);
                _imageViews[i]  =   vkView;
                vkView->setImage(_images[i]);
            }
        }
    }

    void        VKSwapchain::cleanup()
    {
        auto&   vkDevice        =   (VKDevice&)_ctx.device();
        auto    device          =   vkDevice.logicalDevice();
        auto    instance        =   (VkInstance)vkDevice.renderSystem().native();
        if (_native != VK_NULL_HANDLE) 
        {
            _imageViews.clear();
            vkDestroySwapchainKHR(device, _native, nullptr);
        }
        _images.clear();
        if (surface != VK_NULL_HANDLE) 
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
        surface     =    VK_NULL_HANDLE;
        _native     =    VK_NULL_HANDLE;
    }
    
}
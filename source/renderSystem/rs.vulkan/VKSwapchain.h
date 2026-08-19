#pragma     once

#include    "graphic/FESwapchain.h"
#include    "graphic/FEDevice.h"
#include    "FEVulkan.h"
#include    "VKGImage.h"
#include    "VKGImageView.h"

namespace   FE
{
    class   VKSwapchain :public TRSObject<VkSwapchainKHR,FESwapchain>
    {
    public:
        using   VkImages        =   std::vector<VkImage>;
        using   VkImageViews    =   std::vector<VkImageView>;
        using   VkFormats       =   std::vector<VkFormat>;
    public:
        VKSwapchain(FEContext& ctx)
            :TRSObject<VkSwapchainKHR,FESwapchain>(ctx)
        {}
        VKSwapchain(const VKSwapchain& other)
            :TRSObject<VkSwapchainKHR,FESwapchain>(other)
        {}
        virtual ~VKSwapchain();
    public:
        virtual Frame       acquireNextFrame(uint64 timeout)  override;
        virtual bool        create(const CreateInfo& info)  override;
        virtual bool        queuePresent(const PresentInfo& pInfo) override;
        virtual FEFormat    colorFormat() const 
        {
            if (_colorFormat == VK_FORMAT_R8G8B8A8_UNORM)
                return  FMT_R8G8B8A8_UNORM;
            else if (_colorFormat == VK_FORMAT_B8G8R8A8_UNORM)
                return  FMT_B8G8R8A8_UNORM;
            else
                return  FMT_R8G8B8A8_UNORM;
        }
        
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        void    initSurface(void* platformHandle, void* platformWindow);
    #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        void    initSurface(ANativeWindow* window);
    #elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
        void    initSurface(IDirectFB* dfb, IDirectFBSurface* window);
    #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        void    initSurface(wl_display* display, wl_surface* window);
    #elif defined(VK_USE_PLATFORM_XCB_KHR)
        void    initSurface(xcb_connection_t* connection, xcb_window_t window);
    #elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
        void    initSurface(void* view);
    #elif defined(VK_USE_PLATFORM_METAL_EXT)
        void    initSurface(CAMetalLayer* metalLayer);
    #elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
        void    initSurface(uint32_t width, uint32_t height);
    #if defined(_DIRECT2DISPLAY)
        void    createDirect2DisplaySurface(uint32_t width, uint32_t height);
    #endif
    #elif defined(VK_USE_PLATFORM_SCREEN_QNX)
        void    initSurface(screen_context_t screen_context, screen_window_t screen_window);
    #endif
        void    create(uint32_t& width, uint32_t& height, bool vsync, bool fullscreen);
        void    cleanup();
    protected:
        VkFormat            _colorFormat        {};
        VkColorSpaceKHR     _colorSpace         {};
        VkSurfaceKHR        surface             { VK_NULL_HANDLE };
        GImages             _images             {};  
        Frames              _frames             {};
        size_t              _curFrame           {};
        GImgViews           _imageViews         {};

    };
}
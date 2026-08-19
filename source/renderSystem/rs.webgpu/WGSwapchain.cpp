#include    "WGSwapchain.h"
#include    "WGDevice.h"
#include    "WGRenderSystem.h"
#include    "WGGImage.h"
#include    "WGGImageView.h"
#include    <windows.h>

namespace   FE
{
    WGSwapchain::~WGSwapchain()
    {
        _frame  =   nullptr;
        if (_native)
        {
            wgpuSurfaceRelease(_native);
            _native =   nullptr;
        }
    }

    void    WGSwapchain::initSurface(void* platformHandle,void* platformWindow)
    {

        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain =   nullptr;

        WGPUSurfaceSourceWindowsHWND windowsDesc = {};
        windowsDesc.chain.sType =   WGPUSType_SurfaceSourceWindowsHWND;
        windowsDesc.hinstance   =   (HINSTANCE)platformHandle;
        windowsDesc.hwnd        =   (HWND)platformWindow;

        surfaceDesc.nextInChain =   &windowsDesc.chain;

        auto& wgDevice  =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto& renderSys =   wgDevice.renderSystem();
        _native         =   wgpuInstanceCreateSurface(renderSys.instance(),&surfaceDesc);
    }

    bool    WGSwapchain::create(const CreateInfo& info)
    {
        _cInfo =   info;

        if (!_native && info._window)
        {
            initSurface(info._appInst,info._window);
        }

        if (!_native)
            return false;
        auto&       wgDevice        =   (WGDevice&)(_ctx.device());
        WGPUAdapter physicalDevice  =   (WGPUAdapter)wgDevice.physicalDevice();

        WGPUSurfaceCapabilities     surfaceCapabilities = {0};
        wgpuSurfaceGetCapabilities(_native, (physicalDevice), &surfaceCapabilities);

        WGPUSurfaceConfiguration config = {};
        config.nextInChain      =   nullptr;
        config.device           =   wgDevice.device();
        config.format           =   _colorFormat;
        config.usage            =   WGPUTextureUsage_RenderAttachment;
        config.width            =   info._width;
        config.height           =   info._height;
        config.alphaMode        =   surfaceCapabilities.alphaModes ?  surfaceCapabilities.alphaModes[0]: WGPUCompositeAlphaMode_Opaque;
        config.presentMode      =   WGPUPresentMode_Fifo;

        wgpuSurfaceConfigure(_native,&config);
        return true;
    }

    Frame   WGSwapchain::acquireNextFrame(uint64 timeout)
    {
        (void)timeout;
        if (!_native)
            return nullptr;
        if (_frame == nullptr)
            _frame  =   new FEFrame(_ctx);
        WGPUSurfaceTexture  surfaceTexture  =   {};
        wgpuSurfaceGetCurrentTexture(_native,&surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal ||
            surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        {
            WGPUTextureView wGWiew    =   wgpuTextureCreateView(surfaceTexture.texture, nullptr);
            if (_frame->_imageViewer == nullptr)
            {
                _frame->_imageViewer    =   new WGGImageView(_ctx,wGWiew);
            }
            else
            {
                _frame->_imageViewer->destroy(); 
                auto    view    =   _frame->_imageViewer->as<WGGImageView>();
                if (view)
                {
                    view->attach(wGWiew);
                }
            }
        }
        return _frame;
    }

    bool    WGSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        (void)pInfo;
        if (!_native)
            return false;

        wgpuSurfacePresent(_native);
        return true;
    }

    FEFormat WGSwapchain::colorFormat() const
    {
        if (_colorFormat == WGPUTextureFormat_BGRA8Unorm)
            return FMT_B8G8R8A8_UNORM;
        else if (_colorFormat == WGPUTextureFormat_RGBA8Unorm)
            return FMT_R8G8B8A8_UNORM;
        else
            return FMT_R8G8B8A8_UNORM;
    }
}
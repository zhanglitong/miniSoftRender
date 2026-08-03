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
        if (_currentTexture)
        {
            wgpuTextureRelease(_currentTexture);
            _currentTexture =   nullptr;
        }
        if (_native)
        {
            wgpuSurfaceRelease(_native);
            _native =   nullptr;
        }
        _images.clear();
        _imageViews.clear();
    }

    void WGSwapchain::initSurface(void* platformHandle,void* platformWindow)
    {
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain =   nullptr;

        WGPUSurfaceSourceWindowsHWND windowsDesc = {};
        windowsDesc.chain.sType =   WGPUSType_SurfaceSourceWindowsHWND;
        windowsDesc.hinstance =   (HINSTANCE)platformHandle;
        windowsDesc.hwnd =   (HWND)platformWindow;

        surfaceDesc.nextInChain =   &windowsDesc.chain;

        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto& renderSys = wgDevice.renderSystem();
        _native =   wgpuInstanceCreateSurface(renderSys.instance(),&surfaceDesc);
    }

    bool WGSwapchain::create(const CreateInfo& info)
    {
        _cInfo =   info;

        if (!_native && info._window)
        {
            initSurface(info._appInst,info._window);
        }

        if (!_native)
            return false;

        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        WGPUSurfaceConfiguration config = {};
        config.nextInChain =   nullptr;
        config.device =   wgDevice.device();
        config.format =   _colorFormat;
        config.usage =   WGPUTextureUsage_RenderAttachment;
        config.width =   info._width;
        config.height =   info._height;
        config.viewFormatCount =   0;
        config.viewFormats =   nullptr;
        config.alphaMode =   WGPUCompositeAlphaMode_Opaque;
        config.presentMode =   WGPUPresentMode_Fifo;

        wgpuSurfaceConfigure(_native,&config);
        return true;
    }

    bool WGSwapchain::acquireNextImage(uint64 timeout,Semaphore sem,Fence fence,uint& imageIndex)
    {
        if (!_native)
            return false;

        if (_currentTexture)
        {
            wgpuTextureRelease(_currentTexture);
            _currentTexture =   nullptr;
        }

        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(_native,&surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal ||
            surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        {
            _currentTexture =   surfaceTexture.texture;
            _currentImageIndex =   0;
            imageIndex =   _currentImageIndex;
            return true;
        }

        return false;
    }

    GImgViews WGSwapchain::imageViews() const
    {
        return _imageViews;
    }

    bool WGSwapchain::queuePresent(const PresentInfo& pInfo)
    {
        if (!_native)
            return false;

        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
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
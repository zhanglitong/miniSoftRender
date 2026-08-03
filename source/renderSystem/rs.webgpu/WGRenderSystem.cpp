#include    "WGRenderSystem.h"
#include    "WGDevice.h"

namespace   FE
{
    static void log_callback(WGPULogLevel level,WGPUStringView message,void* userdata)
    {
        const char* level_str = "";
        switch (level)
        {
        case WGPULogLevel_Error:
            level_str = "error";
            break;
        case WGPULogLevel_Warn:
            level_str = "warn";
            break;
        case WGPULogLevel_Info:
            level_str = "info";
            break;
        case WGPULogLevel_Debug:
            level_str = "debug";
            break;
        case WGPULogLevel_Trace:
            level_str = "trace";
            break;
        default:
            level_str = "unknown_level";
        }

        WGRenderSystem* pRenderSys = (WGRenderSystem*)userdata;
        if (pRenderSys)
        {
            pRenderSys->debug(message.data);
        }
    }

    WGRenderSystem::WGRenderSystem(FEContext& ctx)
        :FERenderSystem(ctx)
    {
        WGPUInstanceDescriptor instanceDesc = {};
        instanceDesc.nextInChain =   nullptr;

        _native =   wgpuCreateInstance(&instanceDesc);
        if (_native)
        {
            wgpuSetLogCallback(log_callback,this);
            wgpuSetLogLevel(WGPULogLevel_Warn);
        }
    }

    WGRenderSystem::~WGRenderSystem()
    {
        destroy();
    }

    FEResult WGRenderSystem::create()
    {
        LOG_DBG("WGRenderSystem.create");
        if (!_native)
            return FEResult::ER_FAILED;

        _gpus =   gpuListImpl();
        return FEResult::ER_SUCCESS;
    }

    Device WGRenderSystem::createDevice()
    {
        LOG_DBG("WGRenderSystem.createDevice");
        return new WGDevice(_ctx,*this);
    }

    void WGRenderSystem::debug(const char* msg)
    {
        LOG_DBG("wgpu: %s",msg);
    }

    void WGRenderSystem::destroy()
    {
        if (_adapter)
        {
            wgpuAdapterRelease(_adapter);
            _adapter =   nullptr;
        }
        if (_native)
        {
            wgpuInstanceRelease(_native);
            _native =   nullptr;
        }
    }

    struct WGPUAdapterResult
    {
        WGPUAdapter _adapter;
    };

    static void handle_request_adapter(WGPURequestAdapterStatus status,WGPUAdapter adapter,WGPUStringView message,void* userdata1,void* userdata2)
    {
        (void)userdata2;
        if (status == WGPURequestAdapterStatus_Success)
        {
            WGPUAdapterResult* pAResult = (WGPUAdapterResult*)userdata1;
            pAResult->_adapter = adapter;
        }
        else
        {
            printf(" request_adapter status=%#.8x message=%.*s\n",status,(int)message.length,message.data);
        }
    }

    GPUs WGRenderSystem::gpuListImpl() const
    {
        if (!_native)
            return {};

        GPUs gpus;

        WGPURequestAdapterOptions adapterOpts = {};
        adapterOpts.nextInChain =   nullptr;
        adapterOpts.compatibleSurface =   nullptr;
        adapterOpts.powerPreference =   WGPUPowerPreference_HighPerformance;
        adapterOpts.forceFallbackAdapter =   false;

        WGPURequestAdapterCallbackInfo callbackInfo = {};
        callbackInfo.nextInChain =   nullptr;
        callbackInfo.callback =   handle_request_adapter;

        WGPUAdapterResult result;
        callbackInfo.userdata1 =   &result;
        callbackInfo.userdata2 =   nullptr;

        wgpuInstanceRequestAdapter(_native,&adapterOpts,callbackInfo);

        if (result._adapter)
        {
            _adapter =   result._adapter;

            GPU gpuInf = {};
            WGPUAdapterInfo info = {};
            wgpuAdapterGetInfo(result._adapter,&info);

            switch (info.adapterType)
            {
            case WGPUAdapterType_DiscreteGPU:
                gpuInf.type =   DEV_TYPE_DISCRETE_GPU;
                break;
            case WGPUAdapterType_IntegratedGPU:
                gpuInf.type =   DEV_TYPE_INTEGRATED_GPU;
                break;
            case WGPUAdapterType_CPU:
                gpuInf.type =   DEV_TYPE_CPU;
                break;
            default:
                gpuInf.type =   DEV_TYPE_OTHER;
            }

            gpuInf.name =   info.vendor.data ? info.vendor.data : "Unknown GPU";
            gpuInf.gpuId._32[0] =   info.vendorID;
            gpuInf.gpuId._32[1] =   info.deviceID;
            gpuInf.gpuId._32[2] =   info.backendType;
            gpuInf.gpuId._32[3] =   info.subgroupMaxSize;
            gpuInf.gpu =   result._adapter;

            wgpuAdapterInfoFreeMembers(info);

            gpus.push_back(gpuInf);
        }

        return gpus;
    }
}
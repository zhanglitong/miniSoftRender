#include    "WGRenderSystem.h"
#include    "WGDevice.h"

namespace   FE
{
    static void log_callback(WGPULogLevel level,WGPUStringView message,void* userdata)
    {
        WGRenderSystem* pRenderSys = (WGRenderSystem*)userdata;
        if (pRenderSys && message.length)
        {
            pRenderSys->debug(level,message.data);
        }
    }

    WGRenderSystem::WGRenderSystem(FEContext& ctx)
        :FERenderSystem(ctx)
    {
        WGPUInstanceFeatureName instanceFeatures[] = { WGPUInstanceFeatureName_ShaderSourceSPIRV };

        WGPUInstanceDescriptor instanceDesc = {};
        instanceDesc.nextInChain            =   nullptr;
        instanceDesc.requiredFeatureCount   =   1;
        instanceDesc.requiredFeatures       =   instanceFeatures;

        _native =   wgpuCreateInstance(&instanceDesc);
        if (_native)
        {
            wgpuSetLogCallback(log_callback,this);
            wgpuSetLogLevel(WGPULogLevel_Debug);
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

    void WGRenderSystem::debug(WGPULogLevel level,const char* msg)
    {
        switch (level)
        {
        case WGPULogLevel_Error:
            LOG_ERR("wgpu: %s",msg);
            break;
        case WGPULogLevel_Warn:
            LOG_WAR("wgpu: %s",msg);
            break;
        case WGPULogLevel_Info:
            LOG_INF("wgpu: %s",msg);
            break;
        case WGPULogLevel_Debug:
            LOG_DBG("wgpu: %s",msg);
            break;
        case WGPULogLevel_Trace:
            LOG_DBG("wgpu: %s",msg);
            break;
        }
    }

    void WGRenderSystem::destroy()
    {
        for (auto& gpu : _gpus)
        {
            if (gpu.gpu)
            {
                wgpuAdapterRelease(static_cast<WGPUAdapter>(gpu.gpu));
            }
        }
        _gpus.clear();
        if (_native)
        {
            wgpuInstanceRelease(_native);
            _native =   nullptr;
        }
    }


    GPUs WGRenderSystem::gpuListImpl() const
    {
        if (!_native)
            return {};

        GPUs            gpus;
        WGPUAdapter     defaultAdapter  =   nullptr;
        bool            foundDiscrete   =   false;

        WGPUInstanceEnumerateAdapterOptions enumOpts = {};
        enumOpts.nextInChain =   nullptr;
        enumOpts.backends    =   WGPUInstanceBackend_Vulkan;

        size_t  adapterCnt  =   wgpuInstanceEnumerateAdapters(_native, &enumOpts, nullptr);
        auto    adapters    =   new WGPUAdapter[adapterCnt];
        assert(adapters);
        wgpuInstanceEnumerateAdapters(_native, &enumOpts, adapters);

        for (size_t i = 0; i < adapterCnt; i++) 
        {
            GPU             gpuInf  =   {};
            WGPUAdapter     adapter =   adapters[i];
            WGPUAdapterInfo info    =   {0};
            wgpuAdapterGetInfo(adapter, &info);

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
            String  btype   =   "Unknown GPU";
            switch (info.backendType)
            {
            case WGPUBackendType_Null    :  btype   =   "Null/";        break;
            case WGPUBackendType_WebGPU  :  btype   =   "WebGPU/";      break;
            case WGPUBackendType_D3D11   :  btype   =   "D3D11/";       break;
            case WGPUBackendType_D3D12   :  btype   =   "D3D12/";       break;
            case WGPUBackendType_Metal   :  btype   =   "Metal/";       break;
            case WGPUBackendType_Vulkan  :  btype   =   "Vulkan/";      break;
            case WGPUBackendType_OpenGL  :  btype   =   "OpenGL/";      break;
            case WGPUBackendType_OpenGLES:  btype   =   "OpenGLES/";    break;
            default:
                break;
            }
            if (info.device.length > 0 && info.device.data)
                gpuInf.name =   btype + info.device.data;
            else
                gpuInf.name =   btype;

            gpuInf.gpuId._32[0] =   info.vendorID;
            gpuInf.gpuId._32[1] =   info.deviceID;
            gpuInf.gpuId._32[2] =   info.backendType;
            gpuInf.gpuId._32[3] =   info.adapterType;
            gpuInf.gpu          =   adapter;

            wgpuAdapterInfoFreeMembers(info);

            gpus.push_back(gpuInf);

            // 优先选择独立显卡作为默认 adapter，否则选第一个
            if (!foundDiscrete)
            {
                if (gpuInf.type == DEV_TYPE_DISCRETE_GPU)
                {
                    defaultAdapter  =   adapter;
                    foundDiscrete   =   true;
                }
                else if (defaultAdapter == nullptr)
                {
                    defaultAdapter  =   adapter;
                }
            }
        }
        delete  []adapters;
        return gpus;
    }
}
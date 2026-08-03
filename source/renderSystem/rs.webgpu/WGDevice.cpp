#include    "WGDevice.h"
#include    "WGRenderSystem.h"
#include    "WGCmdBuffer.h"
#include    "WGQueue.h"
#include    "WGFrameBuffer.h"
#include    "WGShader.h"
#include    "WGRenderPass.h"
#include    "WGPipeline.h"
#include    "WGSwapchain.h"
#include    "WGGImage.h"
#include    "WGGImageView.h"
#include    "WGCmdPool.h"
#include    "WGDSet.h"
#include    "WGDSetLayout.h"
#include    "WGDSetPool.h"
#include    "WGFence.h"
#include    "WGSemaphore.h"
#include    "WGGPUBuffer.h"

namespace   FE
{
    WGDevice::~WGDevice()
    {
        if (_destroyNotify) _destroyNotify(*this);

        _graphicPool =   nullptr;
        _computePool =   nullptr;
        _transferPool =   nullptr;

        _queueGraphic =   nullptr;
        _queueCompute =   nullptr;
        _queueTransfer =   nullptr;

        if (_nativeDevice)
        {
            wgpuDeviceDestroy(_nativeDevice);
            _nativeDevice =   nullptr;
        }
    }

    FEResult WGDevice::create(const CreateInfo& cInfo)
    {
        auto gpu = _renderSys.gpu(cInfo.deviceId);
        assert(gpu.isValid());
        if (!gpu.isValid())
            return FEResult::ER_FAILED;
        _cInfo =   cInfo;
        LOG_INF("WGDevice.create()");
        LOG_INF("GPU.name = %s",gpu.name.c_str());
        LOG_INF("GPU.type = %s",FERenderSystem::nameOf(gpu.type));

        _gpuInfo =   gpu;

        auto adapter = _renderSys.adapter();
        if (!adapter)
            return FEResult::ER_FAILED;

        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain =   nullptr;
        deviceDesc.label = { "Device",6 };
        deviceDesc.requiredFeatureCount =   0;
        deviceDesc.requiredFeatures =   nullptr;
        deviceDesc.requiredLimits =   nullptr;
        deviceDesc.defaultQueue = {};
        deviceDesc.defaultQueue.nextInChain =   nullptr;
        deviceDesc.defaultQueue.label = { "Default Queue",13 };

        struct DeviceRequestResult
        {
            WGPUDevice device;
        };

        auto deviceCallback = [](WGPURequestDeviceStatus status,WGPUDevice device,WGPUStringView message,void* userdata1,void* userdata2) 
        {
            (void)message;
            (void)userdata1;
            (void)userdata2;
            if (status == WGPURequestDeviceStatus_Success)
            {
                auto* result = static_cast<DeviceRequestResult*>(userdata1);
                result->device = device;
            }
        };

        DeviceRequestResult deviceResult;
        WGPURequestDeviceCallbackInfo callbackInfo = {};
        callbackInfo.nextInChain    =   nullptr;
        callbackInfo.callback       =   deviceCallback;
        callbackInfo.userdata1      =   &deviceResult;
        callbackInfo.userdata2      =   nullptr;

        wgpuAdapterRequestDevice(adapter,&deviceDesc,callbackInfo);

        _nativeDevice =   deviceResult.device;
        if (!_nativeDevice)
            return FEResult::ER_FAILED;

        _queue =   wgpuDeviceGetQueue(_nativeDevice);

        _graphicPool    =   createCmdPool();
        _computePool    =   _graphicPool;
        _transferPool   =   _graphicPool;

        if (_createNotify) _createNotify(*this);

        return FEResult::ER_SUCCESS;
    }

    void WGDevice::waitIdle()
    {
        if (_nativeDevice)
        {
            wgpuDevicePoll(_nativeDevice,true,nullptr);
        }
    }

    Fence WGDevice::createFence()
    {
        LOG_DBG("WGDevice.createFence");
        return new WGFence(_ctx);
    }

    Semaphore WGDevice::createSemaphore()
    {
        LOG_DBG("WGDevice.createSemaphore");
        return new WGSemaphore(_ctx);
    }

    CMDPool WGDevice::createCmdPool()
    {
        LOG_DBG("WGDevice.createCmdPool");
        return new WGCmdPool(_ctx);
    }

    FBOPtr WGDevice::createFrameBuffer()
    {
        LOG_DBG("WGDevice.createFrameBuffer");
        return new WGFrameBuffer(_ctx);
    }

    Shader WGDevice::createShader()
    {
        LOG_DBG("WGDevice.createShader");
        return new WGShader(_ctx);
    }

    RenderPass WGDevice::createRenderPass()
    {
        LOG_DBG("WGDevice.createRenderPass");
        return new WGRenderPass(_ctx);
    }

    GPipeline WGDevice::createGPipeline()
    {
        LOG_DBG("WGDevice.createGPipeline");
        return new WGPipeline(_ctx);
    }

    Swapchain WGDevice::createSwapchain()
    {
        LOG_DBG("WGDevice.createSwapchain");
        return new WGSwapchain(_ctx);
    }

    GImage WGDevice::createGImage()
    {
        LOG_DBG("WGDevice.createGImage");
        return new WGGImage(_ctx);
    }

    DSetLayout WGDevice::createDSLayout()
    {
        LOG_DBG("WGDevice.createDSLayout");
        return new WGDSetLayout(_ctx);
    }

    DSetLayout WGDevice::createDSLayoutFromShaders(const std::vector<Shader>& shaders)
    {
        std::vector<FEDSetBinding> allBindings;
        std::map<uint32_t,FEDSetBinding> bindingMap;

        for (const auto& shader : shaders)
        {
            if (!shader) continue;
            const auto* pShader = shader.get();
            const auto* wgShader = static_cast<const WGShader*>(pShader);
            const auto& reflectData = wgShader->reflectData();

            for (const auto& binding : reflectData._bindings)
            {
                uint32_t key = binding._binding;
                auto it = bindingMap.find(key);
                if (it != bindingMap.end())
                {
                    it->second._stageFlags = (it->second._stageFlags | binding._stageFlags);
                }
                else
                {
                    FEDSetBinding setBinding;
                    setBinding._binding = binding._binding;
                    setBinding._descriptorType = binding._descriptorType;
                    setBinding._stageFlags = binding._stageFlags;
                    setBinding._name = binding._name;
                    bindingMap[key] = setBinding;
                }
            }
        }

        for (const auto& pair : bindingMap)
        {
            allBindings.push_back(pair.second);
        }

        std::sort(allBindings.begin(),allBindings.end(),[](const FEDSetBinding& a,const FEDSetBinding& b) {
            return a._binding < b._binding;
        });

        if (allBindings.empty())
            return nullptr;

        FEDSetLayout::CreateInfo info;
        info._bindings = allBindings;

        auto layout = new WGDSetLayout(_ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }

    DSetPool WGDevice::createDSetPool()
    {
        LOG_DBG("WGDevice.createDSetPool");
        return new WGDSetPool(_ctx);
    }

    DSet WGDevice::createDSet()
    {
        LOG_DBG("WGDevice.createDSet");
        return new WGDSet(_ctx);
    }

    VBO WGDevice::createVBO()
    {
        LOG_DBG("WGDevice.createVBO");
        return new WGGPUBuffer(_ctx,BufferUsage::VERTEX_BUFFER_BIT);
    }

    IBO WGDevice::createIBO()
    {
        LOG_DBG("WGDevice.createIBO");
        return new WGGPUBuffer(_ctx,BufferUsage::INDEX_BUFFER_BIT);
    }

    UBO WGDevice::createUBO()
    {
        LOG_DBG("WGDevice.createUBO");
        return new WGGPUBuffer(_ctx,BufferUsage::UNIFORM_BUFFER_BIT);
    }

    ITO WGDevice::createITO()
    {
        LOG_DBG("WGDevice.createITO");
        return new WGGPUBuffer(_ctx,BufferUsage::INDIRECT_BUFFER_BIT);
    }

    SBO WGDevice::createSBO()
    {
        LOG_DBG("WGDevice.createSBO");
        return new WGGPUBuffer(_ctx,BufferUsage::STORAGE_BUFFER_BIT);
    }

    GPUBuffer WGDevice::createFromBuffer(const void* pData,uint64 length)
    {
        assert(length != 0 && pData != nullptr);
        if (length == 0 || pData == nullptr)
            return nullptr;
        LOG_DBG("WGDevice.createFromBuffer");
        auto object = new WGGPUBuffer(_ctx,BufferUsage::TRANSFER_SRC_BIT);
        FEGPUBuffer::CreateInfo cInfo;
        cInfo._bufUsages =   TRANSFER_SRC_BIT;
        cInfo._memUsages =   HOST_VISIBLE_BIT;
        cInfo._length =   length;
        if (object->create(cInfo))
        {
            object->update(pData,length,0);
            return object;
        }
        LOG_ERR("WGGPUBuffer.create return false");
        return nullptr;
    }

    Queue WGDevice::queueGraphic()
    {
        if (_queueGraphic == nullptr)
        {
            _queueGraphic =   new WGQueue(_ctx,_queue);
        }
        return _queueGraphic;
    }

    Queue WGDevice::queueCompute()
    {
        if (_queueCompute == nullptr)
        {
            _queueCompute =   new WGQueue(_ctx,_queue);
        }
        return _queueCompute;
    }

    Queue WGDevice::queueTransfer()
    {
        if (_queueTransfer == nullptr)
        {
            _queueTransfer =   new WGQueue(_ctx,_queue);
        }
        return _queueTransfer;
    }

    WGPUTextureFormat WGDevice::getWGPUTextureFormat(FEFormat format) const
    {
        switch (format)
        {
        case FMT_R8G8B8A8_UNORM:        return WGPUTextureFormat_RGBA8Unorm;
        case FMT_R8G8B8A8_UINT:         return WGPUTextureFormat_RGBA8Uint;
        case FMT_R8G8B8A8_SINT:         return WGPUTextureFormat_RGBA8Sint;
        case FMT_R16G16B16A16_FLOAT:    return WGPUTextureFormat_RGBA16Float;
        case FMT_R32G32B32A32_FLOAT:    return WGPUTextureFormat_RGBA32Float;
        case FMT_D32_UNORM:             return WGPUTextureFormat_Depth32Float;
        case FMT_R8_UNORM:              return WGPUTextureFormat_R8Unorm;
        case FMT_R8G8_UNORM:            return WGPUTextureFormat_RG8Unorm;
        default:                        return WGPUTextureFormat_Undefined;
        }
    }
}

#include    "VKDevice.h"
#include    "VKRenderSystem.h"

#include    "VKCmdBuffer.h"
#include    "VKQueue.h"
#include    "VKFrameBuffer.h"
#include    "VKDebugUtils.h"
#include    "VKInstanceExtensions.h"
#include    "VKInstanceLayers.h"
#include    "VKShader.h"
#include    "VKRenderPass.h"
#include    "VKGPipeline.h"
#include    "VKSwapchain.h"
#include    "VKGImage.h"
#include    "VKGImageView.h"
#include    "VKCmdPool.h"
#include    "VKDSet.h"
#include    "VKDSetLayout.h"
#include    "VKDSetPool.h"
#include    "VKFence.h"
#include    "VKSemaphore.h"
#include    "VKGPUBuffer.h"

namespace   FE
{
    PCSTR       needExtensions[]    =   
    {
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        /// <summary>
        /// 使用动态渲染
        /// </summary>
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };

    VKDevice::~VKDevice()
    {
        if (_destroyNotify) _destroyNotify(*this);

        _graphicPool    =   nullptr;
        _computePool    =   nullptr;
        _transferPool   =   nullptr;

        _queueGraphic   =   nullptr;       
        _queueCompute   =   nullptr;
        _queueTransfer  =   nullptr;
        
        if (_logicalDevice)
        {
            vkDestroyDevice(_logicalDevice,nullptr);
        }
    }
    FEResult    VKDevice::create(const CreateInfo& cInfo)
    {
        auto    gpu     =   _renderSys.gpu(cInfo.deviceId);
        assert(gpu.isValid());
        if (!gpu.isValid())
            return  FEResult::ER_FAILED;
        _cInfo          =   cInfo;
        LOG_INF("VKDevice.create()");
        LOG_INF("GPU.name   =   %s",gpu.name.c_str());
        LOG_INF("GPU.type   =   %s",FERenderSystem::nameOf(gpu.type));
        LOG_INF("GPU.id     =   %s",gpu.gpuId.toString().c_str());
        
        _physicalDevice =  (VkPhysicalDevice)gpu.gpu;

        vkGetPhysicalDeviceProperties(_physicalDevice,       &_properties);
        vkGetPhysicalDeviceFeatures(_physicalDevice,         &_features);
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memoryProperties);
        uint32_t    queueFamilyCount    =    0;
        vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        _queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, _queueFamilyProperties.data());

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                for (auto& ext : extensions)
                {
                    _supportedExtensions.push_back(ext.extensionName);
                }
            }
        }
        _enabledFeatures.multiDrawIndirect  =   VK_TRUE;

        for (auto var : needExtensions)
        {
            if (supportExtension(var) )
                _cInfo.extensions.push_back(var);
        }
        
        if(createLogicalDevice(_enabledFeatures,_cInfo.extensions,nullptr) != VK_SUCCESS)
            return  FEResult::ER_FAILED;
        _queueGraphic   =   queueGraphic();
        _queueCompute   =   queueCompute();
        _queueTransfer  =   queueTransfer();

        if (_createNotify)  _createNotify(*this);

        return  FEResult::ER_SUCCESS;
    }

    void    VKDevice::waitIdle()
    {
        if (_logicalDevice)
        {
            vkDeviceWaitIdle(_logicalDevice);
        }
    }

    Fence   VKDevice::createFence()
    {
        LOG_DBG("VKRenderSystem.createFence");
        return  new VKFence(_ctx);
    }

    Semaphore   VKDevice::createSemaphore() 
    {
        LOG_DBG("VKRenderSystem.createSemaphore");
        return  new VKSemaphore(_ctx);
    }

    CMDPool     VKDevice::createCmdPool()   
    {
        LOG_DBG("VKRenderSystem.createCmdPool");
        return  new VKCmdPool(_ctx);
    }
    FBOPtr      VKDevice::createFrameBuffer() 
    {
        LOG_DBG("VKRenderSystem.createFrameBuffer");
        return  new VKFrameBuffer(_ctx);
    }
    Shader      VKDevice::createShader()      
    {
        LOG_DBG("VKRenderSystem.createShader");
        return  new VKShader(_ctx);
    }
    RenderPass  VKDevice::createRenderPass()  
    {
        LOG_DBG("VKRenderSystem.createRenderPass");
        return  new VKRenderPass(_ctx);
    }
    GPipeline   VKDevice::createGPipeline()   
    {
        LOG_DBG("VKRenderSystem.createGPipeline");
        return  new VKGPipeline(_ctx);
    }
    Swapchain   VKDevice::createSwapchain()        
    {
        LOG_DBG("VKRenderSystem.Swapchain");
        return  new VKSwapchain(_ctx);
    }
    GImage      VKDevice::createGImage()         
    {
        LOG_DBG("VKRenderSystem.createGImage");
        return  new VKGImage(_ctx);
    }

    DSetLayout  VKDevice::createDSLayout()
   {
       LOG_DBG("VKRenderSystem.createDSLayout");
       return  new VKDSetLayout(_ctx);
   }
    DSetLayout  VKDevice::createDSLayoutFromShaders(const std::vector<Shader>& shaders)
   {
       std::vector<FEDSetBinding>        allBindings;
       std::map<uint32_t, FEDSetBinding> bindingMap;

       for (const auto& shader : shaders)
       {
           if (!shader) continue;
           const auto* pShader      =   shader.get();
           const auto* vkShader     =   static_cast<const VKShader*>(pShader);
           const auto& reflectData  =   vkShader->reflectData();

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
                   bindingMap[key] = binding;
               }
           }
       }

       for (const auto& pair : bindingMap)
       {
           allBindings.push_back(pair.second);
       }

       std::sort(allBindings.begin(), allBindings.end(), [](const FEDSetBinding& a, const FEDSetBinding& b) {
           return a._binding < b._binding;
       });

       if (allBindings.empty())
           return nullptr;

       FEDSetLayout::CreateInfo info;
       info._bindings = allBindings;

       auto layout = new VKDSetLayout(_ctx);
       if (!layout->create(info))
       {
           delete layout;
           return nullptr;
       }
       return layout;
   }
    DSetPool    VKDevice::createDSetPool() 
   {
       LOG_DBG("VKRenderSystem.createDSetPool");
       return  new VKDSetPool(_ctx);
   }
    DSet        VKDevice::createDSet()   
   {
       LOG_DBG("VKRenderSystem.createDSet");
       return  new VKDSet(_ctx);
   }

    VBO         VKDevice::createVBO() 
    {
        LOG_DBG("VKRenderSystem.createVBO");
        return  new VKGPUBuffer(_ctx,BufferUsage::VERTEX_BUFFER_BIT);
    }
    IBO         VKDevice::createIBO()
    {
        LOG_DBG("VKRenderSystem.createIBO");
        return  new VKGPUBuffer(_ctx,BufferUsage::INDEX_BUFFER_BIT);
    }
    UBO         VKDevice::createUBO() 
    {
        LOG_DBG("VKRenderSystem.createUBO");
        return  new VKGPUBuffer(_ctx,BufferUsage::UNIFORM_BUFFER_BIT);
    }
    ITO         VKDevice::createITO() 
    {
        LOG_DBG("VKRenderSystem.createITO");
        return  new VKGPUBuffer(_ctx,BufferUsage::INDIRECT_BUFFER_BIT);
    }

    SBO         VKDevice::createSBO() 
    {
        LOG_DBG("VKRenderSystem.createSBO");
        return  new VKGPUBuffer(_ctx,BufferUsage::STORAGE_BUFFER_BIT);
    }
    
    GPUBuffer   VKDevice::createFromBuffer(const void* pData,uint64 length) 
    {
        assert(length != 0  && pData != nullptr);
        if (length == 0 || pData == nullptr)
            return  nullptr;
        LOG_DBG("VKRenderSystem.createUBO");
        auto    object  =  new VKGPUBuffer(_ctx,BufferUsage::TRANSFER_SRC_BIT);
        FEGPUBuffer::CreateInfo cInfo;
        cInfo._bufUsages    =   TRANSFER_SRC_BIT;
        cInfo._memUsages    =   HOST_VISIBLE_BIT;
        cInfo._length       =   length;
        if(object->create(cInfo))
        {
            object->update(pData,length,0);
            return  object;
        }
        LOG_ERR("VKGPUBuffer.create return false");
        return  nullptr;
    }

    uint32      VKDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
    {
        for (uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    if (memTypeFound)
                    {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound)
        {
            *memTypeFound = false;
            return 0;
        }
        else
        {
            throw std::runtime_error("Could not find a matching memory type");
        }
    }

    uint32      VKDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
    {
        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++)
            {
                if ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++)
            {
                if ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) 
                && ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) 
                && ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)  == 0))
                {
                    return i;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++)
        {
            if ((_queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
            {
                return i;
            }
        }
        throw std::runtime_error("Could not find a matching queue family index");
    }

    uint32      VKDevice::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }

        throw "Could not find a suitable memory type!";
    }

    VkResult    VKDevice::createLogicalDevice(DeviceFeatures enabledFeatures, const PCSTRs& enabledExtensions, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
    {            
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

        const float defaultQueuePriority(0.0f);

        /// Graphics queue
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
        {
            _queueFamilyIndices.graphics =   getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
            VkDeviceQueueCreateInfo queueInfo   =   {};
            queueInfo.sType             =   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex  =   _queueFamilyIndices.graphics;
            queueInfo.queueCount        =   1;
            queueInfo.pQueuePriorities  =   &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
        else
        {
            _queueFamilyIndices.graphics =   0;
        }
        /// Dedicated compute queue
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
        {
            _queueFamilyIndices.compute      =   getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
            if (_queueFamilyIndices.compute != _queueFamilyIndices.graphics)
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo   =   {};
                queueInfo.sType             =   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex  =   _queueFamilyIndices.compute;
                queueInfo.queueCount        =   1;
                queueInfo.pQueuePriorities  =   &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        else
        {
            _queueFamilyIndices.compute  =   _queueFamilyIndices.graphics;
        }
        /// Dedicated transfer queue
        if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
        {
            _queueFamilyIndices.transfer     =   getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
            if ((_queueFamilyIndices.transfer != _queueFamilyIndices.graphics) && (_queueFamilyIndices.transfer != _queueFamilyIndices.compute))
            {
                VkDeviceQueueCreateInfo queueInfo   =   {};
                queueInfo.sType             =   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex  =   _queueFamilyIndices.transfer;
                queueInfo.queueCount        =   1;
                queueInfo.pQueuePriorities  =   &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        else if(_queueFamilyIndices.transfer == _queueFamilyIndices.graphics)
            _queueFamilyIndices.transfer =   _queueFamilyIndices.graphics;
        else if(_queueFamilyIndices.transfer == _queueFamilyIndices.compute)
            _queueFamilyIndices.transfer =   _queueFamilyIndices.compute;
        else
            _queueFamilyIndices.transfer =   _queueFamilyIndices.graphics;

        PCSTRs  deviceExtensions(enabledExtensions);
        if (useSwapChain)
            deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VkDeviceCreateInfo deviceCreateInfo =   {};
        deviceCreateInfo.sType                  =   VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount   =   static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos      =   queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures       =   &enabledFeatures;

        // If a pNext(Chain) has been passed, we need to add it to the device creation info
        VkPhysicalDeviceFeatures2                           physicalDeviceFeatures2     =   {};
        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT    dynamicState3Features       =   {};
        VkPhysicalDeviceDynamicRenderingFeatures            dynamicRenderingFeatures    =   {};

        if (pNextChain) 
        {
            physicalDeviceFeatures2.sType       =   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.features    =   enabledFeatures;
            physicalDeviceFeatures2.pNext       =   pNextChain;
            deviceCreateInfo.pEnabledFeatures   =   nullptr;
            deviceCreateInfo.pNext              =   &physicalDeviceFeatures2;
        }
        else
        {
            
            dynamicState3Features.sType =   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
            dynamicState3Features.pNext =   nullptr;

            /// 根据需求启用功能，例如启用动态设置多边形模式和光栅化采样数
            dynamicState3Features.extendedDynamicState3PolygonMode          =   VK_TRUE;          
            /// 对应 vkCmdSetPolygonModeEXT[reference:9]
            dynamicState3Features.extendedDynamicState3RasterizationSamples =   VK_TRUE; 
            deviceCreateInfo.pNext      =   &dynamicState3Features;

            
            dynamicRenderingFeatures.sType              =   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
            dynamicRenderingFeatures.pNext              =   nullptr;
            dynamicRenderingFeatures.dynamicRendering   =   VK_TRUE;

            dynamicState3Features.pNext                 =   &dynamicRenderingFeatures;
        }


#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_KHR_portability_subset)
        // SRS - When running on iOS/macOS with MoltenVK and VK_KHR_portability_subset is defined and supported by the device, enable the extension
        if (extensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
#endif
        if (deviceExtensions.size() > 0)
        {
            for (auto enabledExtension : deviceExtensions)
            {
                if (!extensionSupported(enabledExtension)) 
                {
                    std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
                }
            }
            deviceCreateInfo.enabledExtensionCount      =   (uint32_t)deviceExtensions.size();
            deviceCreateInfo.ppEnabledExtensionNames    =   deviceExtensions.data();

            VkPhysicalDeviceFeatures features{};
            features.multiDrawIndirect  = VK_TRUE; 
        }
        _enabledFeatures    =   enabledFeatures;
        VkResult result     =   vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_logicalDevice);
        if (result != VK_SUCCESS) 
        {
            return result;
        }

        _graphicPool    =   createCmdPool();
        _computePool    =   createCmdPool();
        _transferPool   =   createCmdPool();

        _graphicPool    ->create({_queueFamilyIndices.graphics});
        _computePool    ->create({_queueFamilyIndices.compute});
        _transferPool   ->create({_queueFamilyIndices.transfer});


        return result;
    }

    VkFormat    VKDevice::getSupportedDepthFormat(bool checkSamplingSupport)
    {
        VkFormats   depthFormats    = 
        { 
            VK_FORMAT_D32_SFLOAT_S8_UINT, 
            VK_FORMAT_D32_SFLOAT, 
            VK_FORMAT_D24_UNORM_S8_UINT, 
            VK_FORMAT_D16_UNORM_S8_UINT, 
            VK_FORMAT_D16_UNORM 
        };
        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &formatProperties);
            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if (checkSamplingSupport) 
                {
                    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                    {
                        continue;
                    }
                }
                return format;
            }
        }
        throw std::runtime_error("Could not find a matching depth format");
    }

    Queue       VKDevice::queueGraphic()
    {
        if (_queueGraphic == nullptr)
        {
            VkQueue vkQ     =   nullptr;
            vkGetDeviceQueue(_logicalDevice, _queueFamilyIndices.graphics, 0, &vkQ);
            assert(vkQ != nullptr);
            _queueGraphic  =   new VKQueue(_ctx,vkQ);
        }
        
        return  _queueGraphic;
    }
    Queue       VKDevice::queueCompute()
    {
        if (_queueCompute == nullptr)
        {
            VkQueue vkQ     =   nullptr;
            vkGetDeviceQueue(_logicalDevice, _queueFamilyIndices.compute, 0, &vkQ);
            assert(vkQ != nullptr);
            _queueCompute  =   new VKQueue(_ctx,vkQ);
        }

        return  _queueCompute;
    }

    Queue       VKDevice::queueTransfer()
    {
        if (_queueTransfer == nullptr)
        {
            VkQueue vkQ     =   nullptr;
            vkGetDeviceQueue(_logicalDevice, _queueFamilyIndices.transfer, 0, &vkQ);
            assert(vkQ != nullptr);
            _queueTransfer  =   new VKQueue(_ctx,vkQ);
        }
        return  _queueTransfer;
    };

}

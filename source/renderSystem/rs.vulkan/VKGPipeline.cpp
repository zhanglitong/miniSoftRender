
#include    "VKGPipeline.h"
#include    "VKDevice.h"
#include    "VKShader.h"
#include    "FEVulkan.h"
namespace   FE
{
    VKGPipeline::~VKGPipeline()
    {
        _pools.clear();

        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyPipeline(device, _native, nullptr);
            vkDestroyPipelineLayout(device, _layout, nullptr);
        }
    }

    DSetLayout  createDSLayoutFromShaders(FEContext& ctx,FEDevice& device,const Shaders& shaders)
    {
        (void)device;
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
                uint32_t    key =   binding._binding;
                auto        it  =   bindingMap.find(key);
                if (it != bindingMap.end())
                {
                    it->second._stageFlags =    (it->second._stageFlags | binding._stageFlags);
                }
                else
                {
                    bindingMap[key]         =   binding;
                }
            }
        }

        for (const auto& pair : bindingMap)
        {
            allBindings.push_back(pair.second);
        }

        std::sort(allBindings.begin(), allBindings.end(), [](const FEDSetBinding& a, const FEDSetBinding& b) 
        {
            return a._binding < b._binding;
        });

        if (allBindings.empty())
            return nullptr;

        FEDSetLayout::CreateInfo info;
        info._bindings  =   allBindings;

        auto layout = new VKDSetLayout(ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }


    bool    VKGPipeline::create(const FEGPipeline::CreateInfo& info) 
    {
        using   PushConsts  =   std::vector<VkPushConstantRange>;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        _cInfo  =   info;
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};

        Shaders     shaders;
        PushConsts  pushConsts;

        for (auto& shader : info._shaders)
        {
            if (shader == nullptr)  
                continue;
            shaders.emplace_back(shader);
            auto    vkShader    =   shader->as<VKShader>();
            auto    refect      =   vkShader->reflectData();
            pushConsts.insert(pushConsts.end(),refect._pushConstants.begin(),refect._pushConstants.end());
            if (!refect._pushConstants.empty())
                _cInfo._pushConstantStage  |=  refect._stage;
        }
        auto    layout  =   createDSLayoutFromShaders(_ctx,_ctx.device(),shaders);
        if (layout)
        {
            _dsLayouts.clear();
            _dsLayouts.push_back(layout);
        }
        VkDescriptorSetLayout   dsLayouts[1]    =   {layout ? (VkDescriptorSetLayout)layout->native() : nullptr};
 
        pipelineLayoutCI.sType                  =    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext                  =    nullptr;
        pipelineLayoutCI.setLayoutCount         =    layout ? 1 : 0;
        pipelineLayoutCI.pSetLayouts            =    layout ? dsLayouts : nullptr;
        pipelineLayoutCI.pushConstantRangeCount =    (uint)pushConsts.size();
        pipelineLayoutCI.pPushConstantRanges    =    pushConsts.data();
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &_layout));

        /// 锟解部锟斤拷询使锟斤拷
        _cInfo._pushConstant   =   !pushConsts.empty();

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType                =   VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout               =  _layout;
        pipelineCI.renderPass           =   (VkRenderPass)(info._renderPass ? info._renderPass->native():nullptr);

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType      =   VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology   =   system2Native(_cInfo._inputAssemblyState._primitive);

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType                      =   VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.polygonMode                =   VK_POLYGON_MODE_FILL;
        // cullMode 覆盖：-1=默认BACK, 0=NONE, 1=FRONT, 2=BACK, 3=FRONT_AND_BACK
        {
            VkCullModeFlagBits defaultCull = VK_CULL_MODE_BACK_BIT;
            if (info._cullModeOverride >= 0 && info._cullModeOverride <= 3)
            {
                switch (info._cullModeOverride)
                {
                case 0: defaultCull = VK_CULL_MODE_NONE; break;
                case 1: defaultCull = VK_CULL_MODE_FRONT_BIT; break;
                case 2: defaultCull = VK_CULL_MODE_BACK_BIT; break;
                case 3: defaultCull = VK_CULL_MODE_FRONT_AND_BACK; break;
                }
            }
            rasterizationStateCI.cullMode               =   defaultCull;
        }
        rasterizationStateCI.frontFace                  =   VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.depthClampEnable           =   VK_FALSE;
        rasterizationStateCI.rasterizerDiscardEnable    =   VK_FALSE;
        rasterizationStateCI.depthBiasEnable            =   VK_FALSE;
        rasterizationStateCI.lineWidth                  =   1.0f;

        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.colorWriteMask             =   0xf;
        blendAttachmentState.blendEnable                =   info._enableColorBlend ? VK_TRUE : VK_FALSE;
        if (info._enableColorBlend)
        {
            // 标准 alpha blending: src * srcAlpha + dst * (1 - srcAlpha)
            // 预乘 alpha(transmission 玻璃): src * 1 + dst * (1 - srcAlpha)
            blendAttachmentState.srcColorBlendFactor    =   info._premultipliedAlpha
                                                            ? VK_BLEND_FACTOR_ONE
                                                            : VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachmentState.dstColorBlendFactor    =   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachmentState.colorBlendOp           =   VK_BLEND_OP_ADD;
            blendAttachmentState.srcAlphaBlendFactor    =   VK_BLEND_FACTOR_ONE;
            blendAttachmentState.dstAlphaBlendFactor    =   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachmentState.alphaBlendOp           =   VK_BLEND_OP_ADD;
        }
        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType                         =   VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.attachmentCount               =   info._colorAttachmentCount;
        colorBlendStateCI.pAttachments                  =   (info._colorAttachmentCount > 0) ? &blendAttachmentState : nullptr;

        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType                           =   VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount                   =   1;
        viewportStateCI.scissorCount                    =   1;

        std::vector<VkDynamicState> dynamicStateEnables;
        for (auto& var: _cInfo._dynamicStates)
        {
            auto    state   =   system2Native(var);
            dynamicStateEnables.push_back(state);
        }
        /// 确保光栅化动态状态始终启用 (appDynamicState 会设置这些状态)
        auto    fnHas   =   [&](VkDynamicState s)
        {
            return  std::find(dynamicStateEnables.begin(), dynamicStateEnables.end(), s) != dynamicStateEnables.end();
        };
        if (!fnHas(VK_DYNAMIC_STATE_CULL_MODE))
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_CULL_MODE);
        if (!fnHas(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE))
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);
        if (!fnHas(VK_DYNAMIC_STATE_LINE_WIDTH))
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType                            =   VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates                   =   dynamicStateEnables.data();
        dynamicStateCI.dynamicStateCount                =   static_cast<uint32_t>(dynamicStateEnables.size());

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType                       =   VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable             =   info._enableDepthTest ? VK_TRUE : VK_FALSE;
        depthStencilStateCI.depthWriteEnable            =   info._enableDepthWrite ? VK_TRUE : VK_FALSE;
        depthStencilStateCI.depthCompareOp              =   VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateCI.depthBoundsTestEnable       =   VK_FALSE;
        depthStencilStateCI.back.failOp                 =   VK_STENCIL_OP_KEEP;
        depthStencilStateCI.back.passOp                 =   VK_STENCIL_OP_KEEP;
        depthStencilStateCI.back.compareOp              =   VK_COMPARE_OP_ALWAYS;
        depthStencilStateCI.stencilTestEnable           =   VK_FALSE;
        depthStencilStateCI.front                       =   depthStencilStateCI.back;

        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType                        =   VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCI.rasterizationSamples         =   VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCI.pSampleMask                  =   nullptr;
        // 无 color attachment 的管线（如 OIT geometry pass）alpha-to-coverage 无意义且会触发 VUID-08891
        multisampleStateCI.alphaToCoverageEnable        =   (info._colorAttachmentCount > 0) ? VK_TRUE : VK_FALSE;

        std::vector<VkVertexInputBindingDescription>    vertexInputBindings; 
        std::vector<VkVertexInputAttributeDescription>  vertexInputAttributs;
        vertexInputBindings.resize(_cInfo._binds.size());

        for (size_t i = 0; i < _cInfo._binds.size(); i++)
        {
            auto&   sysBind                     =   _cInfo._binds[i];
            vertexInputBindings[i].binding      =   sysBind.binding;
            vertexInputBindings[i].stride       =   sysBind.stride;
            vertexInputBindings[i].inputRate    =   (VkVertexInputRate)sysBind.inputRate;

            for (auto& var: sysBind.inputs)
            {
                VkVertexInputAttributeDescription   attr    =   {};
                attr.binding     =   sysBind.binding;
                attr.location    =   var.location;
                attr.format      =   system2Native(var.format);
                attr.offset      =   var.offset;
                vertexInputAttributs.emplace_back(attr);
            }
        }
        // Vertex input state used for pipeline creation
        VkPipelineVertexInputStateCreateInfo vsState{};
        vsState.sType                            =   VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vsState.vertexBindingDescriptionCount    =   (uint)vertexInputBindings.size();
        vsState.pVertexBindingDescriptions       =   vertexInputBindings.data();
        vsState.vertexAttributeDescriptionCount  =   (uint)vertexInputAttributs.size();
        vsState.pVertexAttributeDescriptions     =   vertexInputAttributs.data();

        // Shaders
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.resize(shaders.size());
        for (size_t i = 0; i < shaders.size(); i++)
        {
            shaderStages[i]                 =   {};
            shaderStages[i].sType           =   VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage           =   (VkShaderStageFlagBits)system2Native(shaders[i]->cInfo()._shaderType);
            shaderStages[i].module          =   (VkShaderModule)(shaders[i]->native());
            shaderStages[i].pName           =   "main";
        }

        // Set pipeline shader stage info
        pipelineCI.stageCount           =   static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages              =   shaderStages.data();

        // Assign the pipeline states to the pipeline creation info structure
        pipelineCI.pVertexInputState    =   &vsState;
        pipelineCI.pInputAssemblyState  =   &inputAssemblyStateCI;
        pipelineCI.pRasterizationState  =   &rasterizationStateCI;
        pipelineCI.pColorBlendState     =   &colorBlendStateCI;
        pipelineCI.pMultisampleState    =   &multisampleStateCI;
        pipelineCI.pViewportState       =   &viewportStateCI;
        pipelineCI.pDepthStencilState   =   &depthStencilStateCI;
        pipelineCI.pDynamicState        =   &dynamicStateCI;

        // New create info to define color, depth and stencil attachments at pipeline create time
        VkPipelineRenderingCreateInfoKHR    pRenderingCreateInfo    =   {};
        VkFormat    colorAttrFormats[]                  =   {VK_FORMAT_R8G8B8A8_UNORM};
        pRenderingCreateInfo.sType                      =   VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pRenderingCreateInfo.colorAttachmentCount       =   info._colorAttachmentCount;
        pRenderingCreateInfo.pColorAttachmentFormats    =   (info._colorAttachmentCount > 0) ? colorAttrFormats : nullptr;
        pRenderingCreateInfo.depthAttachmentFormat      =   VK_FORMAT_D32_SFLOAT_S8_UINT;
        pRenderingCreateInfo.stencilAttachmentFormat    =   VK_FORMAT_D32_SFLOAT_S8_UINT;

        pipelineCI.pNext    =   &pRenderingCreateInfo;
        

        // Create rendering pipeline using the specified states
        (void)vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &_native);

        return  _native != nullptr;
    }

    DSets   VKGPipeline::createDSets() 
    {
        DSetPoolVK  pool    =   nullptr;
        for (auto& var : _pools)
        {
            if (!var->hasFree() )
                continue;
            pool    =   var;
            break;
        }
        if (pool == nullptr)
        {
            pool    =   new VKDSetPool(_ctx);
            FEDSetPool::CreateInfo  info;
            info._layouts   =   _dsLayouts;
            info._maxSets   =   64;
            if(!pool->create(info))
                return  {};
            _pools.emplace_back(pool);
        }

        DSets   result;
        for (size_t i = 0; i < _dsLayouts.size(); i++)
        {
            DSet    dset    =   _ctx.device().createDSet();
            FEDSet::CreateInfo  cInfo;
            cInfo._layout   =   _dsLayouts[i];
            cInfo._pool     =   pool;
            
            if(dset->create(cInfo))
                result.emplace_back(dset);
        }
        return  result;
    }

    
}

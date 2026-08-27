
#include    "VKCPipeline.h"
#include    "VKDevice.h"
#include    "VKShader.h"
#include    "FEVulkan.h"
namespace   FE
{
    extern  DSetLayout  createDSLayoutFromShaders(FEContext& ctx,FEDevice& device,const Shaders& shaders);

    VKCPipeline::~VKCPipeline()
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


    bool    VKCPipeline::create(const FEGPipeline::CreateInfo& info) 
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
        _dsLayouts.clear();
        _dsLayouts.push_back(layout);

        VkDescriptorSetLayout   dsLayouts[1]    =   {(VkDescriptorSetLayout)layout->native()};
 
        pipelineLayoutCI.sType                  =    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext                  =    nullptr;
        pipelineLayoutCI.setLayoutCount         =    1;
        pipelineLayoutCI.pSetLayouts            =    dsLayouts;
        pipelineLayoutCI.pushConstantRangeCount =    (uint)pushConsts.size();
        pipelineLayoutCI.pPushConstantRanges    =    pushConsts.data();
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &_layout));

        /// 
        _cInfo._pushConstant   =   !pushConsts.empty();

        VkPipelineShaderStageCreateInfo shaderStage = {};

        shaderStage.sType               =   VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.pNext               =   NULL;
        shaderStage.flags               =   0;
        shaderStage.stage               =   VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module              =   (VkShaderModule)shaders.front()->native();
        shaderStage.pName               =   "main";
        shaderStage.pSpecializationInfo =   nullptr;

        VkComputePipelineCreateInfo     pipelineCI{};
        pipelineCI.sType                =   VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCI.layout               =  _layout;
        pipelineCI.stage                =   shaderStage;
        // Create rendering pipeline using the specified states
        auto    result  =   vkCreateComputePipelines(device, nullptr, 1, &pipelineCI, nullptr, &_native);
        UNUSED(result);
        return  _native != nullptr;
    }

    DSets   VKCPipeline::createDSets() 
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


#include    "VKShader.h"
#include    "VKDevice.h"
#include    "VKDSetLayout.h"
#include    "graphic/spirv-reflect/spirv_reflect.h"

namespace   FE
{
    VKShader::~VKShader()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyShaderModule(device,_native,nullptr);
        }
    }
    bool    VKShader::create(const FEShader::CreateInfo& info) 
    {
        if (info._buffer == nullptr || info._buffer->length() == 0)
            return  false;

        assert(info._buffer->length()%sizeof(uint) == 0);
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        _cInfo  =   info;
        VkShaderModuleCreateInfo cInfo{};
        cInfo.sType     =   VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        cInfo.codeSize  =   _cInfo._buffer->length();
        cInfo.pCode     =   (uint32_t*)_cInfo._buffer->data();

        VK_CHECK_RESULT(vkCreateShaderModule(device, &cInfo, nullptr, &_native));

        spirvReflect(cInfo);
        return  _native != nullptr;
    }

    void    VKShader::spirvReflect(const VkShaderModuleCreateInfo& info)
    {
        using   SRDSetBinds     =   std::vector<SpvReflectDescriptorBinding*>;  
        using   SRInputs        =   std::vector<SpvReflectInterfaceVariable*>;
        using   SRConstants     =   std::vector<SpvReflectBlockVariable*>;
        SpvReflectShaderModule  module  =   {};
        SpvReflectResult        result  =   SPV_REFLECT_RESULT_SUCCESS;
        result  =   spvReflectCreateShaderModule(info.codeSize,info.pCode,&module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        uint32_t    bindingCount = 0;
        result  =   spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
        SRDSetBinds bindings(bindingCount);
        result =    spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        FEShaderType stageFlags = FEShaderType::ST_VERTEX_BIT;
        if (module.entry_point_count > 0 && module.entry_points != nullptr)
        {
            switch (module.entry_points[0].shader_stage)
            {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT                 :  stageFlags  =   ST_VERTEX_BIT                   ;    break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT   :  stageFlags  =   ST_TESSELLATION_CONTROL_BIT     ;    break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:  stageFlags  =   ST_TESSELLATION_EVALUATION_BIT  ;    break;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT               :  stageFlags  =   ST_GEOMETRY_BIT                 ;    break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT               :  stageFlags  =   ST_FRAGMENT_BIT                 ;    break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT                :  stageFlags  =   ST_COMPUTE_BIT                  ;    break;
            case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV                :  stageFlags  =   ST_TASK_BIT                     ;    break;
            case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV                :  stageFlags  =   ST_MESH_BIT                     ;    break;
            case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR             :  stageFlags  =   ST_RAYGEN_BIT                   ;    break;
            case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR            :  stageFlags  =   ST_ANY_HIT_BIT                  ;    break;
            case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR        :  stageFlags  =   ST_CLOSEST_HIT_BIT              ;    break;
            case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR               :  stageFlags  =   ST_MISS_BIT                     ;    break;
            case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR       :  stageFlags  =   ST_INTERSECTION_BIT             ;    break;
            case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR           :  stageFlags  =   ST_CALLABLE_BIT                 ;    break;
            default:
                break;
            }
        }

        for (const auto& binding : bindings)
        {
            FEDSetBinding layoutBinding{};
            layoutBinding._binding          =   binding->binding;
            layoutBinding._descriptorType   =   (FEDescType)binding->descriptor_type;
            layoutBinding._descriptorCount  =   binding->count;
            layoutBinding._stageFlags       =   stageFlags;
            layoutBinding._name             =   binding->name;
            if (layoutBinding._name .empty() && binding->block.type_description && binding->block.type_description->members)
            {
                layoutBinding._name =   binding->block.type_description->members->struct_member_name;
            }
            layoutBinding._size             =   uint16_t(binding->block.size);
            layoutBinding._typeName         =   binding->block.type_description ? binding->block.type_description->type_name : "";

            _reflectData._bindings.push_back(layoutBinding);
        }
        uint32_t    pushConstCout   =   0;
        result  =   spvReflectEnumeratePushConstantBlocks(&module, &pushConstCout, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        _reflectData._stage =   stageFlags;

        _cInfo._shaderType  =   (FEShaderType)stageFlags;

        SRConstants pushConsts(pushConstCout);
        result  =   spvReflectEnumeratePushConstantBlocks(&module, &pushConstCout, pushConsts.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        for (const auto& pc : pushConsts) 
        {
            VkPushConstantRange range   =   
            {
                module.shader_stage,
                pc->offset,
                pc->size,
            };
            _reflectData._pushConstants.push_back(range);
        }

        uint32_t inputVarCount = 0;
        result = spvReflectEnumerateInputVariables(&module, &inputVarCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        SRInputs inputVars(inputVarCount);
        result = spvReflectEnumerateInputVariables(&module, &inputVarCount, inputVars.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);


        spvReflectDestroyShaderModule(&module);
    }

    DSetLayout VKShader::createLayoutFromReflect()
    {
        if (_reflectData._bindings.empty())
            return nullptr;

        FEDSetLayout::CreateInfo info;
        info._bindings = _reflectData._bindings;

        auto layout = new VKDSetLayout(_ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }
}

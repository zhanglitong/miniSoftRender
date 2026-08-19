#include    "WGShader.h"
#include    "WGDevice.h"
#include    "WGDSetLayout.h"
#include    "graphic/spirv-reflect/spirv_reflect.h"
#include    <algorithm>
#include    <string>

namespace   FE
{
    WGShader::~WGShader()
    {
        if (_native)
        {
            wgpuShaderModuleRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGShader::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        if (info._buffer == nullptr || info._buffer->length() == 0)
            return false;

        assert(info._buffer->length() % sizeof(uint32_t) == 0);

        const auto* source = reinterpret_cast<const uint32_t*>(info._buffer->data());
        uint32_t sourceSize = static_cast<uint32_t>(info._buffer->length() / sizeof(uint32_t));

        if (source == nullptr || sourceSize == 0)
            return false;

        _native = createShaderModule(wgDevice.device(), source, sourceSize);

        if (_native)
        {
            reflectShaderSPIRV(info);
            return true;
        }

        LOG_ERR("WGShader.create failed");
        return false;
    }

    WGPUShaderModule WGShader::createShaderModule(WGPUDevice device, const uint32_t* source, uint32_t sourceSize)
    {
        if (device == nullptr)
        {
            LOG_ERR("WGShader::createShaderModule: device is null");
            return nullptr;
        }

        // Check if PassthroughShaders feature is available
        bool hasPassthrough = wgpuDeviceHasFeature(device, (WGPUFeatureName)0x00030036);
        if (hasPassthrough)
        {
            WGPUShaderModuleDescriptorSpirV spirvDesc = {};
            spirvDesc.label     =   { nullptr, 0 };
            spirvDesc.source    =   source;
            spirvDesc.sourceSize=   sourceSize;

            WGPUShaderModule module = wgpuDeviceCreateShaderModuleSpirV(device, &spirvDesc);
            if (module != nullptr)
            {
                LOG_INF("WGShader: created shader module via PassthroughShaders");
                return module;
            }
            LOG_INF("WGShader: Passthrough failed, falling back to standard SPIR-V");
        }
        else
        {
            LOG_INF("WGShader: PassthroughShaders not available, using standard SPIR-V");
        }

        // Fallback: use standard WebGPU API with WGPUShaderSourceSPIRV
        {
            WGPUShaderSourceSPIRV spirvSource = {};
            spirvSource.chain.sType     =   WGPUSType_ShaderSourceSPIRV;
            spirvSource.chain.next      =   nullptr;
            spirvSource.codeSize        =   sourceSize;
            spirvSource.code            =   source;

            WGPUShaderModuleDescriptor shaderDesc = {};
            shaderDesc.nextInChain      =   reinterpret_cast<WGPUChainedStruct*>(&spirvSource);
            shaderDesc.label            =   { nullptr, 0 };

            WGPUShaderModule    module  =   wgpuDeviceCreateShaderModule(device, &shaderDesc);
            if (module != nullptr)
            {
                LOG_INF("WGShader: created shader module via standard SPIR-V");
                return module;
            }
            LOG_ERR("WGShader: standard SPIR-V also failed");
        }

        return nullptr;
    }

    void WGShader::reflectShaderSPIRV(const CreateInfo& info)
    {
        using SRDSetBinds       =   std::vector<SpvReflectDescriptorBinding*>;
        using SRInputs          =   std::vector<SpvReflectInterfaceVariable*>;
        using SRConstants       =   std::vector<SpvReflectBlockVariable*>;

        const auto* pCode       =   reinterpret_cast<const uint32_t*>(info._buffer->data());
        uint32_t    codeSize    =   info._buffer->length();

        SpvReflectShaderModule  module  =   {};
        SpvReflectResult        result  =   spvReflectCreateShaderModule(codeSize, pCode, &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS)
        {
            LOG_ERR("WGShader: spvReflectCreateShaderModule failed: %d", result);
            _reflectData._stages        =   info._shaderType;
            _reflectData._stageFlags    =   0;
            return;
        }

        uint32_t    bindingCount    =   0;
        result  =   spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS)
        {
            spvReflectDestroyShaderModule(&module);
            return;
        }

        SRDSetBinds bindings(bindingCount);
        result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS)
        {
            spvReflectDestroyShaderModule(&module);
            return;
        }

        uint32_t    stageFlags  =   0;
        for (uint32_t i = 0; i < module.entry_point_count ; i++)
        {
            switch (module.entry_points[i].shader_stage)
            {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:   stageFlags  |= ST_VERTEX_BIT;     break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: stageFlags  |= ST_FRAGMENT_BIT;   break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:  stageFlags  |= ST_COMPUTE_BIT;    break;
            default: break;
            }
        }

        _reflectData._stages        =   stageFlags;
        _reflectData._stageFlags    =   stageFlags;
        _cInfo._shaderType          =   stageFlags;

        for (const auto& binding : bindings)
        {
            ReflectBinding reflectBinding;
            reflectBinding._binding         =   binding->binding;
            reflectBinding._descriptorType  =   static_cast<FEDescType>(binding->descriptor_type);
            reflectBinding._stageFlags      =   stageFlags;
            reflectBinding._name            =   binding->name ? binding->name : "";
            if (reflectBinding._name.empty() && binding->block.type_description && binding->block.type_description->members)
            {
                reflectBinding._name        =   binding->block.type_description->members->struct_member_name;
            }
            _reflectData._bindings.push_back(reflectBinding);
        }

        // Reflect push constants
        uint32_t pushConstCount = 0;
        result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstCount, nullptr);
        if (result == SPV_REFLECT_RESULT_SUCCESS && pushConstCount > 0)
        {
            SRConstants pushConsts(pushConstCount);
            result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstCount, pushConsts.data());
        }
        spvReflectDestroyShaderModule(&module);
    }

    DSetLayout WGShader::createLayoutFromReflect()
    {
        if (_reflectData._bindings.empty())
            return nullptr;

        FEDSetLayout::CreateInfo info;
        for (const auto& binding : _reflectData._bindings)
        {
            FEDSetBinding layoutBinding;
            layoutBinding._binding          =   binding._binding;
            layoutBinding._descriptorType   =   binding._descriptorType;
            layoutBinding._stageFlags       =   _reflectData._stageFlags;
            layoutBinding._name             =   binding._name;
            info._bindings.push_back(layoutBinding);
        }

        auto layout = new WGDSetLayout(_ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }
}

#include    "WGDSetLayout.h"
#include    "WGDevice.h"
#include    "WGDSet.h"
#include    "graphic/FEGraphicEnums.h"

namespace   FE
{
    WGDSetLayout::~WGDSetLayout()
    {
        if (_native)
        {
            wgpuBindGroupLayoutRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGDSetLayout::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        std::vector<WGPUBindGroupLayoutEntry> entries;
        for (const auto& binding : cInf._bindings)
        {
            WGPUBindGroupLayoutEntry entry = {};
            entry.nextInChain =   nullptr;
            entry.binding =   binding._binding;

            uint32_t shaderStage =   0;
            if (binding._stageFlags.hasFlag(ST_VERTEX_BIT))
                shaderStage |=   WGPUShaderStage_Vertex;
            if (binding._stageFlags.hasFlag(ST_FRAGMENT_BIT))
                shaderStage |=   WGPUShaderStage_Fragment;
            entry.visibility =   shaderStage;

            switch (binding._descriptorType)
            {
            case DT_UNIFORM_BUFFER:
                entry.buffer.type =   WGPUBufferBindingType_Uniform;
                break;
            case DT_STORAGE_BUFFER:
                entry.buffer.type =   WGPUBufferBindingType_Storage;
                break;
            case DT_SAMPLED_IMAGE:
                entry.texture.sampleType =   WGPUTextureSampleType_Float;
                break;
            case DT_SAMPLER:
                entry.sampler.type =   WGPUSamplerBindingType_Filtering;
                break;
            default:
                entry.buffer.type =   WGPUBufferBindingType_Uniform;
                break;
            }

            entries.push_back(entry);
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = {};
        layoutDesc.nextInChain =   nullptr;
        layoutDesc.entryCount =   (uint32_t)entries.size();
        layoutDesc.entries =   entries.empty() ? nullptr : entries.data();

        _native =   wgpuDeviceCreateBindGroupLayout(wgDevice.device(),&layoutDesc);
        return _native != nullptr;
    }

    DSet WGDSetLayout::createDSet()
    {
        auto dset = new WGDSet(_ctx);
        FEDSet::CreateInfo info;
        info._layout =   this;
        if (dset->create(info))
        {
            return dset;
        }
        delete dset;
        return nullptr;
    }
}
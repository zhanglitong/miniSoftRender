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
        auto&   wgDevice =  (WGDevice&)(_ctx.device());

        std::vector<WGPUBindGroupLayoutEntry> entries;
        for (const auto& binding : cInf._bindings)
        {
            WGPUBindGroupLayoutEntry entry = {};
            entry.nextInChain      =   nullptr;
            entry.binding          =   binding._binding;
            entry.bindingArraySize =   binding._descriptorCount == 0 ? 1 : binding._descriptorCount;

            entry.visibility        =   WGPUShaderStage_None;
            if (binding._stageFlags.hasFlag(ST_VERTEX_BIT))
                entry.visibility |=   WGPUShaderStage_Vertex;
            if (binding._stageFlags.hasFlag(ST_FRAGMENT_BIT))
                entry.visibility |=   WGPUShaderStage_Fragment;
            if (binding._stageFlags.hasFlag(ST_COMPUTE_BIT))
                entry.visibility |=   WGPUShaderStage_Compute;

            switch (binding._descriptorType)
            {
            case DT_UNIFORM_BUFFER:
                entry.buffer.nextInChain      =   nullptr;
                entry.buffer.type             =   WGPUBufferBindingType_Uniform;
                entry.buffer.hasDynamicOffset =   WGPU_FALSE;
                entry.buffer.minBindingSize   =   0;
                break;
            case DT_UNIFORM_BUFFER_DYNAMIC:
                entry.buffer.nextInChain      =   nullptr;
                entry.buffer.type             =   WGPUBufferBindingType_Uniform;
                entry.buffer.hasDynamicOffset =   WGPU_TRUE;
                entry.buffer.minBindingSize   =   0;
                break;
            case DT_STORAGE_BUFFER:
                entry.buffer.nextInChain      =   nullptr;
                entry.buffer.type             =   WGPUBufferBindingType_Storage;
                entry.buffer.hasDynamicOffset =   WGPU_FALSE;
                entry.buffer.minBindingSize   =   0;
                break;
            case DT_STORAGE_BUFFER_DYNAMIC:
                entry.buffer.nextInChain      =   nullptr;
                entry.buffer.type             =   WGPUBufferBindingType_Storage;
                entry.buffer.hasDynamicOffset =   WGPU_TRUE;
                entry.buffer.minBindingSize   =   0;
                break;
            case DT_SAMPLED_IMAGE:
                entry.texture.nextInChain    =   nullptr;
                entry.texture.sampleType     =   WGPUTextureSampleType_Float;
                entry.texture.viewDimension  =   WGPUTextureViewDimension_2D;
                entry.texture.multisampled   =   WGPU_FALSE;
                break;
            case DT_STORAGE_IMAGE:
                entry.storageTexture.nextInChain   =   nullptr;
                entry.storageTexture.access        =   WGPUStorageTextureAccess_WriteOnly;
                entry.storageTexture.format        =   WGPUTextureFormat_RGBA8Unorm;
                entry.storageTexture.viewDimension =   WGPUTextureViewDimension_2D;
                break;
            case DT_SAMPLER:
                entry.sampler.nextInChain  =   nullptr;
                entry.sampler.type         =   WGPUSamplerBindingType_Filtering;
                break;
            case DT_COMBINED_IMAGE_SAMPLER:
                entry.texture.nextInChain    =   nullptr;
                entry.texture.sampleType     =   WGPUTextureSampleType_Float;
                entry.texture.viewDimension  =   WGPUTextureViewDimension_2D;
                entry.texture.multisampled   =   WGPU_FALSE;
                break;
            }
            entries.push_back(entry);
        }

        WGPUBindGroupLayoutDescriptor layoutDesc = {};
        layoutDesc.nextInChain  =   nullptr;
        layoutDesc.label.data   =   nullptr;
        layoutDesc.label.length =   0;
        layoutDesc.entryCount   =   (uint32_t)entries.size();
        layoutDesc.entries      =   entries.empty() ? nullptr : entries.data();
        /// ErrorScope get  wgpuDeviceCreateBindGroupLayout 's validation 
        WGPUDevice dev = wgDevice.device();
        wgpuDevicePushErrorScope(dev,WGPUErrorFilter_Validation);

        _native =   wgpuDeviceCreateBindGroupLayout(dev,&layoutDesc);

        struct PopResult { WGPUPopErrorScopeStatus status; WGPUErrorType type; std::string msg; };
        auto popCb = [](WGPUPopErrorScopeStatus status,WGPUErrorType type,WGPUStringView msg,void* ud1,void* ud2)
        {
            (void)ud2;
            auto* r = static_cast<PopResult*>(ud1);
            r->status =   status;
            r->type   =   type;
            if (msg.data && msg.length > 0)
                r->msg.assign(msg.data,msg.length);
        };

        PopResult pr;
        WGPUPopErrorScopeCallbackInfo popInfo = {};
        popInfo.nextInChain =   nullptr;
        popInfo.mode        =   WGPUCallbackMode_AllowProcessEvents;
        popInfo.callback    =   popCb;
        popInfo.userdata1   =   &pr;
        popInfo.userdata2   =   nullptr;
        wgpuDevicePopErrorScope(dev,popInfo);

        if (pr.type != WGPUErrorType_NoError)
        {
            printf("[WGPU ErrorScope][BindGroupLayout] type=%d msg=%s\n",(int)pr.type,pr.msg.c_str());
            fflush(stdout);
        }

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

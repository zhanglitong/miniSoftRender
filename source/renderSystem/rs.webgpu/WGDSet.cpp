#include    "WGDSet.h"
#include    "WGDevice.h"
#include    "WGDSetLayout.h"
#include    "WGDSetPool.h"
#include    "WGGPUBuffer.h"
#include    "WGTexture.h"

namespace   FE
{
    WGDSet::~WGDSet()
    {
        if (_native)
        {
            wgpuBindGroupRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGDSet::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        if (!cInf._layout)
            return false;

        auto* wgLayout = const_cast<WGDSetLayout*>(static_cast<const WGDSetLayout*>(cInf._layout.get()));
        if (!wgLayout)
            return false;

        /// 与 Vulkan 后端 (VKDSet::create) 对齐:
        /// 从 layout 的 bindings 预填 _cInfo._binds 的元信息(binding/type/name/typeName),
        /// 这样后续 setBinding(binding, ...) 才能按 _binding 字段查到对应槽位并关联对象。
        const auto& layoutBinds = cInf._layout->cInfo()._bindings;
        if (_cInfo._binds.empty() && !layoutBinds.empty())
        {
            _cInfo._binds.resize(layoutBinds.size());
            for (size_t i = 0; i < layoutBinds.size(); i++)
            {
                _cInfo._binds[i]._binding   =   layoutBinds[i]._binding;
                _cInfo._binds[i]._type      =   layoutBinds[i]._descriptorType;
                _cInfo._binds[i]._name      =   layoutBinds[i]._name;
                _cInfo._binds[i]._typeName   =   layoutBinds[i]._typeName;
            }
        }

        /// WebGPU 的 bind group 是不可变的,创建时必须为每个 binding 提供资源。
        /// 若某个必需 binding 尚未关联对象,推迟 bind group 创建(等待 update() 再次调用),
        /// 避免出现 "Number of bindings in bind group descriptor (0) does not match layout" 验证错误。
        /// 纯 sampler 槽位(如 PBR 的 binding 13)无独立对象,可从已绑定纹理合成,故不参与该检查。
        bool    allBound    =   true;
        for (const auto& binding : _cInfo._binds)
        {
            if (binding._type == DT_SAMPLER)
                continue;
            if (binding._objects.empty())
            {
                allBound    =   false;
                break;
            }
        }
        if (!allBound)
        {
            /// 不创建 bind group,_native 保持为 null,
            /// bindDescriptors 遇到 null bind group 时会跳过绑定。
            return  true;
        }

        /// 任意一个已绑定的纹理:用于为纯 sampler 槽位合成采样器
        const WGTexture*    sharedTex   =   nullptr;
        for (auto& binding : _cInfo._binds)
        {
            for (auto& obj : binding._objects)
            {
                auto* tex = dynamic_cast<WGTexture*>(obj.get());
                if (tex != nullptr) { sharedTex = tex; break; }
            }
            if (sharedTex != nullptr) break;
        }

        std::vector<WGPUBindGroupEntry> entries;
        for (auto& binding : _cInfo._binds)
        {
            const bool  bSampler    =   (binding._type == DT_SAMPLER);
            if (binding._objects.empty() && !bSampler)
                continue;

            WGPUBindGroupEntry entry = {};
            entry.nextInChain =   nullptr;
            entry.binding =   binding._binding;

            switch (binding._type)
            {
            case DT_SAMPLED_IMAGE:
            case DT_COMBINED_IMAGE_SAMPLER:
                {
                    auto* tex = dynamic_cast<WGTexture*>(binding._objects.front().get());
                    if (tex == nullptr || tex->sampler() == nullptr)
                    {
                        LOG_ERR("WGDSet.create: binding %u bind invalid texture object",binding._binding);
                        return  false;
                    }
                    entry.textureView   =   (WGPUTextureView)tex->native();
                }
                break;
            case DT_SAMPLER:
                {
                    WGPUSampler sampler =   nullptr;
                    if (!binding._objects.empty())
                    {
                        auto* tex = dynamic_cast<WGTexture*>(binding._objects.front().get());
                        if (tex != nullptr) sampler = tex->sampler();
                    }
                    if (sampler == nullptr && sharedTex != nullptr)
                        sampler =   sharedTex->sampler();
                    if (sampler == nullptr)
                    {
                        LOG_ERR("WGDSet.create: binding %u has no sampler to bind",binding._binding);
                        return  false;
                    }
                    entry.sampler   =   sampler;
                }
                break;
            default:
                {
                    auto* buffer = dynamic_cast<WGGPUBuffer*>(binding._objects.front().get());
                    if (buffer == nullptr)
                    {
                        LOG_ERR("WGDSet.create: binding %u bind invalid buffer object",binding._binding);
                        return  false;
                    }
                    entry.buffer =   (WGPUBuffer)buffer->native();
                    /// 与 Vulkan 对齐:若绑定时显式给出了 offset/range(如灯光 SSBO 只绑
                    /// 实际使用的 count*stride),则用该子范围绑定。否则 WGSL arrayLength()
                    /// 会返回整个 buffer 的容量(128),对所有空槽各累加一次 emissive,
                    /// 导致颜色过曝变白。layout 的 minBindingSize=0,绑定更小范围合法。
                    entry.offset =   binding._offsets.empty() ? 0 : binding._offsets[0];
                    entry.size   =   binding._ranges.empty()
                                     ? buffer->cInfo()._length
                                     : binding._ranges[0];
                }
                break;
            }

            entries.push_back(entry);
        }

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.nextInChain =   nullptr;
        bindGroupDesc.layout =   (WGPUBindGroupLayout)wgLayout->native();
        bindGroupDesc.entryCount =   (uint32_t)entries.size();
        bindGroupDesc.entries =   entries.empty() ? nullptr : entries.data();

        _native =   wgpuDeviceCreateBindGroup(wgDevice.device(),&bindGroupDesc);
        LOG_DBG("WGDSet.create: bind group created entries=%zu native=%p",entries.size(),(void*)_native);
        return _native != nullptr;
    }

    bool WGDSet::update()
    {
        if (!_dirty)
            return false;
        else
            _dirty =   false;

        if (_native)
        {
            wgpuBindGroupRelease(_native);
            _native =   nullptr;
        }

        return create(_cInfo);
    }
}

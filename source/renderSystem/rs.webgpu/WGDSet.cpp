#include    "WGDSet.h"
#include    "WGDevice.h"
#include    "WGDSetLayout.h"
#include    "WGDSetPool.h"
#include    "WGGPUBuffer.h"

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

        /// WebGPU 的 bind group 是不可变的,创建时必须为每个 binding 提供 buffer。
        /// 若有 binding 尚未关联对象,推迟 bind group 创建(等待 update() 再次调用),
        /// 避免出现 "Number of bindings in bind group descriptor (0) does not match layout" 验证错误。
        bool    allBound    =   true;
        for (const auto& binding : _cInfo._binds)
        {
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

        std::vector<WGPUBindGroupEntry> entries;
        for (const auto& binding : _cInfo._binds)
        {
            if (binding._objects.empty())
                continue;

            WGPUBindGroupEntry entry = {};
            entry.nextInChain =   nullptr;
            entry.binding =   binding._binding;

            auto* buffer = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(binding._objects.front().get()));
            if (buffer)
            {
                entry.buffer =   (WGPUBuffer)buffer->native();
                entry.offset =   0;
                entry.size =   buffer->cInfo()._length;
            }

            entries.push_back(entry);
        }

        WGPUBindGroupDescriptor bindGroupDesc = {};
        bindGroupDesc.nextInChain =   nullptr;
        bindGroupDesc.layout =   (WGPUBindGroupLayout)wgLayout->native();
        bindGroupDesc.entryCount =   (uint32_t)entries.size();
        bindGroupDesc.entries =   entries.empty() ? nullptr : entries.data();

        _native =   wgpuDeviceCreateBindGroup(wgDevice.device(),&bindGroupDesc);
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

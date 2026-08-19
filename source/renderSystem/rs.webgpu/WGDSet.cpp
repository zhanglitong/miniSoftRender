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

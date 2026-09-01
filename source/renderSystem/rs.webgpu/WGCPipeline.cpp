#include    "WGCPipeline.h"
#include    "WGDevice.h"
#include    "WGShader.h"
#include    "WGDSet.h"
#include    "FEWebgpu.hpp"

namespace   FE
{
    WGCPipeline::~WGCPipeline()
    {
        if (_native)
        {
            wgpuComputePipelineRelease(_native);
            _native =   nullptr;
        }
        if (_layout)
        {
            wgpuPipelineLayoutRelease(_layout);
            _layout =   nullptr;
        }
    }

    bool    WGCPipeline::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto&   wgDevice    =   static_cast<const WGDevice&>(_ctx.device());

        /// 1) 收集所有 shader 的 bind group layout
        std::vector<WGPUBindGroupLayout> bindGroupLayouts;
        for (auto shader : info._shaders)
        {
            if (shader == nullptr)
                continue;
            auto    wgShader    =   static_cast<WGShader*>(shader.get());
            if (wgShader == nullptr)
                continue;

            auto    layout  =   wgShader->createLayoutFromReflect();
            if (layout)
            {
                _dsLayouts.push_back(layout);
                auto    wgLayout    =   const_cast<WGDSetLayout*>(static_cast<const WGDSetLayout*>(layout.get()));
                if (wgLayout)
                    bindGroupLayouts.push_back((WGPUBindGroupLayout)wgLayout->native());
            }
        }

        WGPUPipelineLayoutDescriptor layoutDesc = {};
        layoutDesc.nextInChain          =   nullptr;
        layoutDesc.bindGroupLayoutCount =   (uint32_t)bindGroupLayouts.size();
        layoutDesc.bindGroupLayouts     =   bindGroupLayouts.empty() ? nullptr : bindGroupLayouts.data();

        _layout =   wgpuDeviceCreatePipelineLayout(wgDevice.device(),&layoutDesc);
        if (!_layout)
        {
            LOG_ERR("WGCPipeline.create: failed to create pipeline layout");
            return  false;
        }

        /// 2) 找到 compute shader 模块
        WGPUShaderModule csModule    =   nullptr;
        for (const auto& shader : info._shaders)
        {
            if (shader == nullptr)
                continue;
            auto    wgShader =   static_cast<const WGShader*>(shader.get());
            if (wgShader == nullptr)
                continue;
            if (shader->cInfo()._shaderType.hasFlag(ST_COMPUTE_BIT))
            {
                csModule =   (WGPUShaderModule)wgShader->native();
                break;
            }
        }
        if (!csModule)
        {
            LOG_ERR("WGCPipeline.create: missing compute shader");
            return  false;
        }

        /// 3) 创建 WGPUComputePipeline
        WGPUComputePipelineDescriptor pipelineDesc = {};
        pipelineDesc.nextInChain    =   nullptr;
        pipelineDesc.layout         =   _layout;

        WGPUComputeState computeState   =   {};
        computeState.nextInChain    =   nullptr;
        computeState.module         =   csModule;
        /// 入口点名与 WGSL/SPIR-V 中的 fn main 对齐
        computeState.entryPoint     =   { "main",4 };
        computeState.constantCount  =   0;
        computeState.constants      =   nullptr;
        pipelineDesc.compute         =   computeState;

        pipelineDesc.label          =   { nullptr,0 };

        _native =   wgpuDeviceCreateComputePipeline(wgDevice.device(),&pipelineDesc);
        if (_native == nullptr)
        {
            LOG_ERR("WGCPipeline.create: wgpuDeviceCreateComputePipeline failed");
            return  false;
        }
        return  true;
    }

    DSets   WGCPipeline::createDSets()
    {
        DSets dSets;
        if (_dsLayouts.empty())
            return  dSets;

        auto    pool    =   _ctx.device().createDSetPool();
        if (pool)
            _pools.push_back(pool);

        for (auto layout : _dsLayouts)
        {
            auto    dSet    =   layout->createDSet();
            if (dSet)
                dSets.push_back(dSet);
        }
        return  dSets;
    }
}

#include    "WGPipeline.h"
#include    "WGDevice.h"
#include    "WGShader.h"
#include    "WGDSet.h"
#include    "FEWebgpu.hpp"

namespace   FE
{
    WGPipeline::~WGPipeline()
    {
        if (_native)
        {
            wgpuRenderPipelineRelease(_native);
            _native =   nullptr;
        }
        if (_layout)
        {
            wgpuPipelineLayoutRelease(_layout);
            _layout =   nullptr;
        }
    }

    bool    WGPipeline::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto&   wgDevice    =   static_cast<const WGDevice&>(_ctx.device());

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
                {
                    bindGroupLayouts.push_back((WGPUBindGroupLayout)wgLayout->native());
                }
            }
        }

        WGPUPipelineLayoutDescriptor layoutDesc = {};
        layoutDesc.nextInChain          =   nullptr;
        layoutDesc.bindGroupLayoutCount =   (uint32_t)bindGroupLayouts.size();
        layoutDesc.bindGroupLayouts     =   bindGroupLayouts.empty() ? nullptr : bindGroupLayouts.data();

        _layout =   wgpuDeviceCreatePipelineLayout(wgDevice.device(),&layoutDesc);
        if (!_layout)
        {
            LOG_ERR("WGPipeline.create: failed to create pipeline layout");
            return false;
        }

        WGPUShaderModule vsModule =   nullptr;
        WGPUShaderModule fsModule =   nullptr;
        for (const auto& shader : info._shaders)
        {
            if (shader == nullptr)
                continue;
            auto    wgShader =  static_cast<const WGShader*>(shader.get());
            if (wgShader == nullptr)
                continue;
            if (shader->cInfo()._shaderType.hasFlag(ST_VERTEX_BIT))
                vsModule =   (WGPUShaderModule)wgShader->native();
            if (shader->cInfo()._shaderType.hasFlag(ST_FRAGMENT_BIT))
                fsModule =   (WGPUShaderModule)wgShader->native();
        }

        if (!vsModule || !fsModule)
        {
            LOG_ERR("WGPipeline.create: missing vertex or fragment shader");
            return false;
        }

        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.nextInChain    =   nullptr;
        pipelineDesc.layout         =   _layout;

        WGPUVertexState vertexState =   {};
        vertexState.module          =   vsModule;
        vertexState.entryPoint      =   { "vs_main",8 };

        uint32_t    bufferCount =   0;
        WGPUVertexBufferLayout* bufferLayouts =   nullptr;

        vertexState.bufferCount =   bufferCount;
        vertexState.buffers     =   bufferLayouts;
        pipelineDesc.vertex     =   vertexState;

        WGPUPrimitiveState primitiveState = {};
        primitiveState.nextInChain      =   nullptr;
        primitiveState.topology         =   system2Native(info._inputAssemblyState._primitive);
        primitiveState.frontFace        =   WGPUFrontFace_CCW;
        primitiveState.cullMode         =   WGPUCullMode_None;
        primitiveState.unclippedDepth   =   false;

        pipelineDesc.primitive =   primitiveState;

        WGPUMultisampleState multisampleState = {};
        multisampleState.count                  =   1;
        multisampleState.mask                   =   0xFFFFFFFF;
        multisampleState.alphaToCoverageEnabled =   false;
        pipelineDesc.multisample                =   multisampleState;

        WGPUFragmentState fragmentState         =   {};
        fragmentState.module                    =   fsModule;
        fragmentState.entryPoint                =   { "fs_main",8 };

        WGPUBlendState blendState   =   {};
        blendState.color.srcFactor  =   WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor  =   WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation  =   WGPUBlendOperation_Add;
        blendState.alpha.srcFactor  =   WGPUBlendFactor_One;
        blendState.alpha.dstFactor  =   WGPUBlendFactor_Zero;
        blendState.alpha.operation  =   WGPUBlendOperation_Add;

        WGPUColorTargetState colorTarget = {};
        colorTarget.format          =   WGPUTextureFormat_BGRA8Unorm;
        colorTarget.blend           =   &blendState;
        colorTarget.writeMask       =   WGPUColorWriteMask_All;

        fragmentState.targetCount   =   1;
        fragmentState.targets       =   &colorTarget;
        pipelineDesc.fragment       =   &fragmentState;

        WGPUDepthStencilState depthStencilState     =   {};
        depthStencilState.nextInChain               =   nullptr;
        depthStencilState.format                    =   WGPUTextureFormat_Depth24PlusStencil8;
        depthStencilState.depthWriteEnabled         =   WGPUOptionalBool_True;
        depthStencilState.depthCompare              =   WGPUCompareFunction_Less;
        depthStencilState.stencilFront.compare      =   WGPUCompareFunction_Always;
        depthStencilState.stencilFront.failOp       =   WGPUStencilOperation_Keep;
        depthStencilState.stencilFront.depthFailOp  =   WGPUStencilOperation_Keep;
        depthStencilState.stencilFront.passOp       =   WGPUStencilOperation_Keep;
        depthStencilState.stencilBack               =   depthStencilState.stencilFront;

        pipelineDesc.depthStencil                   =   &depthStencilState;

        _native =   wgpuDeviceCreateRenderPipeline(wgDevice.device(),&pipelineDesc);
        assert(_native != nullptr);
        return _native != nullptr;
    }

    DSets   WGPipeline::createDSets()
    {
        DSets dSets;
        if (_dsLayouts.empty())
            return dSets;

        auto pool = _ctx.device().createDSetPool();
        if (pool)
        {
            _pools.push_back(pool);
        }

        for (auto layout : _dsLayouts)
        {
            auto dSet = layout->createDSet();
            if (dSet)
            {
                dSets.push_back(dSet);
            }
        }

        return dSets;
    }
}

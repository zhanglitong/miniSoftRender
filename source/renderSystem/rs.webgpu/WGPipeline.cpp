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

        /// 与 Vulkan 后端 (VKGPipeline / createDSLayoutFromShaders) 对齐:
        /// 将所有 shader stage(VS/FS)的 binding 按 binding 编号合并,stage flags 取并集,
        /// 然后创建单一的 bind group layout 对应 group 0。
        /// WebGPU 的 pipeline layout 中每个 bind group layout 对应一个 group,
        /// VS/FS 共享 group 0,因此必须合并而非各自创建独立 layout(否则 VS 的 binding 0
        /// 会被放到 group 1,与 shader 中 @group(0) @binding(0) 不匹配)。
        auto&   wgDeviceRef =   const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
        auto    layout  =   wgDeviceRef.createDSLayoutFromShaders(info._shaders);
        if (layout)
        {
            _dsLayouts.clear();
            _dsLayouts.push_back(layout);
        }

        WGPUBindGroupLayout bindGroupLayout    =   nullptr;
        if (!_dsLayouts.empty())
        {
            auto    wgLayout    =   const_cast<WGDSetLayout*>(static_cast<const WGDSetLayout*>(_dsLayouts[0].get()));
            if (wgLayout)
                bindGroupLayout    =   (WGPUBindGroupLayout)wgLayout->native();
        }

        /// 汇总各 shader stage 的 var<immediate> 结构体大小,取最大值作为
        /// pipeline layout 的 immediateSize(对应 Vulkan 的 pushConstantRange size)。
        /// 仅 WebGPU Immediates 特性启用时有意义;为 0 时布局不带 immediate 区间。
        uint32_t    immediateSize  =   0;
        for (const auto& shader : info._shaders)
        {
            if (!shader) continue;
            const auto* wgShader =   static_cast<const WGShader*>(shader.get());
            if (!wgShader) continue;
            const auto& imm = wgShader->reflectData()._immediateSize;
            if (imm > immediateSize) immediateSize = imm;
        }

        WGPUPipelineLayoutDescriptor layoutDesc = {};
        layoutDesc.nextInChain          =   nullptr;
        layoutDesc.bindGroupLayoutCount =   bindGroupLayout ? 1 : 0;
        layoutDesc.bindGroupLayouts     =   bindGroupLayout ? &bindGroupLayout : nullptr;
        layoutDesc.immediateSize        =   immediateSize;

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
        /// Vulkan SPIR-V 由 glslc 从 `void main()` 编译,入口点名为 "main"。
        /// WGSL 同样使用 "main" 作为约定入口点。这里与 SPIR-V/WGSL 两种路径都对齐。
        vertexState.entryPoint      =   { "main",4 };

        /// 从 info._binds 构建顶点缓冲布局(WGPUVertexBufferLayout)。
        /// 每个 FEInputBindDesc 对应一个 vertex buffer binding,
        /// 其中的 inputs(向量 FEInputDesc) 为该 binding 下的属性列表。
        /// 与 Vulkan VKGPipeline 的 vertexInputBindings/vertexInputAttributs 对齐。
        std::vector<WGPUVertexBufferLayout>    vbLayouts;
        std::vector<std::vector<WGPUVertexAttribute>>  vbAttrs;
        vbLayouts.reserve(_cInfo._binds.size());
        vbAttrs.resize(_cInfo._binds.size());
        for (size_t i = 0; i < _cInfo._binds.size(); i++)
        {
            const auto& sysBind =   _cInfo._binds[i];
            WGPUVertexBufferLayout vbLayout  =   {};
            vbLayout.arrayStride            =   sysBind.stride;
            vbLayout.stepMode                =   (sysBind.inputRate == V_INPUT_INSTANCE)
                                                ? WGPUVertexStepMode_Instance
                                                : WGPUVertexStepMode_Vertex;

            vbAttrs[i].reserve(sysBind.inputs.size());
            for (const auto& var : sysBind.inputs)
            {
                WGPUVertexAttribute attr    =   {};
                attr.shaderLocation         =   var.location;
                attr.format                 =   vertexFormat2Native(var.format);
                attr.offset                =   var.offset;
                vbAttrs[i].push_back(attr);
            }
            vbLayout.attributeCount     =   (uint32_t)vbAttrs[i].size();
            vbLayout.attributes         =   vbAttrs[i].empty() ? nullptr : vbAttrs[i].data();
            vbLayouts.push_back(vbLayout);
        }

        vertexState.bufferCount =   (uint32_t)vbLayouts.size();
        vertexState.buffers     =   vbLayouts.empty() ? nullptr : vbLayouts.data();
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
        fragmentState.entryPoint                =   { "main",4 };

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
        /// 与渲染目标深度格式对齐:FMT_D32_S8_UNORM -> Depth32FloatStencil8。
        /// 若与渲染 pass 中实际深度附件格式不一致,会触发验证错误:
        /// "Incompatible depth-stencil attachment format"。
        depthStencilState.format                    =   WGPUTextureFormat_Depth32FloatStencil8;
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

#pragma     once

#include    "graphic/FEShader.h"
#include    "graphic/FEDSetLayout.h"
#include    "graphic/FEGraphicEnums.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGShader :public TRSObject<WGPUShaderModule, FEShader>
    {
    public:
        struct ReflectBinding
        {
            uint32_t    _binding;
            FEDescType  _descriptorType;
            uint32_t    _stageFlags;
            String      _name;
            /// WGSL 声明的类型名(如 texture_2d / texture_cube / sampler / uniform 结构体名)。
            /// WGDSetLayout 据此决定纹理视图维度(立方体)与采样类型(深度)等细节。
            String      _typeName;
        };
        using ReflectBindings = std::vector<ReflectBinding>;

        struct ReflectData
        {
            ReflectBindings     _bindings;
            ShaderTypes         _stages;
            uint32_t            _stageFlags    =   0;
            /// var<immediate> 声明的结构体字节大小(WebGPU 原生 push_constant 等价物)。
            /// 由 reflectShaderWGSL 解析 WGSL 结构体布局得到,供 WGPipeline 设置
            /// pipeline layout 的 immediateSize 字段。
            uint32_t            _immediateSize =   0;
            /// 顶点/片元入口点名称。WGSL 要求同一模块内函数名唯一,故入口点未必叫 "main"
            /// (例如 texture 着色器用 vs_main/fs_main),WGPipeline 必须按反射结果传入,
            /// 否则会报 "Unable to find entry point"。
            String              _entryPointVert;
            String              _entryPointFrag;
        };
    public:
        WGShader(FEContext& ctx)
            :TRSObject<WGPUShaderModule, FEShader>(ctx)
        {}
        WGShader(const WGShader& other)
            :TRSObject<WGPUShaderModule, FEShader>(other)
        {}

        virtual ~WGShader();

        virtual bool    create(const CreateInfo& info) override;

        const ReflectData&     reflectData() const
        {
            return _reflectData;
        }
        DSetLayout    createLayoutFromReflect();

    protected:
        WGPUShaderModule    createShaderModule(WGPUDevice device, const uint32_t* source, uint32_t sourceSize);
        /// 通过 WGPUShaderSourceWGSL 创建 shader module,source 为 UTF-8 文本
        WGPUShaderModule    createShaderModuleWGSL(WGPUDevice device, const char* source, size_t length);
        void    reflectShaderSPIRV(const CreateInfo& info);
        /// 简单文本解析:扫描 @group(N) @binding(M) var<...> 提取 binding 信息;
        /// 通过 @compute/@vertex/@fragment 推断 stage。
        void    reflectShaderWGSL(const CreateInfo& info);
        ReflectData    _reflectData;
    };
}

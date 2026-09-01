#include    "WGShader.h"
#include    "WGDevice.h"
#include    "WGDSetLayout.h"
#include    "graphic/spirv-reflect/spirv_reflect.h"
#include    <algorithm>
#include    <string>

namespace
{
    /// 上取整:x 对齐到 a 的倍数(a>0)。
    inline  uint32_t    roundUp(uint32_t a,uint32_t x)
    {
        return  a ? ((x + a - 1) / a * a) : x;
    }

    struct  TypeLayout   { uint32_t    size; uint32_t    align; };

    /// WGSL 内置标量/向量/矩阵类型的 (size,align)。未知类型返回 {0,0}。
    TypeLayout   wgslBuiltinLayout(const std::string& t)
    {
        if (t=="u32"||t=="i32"||t=="f32")            return {4,4};
        if (t=="vec2f"||t=="vec2u"||t=="vec2i")      return {8,8};
        if (t=="vec3f"||t=="vec3u"||t=="vec3i")      return {12,16};
        if (t=="vec4f"||t=="vec4u"||t=="vec4i")      return {16,16};
        if (t=="mat2x2f")                            return {16,16};
        if (t=="mat3x3f")                            return {48,16};
        if (t=="mat4x4f")                            return {64,16};
        return {0,0};
    }

    /// 定位并提取 `struct name { ... }` 的成员体文本。
    bool    wgslStructBody(const std::string& text,const std::string& name,std::string& outBody)
    {
        const std::string kw  =   "struct";
        size_t  pos =   0;
        while ((pos = text.find(kw,pos)) != std::string::npos)
        {
            size_t  p   =   pos + kw.size();
            while (p < text.size() && isspace((unsigned char)text[p])) ++p;
            size_t  ns  =   p;
            while (p < text.size() && (isalnum((unsigned char)text[p]) || text[p]=='_')) ++p;
            if (text.substr(ns,p - ns) != name) { pos = p; continue; }
            size_t  lb  =   text.find('{',p);
            if (lb == std::string::npos) { pos = p; continue; }
            size_t  b   =   lb + 1;
            size_t  e   =   lb + 1;
            int     d   =   1;
            while (e < text.size() && d > 0)
            {
                if (text[e] == '{') ++d;
                else if (text[e] == '}') --d;
                if (d == 0) break;
                ++e;
            }
            outBody =   text.substr(b,e - b);
            return  true;
        }
        return  false;
    }

    /// 计算 WGSL struct 类型的 (size,align),按 host-shareable 地址空间布局:
    /// 成员按各自 align 对齐排布,struct size 上取整到 structAlign(=成员 align 最大值)。
    /// 支持嵌套 struct(递归)。找不到返回 {0,0}。
    TypeLayout   wgslStructLayout(const std::string& text,const std::string& name)
    {
        std::string body;
        if (!wgslStructBody(text,name,body)) return {0,0};

        uint32_t    offset      =   0;
        uint32_t    structAlign =   1;
        size_t      mp          =   0;
        while (mp < body.size())
        {
            /// 跳过分隔/空白
            while (mp < body.size() && (isspace((unsigned char)body[mp]) || body[mp]==',')) ++mp;
            if (mp >= body.size()) break;
            /// 跳过成员名
            while (mp < body.size() && (isalnum((unsigned char)body[mp]) || body[mp]=='_')) ++mp;
            while (mp < body.size() && isspace((unsigned char)body[mp])) ++mp;
            if (mp >= body.size() || body[mp] != ':') { if (mp < body.size()) ++mp; continue; }
            ++mp;   /// 跳过 ':'
            while (mp < body.size() && isspace((unsigned char)body[mp])) ++mp;
            /// 读类型 token:到 ',' / '}' / 空白
            size_t  ts  =   mp;
            while (mp < body.size() && body[mp]!=',' && body[mp]!='}' && !isspace((unsigned char)body[mp])) ++mp;
            std::string type = body.substr(ts,mp - ts);
            if (type.empty()) continue;

            TypeLayout tl = wgslBuiltinLayout(type);
            if (tl.align == 0)  /// 非内置:当作嵌套 struct 递归
                tl = wgslStructLayout(text,type);
            if (tl.align == 0)  continue;   /// 无法识别,跳过

            if (structAlign < tl.align) structAlign = tl.align;
            offset  =   roundUp(tl.align,offset);
            offset  +=  tl.size;
        }
        if (structAlign == 0) return {0,0};
        return { roundUp(structAlign,offset), structAlign };
    }
}

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

        /// SPIR-V 的 magic number:0x07230203 (little-endian 字节序为 03 02 23 07)
        /// 不是 SPIR-V 的就当作 WGSL 文本处理
        const auto* data = static_cast<const uint8_t*>(info._buffer->data());
        size_t      length = info._buffer->length();
        bool        isSPIRV = (length >= 4 &&
                              data[0] == 0x03 && data[1] == 0x02 &&
                              data[2] == 0x23 && data[3] == 0x07);

        if (isSPIRV)
        {
            if (length % sizeof(uint32_t) != 0)
            {
                LOG_ERR("WGShader.create: SPIR-V length not multiple of 4");
                return false;
            }
            const auto* source = reinterpret_cast<const uint32_t*>(data);
            uint32_t sourceSize = static_cast<uint32_t>(length / sizeof(uint32_t));
            _native = createShaderModule(wgDevice.device(), source, sourceSize);
            if (_native)
            {
                reflectShaderSPIRV(info);
                return true;
            }
        }
        else
        {
            /// WGSL 文本:需要保证 null 终止,wgpu 要求 WGPUStringView 指向合法 C 字符串
            std::string sourceText(reinterpret_cast<const char*>(data), length);
            _native = createShaderModuleWGSL(wgDevice.device(), sourceText.c_str(), sourceText.size());
            if (_native)
            {
                reflectShaderWGSL(info);
                /// 用反射结果回填 _cInfo._shaderType,供 WGPipeline/WGCPipeline 选 vs/fs/cs 模块
                _cInfo._shaderType  =   _reflectData._stageFlags;
                LOG_INF("WGShader: created shader module via WGSL");
                return true;
            }
            LOG_ERR("WGShader.create: WGSL shader module creation failed");
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

        LOG_INF("WGShader::reflectShaderSPIRV: stageFlags=0x%x bindingCount=%u",stageFlags,bindingCount);

        for (const auto& binding : bindings)
        {
            ReflectBinding reflectBinding;
            reflectBinding._binding         =   binding->binding;
            reflectBinding._descriptorType  =   static_cast<FEDescType>(binding->descriptor_type);
            /// WebGPU 区分 storage buffer 的 read / read_write 权限,
            /// SPIR-V 中 readonly 限定符对应 NonWritable decoration。
            /// 当 descriptor type 为 STORAGE_BUFFER 且带 NonWritable 标记时,
            /// 改用 DT_STORAGE_BUFFER_READ,映射为 WGPUBufferBindingType_ReadOnlyStorage。
            if (reflectBinding._descriptorType == DT_STORAGE_BUFFER &&
                (binding->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE))
            {
                reflectBinding._descriptorType  =   DT_STORAGE_BUFFER_READ;
            }
            reflectBinding._stageFlags      =   stageFlags;
            reflectBinding._name            =   binding->name ? binding->name : "";
            if (reflectBinding._name.empty() && binding->block.type_description && binding->block.type_description->members)
            {
                reflectBinding._name        =   binding->block.type_description->members->struct_member_name;
            }
            LOG_INF("WGShader::reflectShaderSPIRV: set=%u binding=%u descType=%u name=%s",
                    binding->set,binding->binding,(uint32_t)binding->descriptor_type,
                    reflectBinding._name.c_str());
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
        LOG_INF("WGShader::createLayoutFromReflect: bindings=%zu stageFlags=0x%x",
                _reflectData._bindings.size(),_reflectData._stageFlags);
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

    WGPUShaderModule WGShader::createShaderModuleWGSL(WGPUDevice device, const char* source, size_t length)
    {
        if (device == nullptr || source == nullptr || length == 0)
        {
            LOG_ERR("WGShader::createShaderModuleWGSL: invalid args");
            return nullptr;
        }

        WGPUShaderSourceWGSL wglSource     =   {};
        wglSource.chain.sType              =   WGPUSType_ShaderSourceWGSL;
        wglSource.chain.next               =   nullptr;
        /// WGPUStringView 的 length 不包含 null 终止符
        wglSource.code                     =   { source, length };

        WGPUShaderModuleDescriptor shaderDesc  =   {};
        shaderDesc.nextInChain              =   reinterpret_cast<WGPUChainedStruct*>(&wglSource);
        shaderDesc.label                    =   { nullptr, 0 };

        return  wgpuDeviceCreateShaderModule(device, &shaderDesc);
    }

    void    WGShader::reflectShaderWGSL(const CreateInfo& info)
    {
        _reflectData._bindings.clear();
        _reflectData._stageFlags    =   0;
        _reflectData._stages        =   0;

        const auto* data   =   static_cast<const uint8_t*>(info._buffer->data());
        size_t      length =   info._buffer->length();
        std::string text(reinterpret_cast<const char*>(data), length);

        /// 1) 推断 stage
        if (text.find("@compute")    != std::string::npos)  _reflectData._stageFlags |= ST_COMPUTE_BIT;
        if (text.find("@vertex")     != std::string::npos)  _reflectData._stageFlags |= ST_VERTEX_BIT;
        if (text.find("@fragment")   != std::string::npos)  _reflectData._stageFlags |= ST_FRAGMENT_BIT;
        _reflectData._stages =   _reflectData._stageFlags;

        /// 2) 扫描 @group(N) @binding(M) var<K> name : type; 形式的全局变量
        const std::string groupTag   =   "@group(";
        const std::string bindingTag =   "@binding(";
        const std::string varTag     =   "var";

        size_t pos = 0;
        while ((pos = text.find(groupTag, pos)) != std::string::npos)
        {
            size_t gOpen   = pos + groupTag.size();
            size_t gClose  = text.find(')', gOpen);
            if (gClose == std::string::npos) break;
            int group = atoi(text.c_str() + gOpen);
            pos = gClose + 1;

            /// 紧跟着应该是 @binding(M)
            size_t bPos = text.find(bindingTag, gClose);
            if (bPos == std::string::npos) break;
            size_t bOpen  = bPos + bindingTag.size();
            size_t bClose = text.find(')', bOpen);
            if (bClose == std::string::npos) break;
            int binding = atoi(text.c_str() + bOpen);

            /// 接着找 var<...> 或 var
            size_t varPos = text.find(varTag, bClose);
            if (varPos == std::string::npos) break;

            /// 跳过 var 关键字
            size_t afterVar = varPos + varTag.size();
            /// 解析 var<uniform> / var<storage,read> / var<storage,read_write> / var<storage>
            FEDescType descType = DT_STORAGE_BUFFER;
            if (text[afterVar] == '<')
            {
                size_t close = text.find('>', afterVar);
                if (close != std::string::npos)
                {
                    std::string accessors = text.substr(afterVar + 1, close - afterVar - 1);
                    if (accessors.find("uniform") != std::string::npos)
                        descType = DT_UNIFORM_BUFFER;
                    else if (accessors.find("texture") != std::string::npos)
                        descType = DT_SAMPLED_IMAGE;
                    else
                    {
                        /// storage: 区分 read 与 read_write
                        if (accessors.find("read_write") != std::string::npos)
                            descType = DT_STORAGE_BUFFER;
                        else
                            descType = DT_STORAGE_BUFFER_READ;
                    }
                }
                afterVar = close + 1;
            }

            /// 跳过空白,提取变量名
            while (afterVar < text.size() && isspace((unsigned char)text[afterVar]))
                ++afterVar;
            size_t nameStart = afterVar;
            while (afterVar < text.size() &&
                   (isalnum((unsigned char)text[afterVar]) || text[afterVar] == '_'))
                ++afterVar;
            std::string name = text.substr(nameStart, afterVar - nameStart);

            /// 只接受 group == 0 的 binding(本项目的描述符集约定)
            if (group == 0 && !name.empty())
            {
                ReflectBinding rb;
                rb._binding        =   (uint32_t)binding;
                rb._descriptorType =   descType;
                rb._stageFlags     =   _reflectData._stageFlags;
                rb._name           =   name;
                _reflectData._bindings.push_back(rb);
            }

            pos = afterVar;
        }

        /// 3) 扫描 var<immediate> 声明,计算 immediate 数据字节大小。
        ///    WebGPU 的 immediates 是 push_constant 的原生等价物,
        ///    声明形式为 `var<immediate> name : StructType;`(无 @group/@binding)。
        ///    仅 group 0 / 描述符集约定的 binding 由上一步处理,immediates 不占描述符槽位。
        {
            const std::string immTag  =   "var<immediate>";
            size_t  ip  =   0;
            while ((ip = text.find(immTag,ip)) != std::string::npos)
            {
                size_t  after   =   ip + immTag.size();
                /// 跳过空白,提取变量名
                while (after < text.size() && isspace((unsigned char)text[after])) ++after;
                size_t  vStart  =   after;
                while (after < text.size() && (isalnum((unsigned char)text[after]) || text[after]=='_')) ++after;
                std::string varName = text.substr(vStart,after - vStart);
                /// 定位 ':'
                size_t  colon   =   text.find(':',after);
                if (colon == std::string::npos) { ip = after; continue; }
                size_t  tStart  =   colon + 1;
                while (tStart < text.size() && isspace((unsigned char)text[tStart])) ++tStart;
                /// 类型名:读到 ';'/','/'}'/空白
                size_t  tEnd    =   tStart;
                while (tEnd < text.size() && text[tEnd]!=';' && text[tEnd]!=',' && text[tEnd]!='}' && !isspace((unsigned char)text[tEnd])) ++tEnd;
                std::string typeName = text.substr(tStart,tEnd - tStart);

                TypeLayout tl = wgslStructLayout(text,typeName);
                if (tl.size > _reflectData._immediateSize)
                    _reflectData._immediateSize = tl.size;

                LOG_INF("WGShader::reflectShaderWGSL var<immediate> %s : %s size=%u",
                        varName.c_str(), typeName.c_str(), tl.size);
                ip  =   tEnd;
            }
        }

        LOG_INF("WGShader::reflectShaderWGSL bindings=%zu stage=0x%x immediateSize=%u",
                _reflectData._bindings.size(), _reflectData._stageFlags,_reflectData._immediateSize);
    }
}

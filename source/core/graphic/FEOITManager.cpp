#include    "graphic/FEOITManager.h"
#include    "graphic/FEDevice.h"
#include    "graphic/FECmdBuffer.h"
#include    "mesh/FEPrimitive.hpp"
#include    "FELog.hpp"

namespace   FE
{
    // ===== 全局活动实例/渲染趟状态 =====
    namespace
    {
        FEOITManager*           s_activeOIT =   nullptr;
        FEOITManager::OITPass   s_oitPass   =   FEOITManager::OITPass::NONE;
    }

    void            FEOITManager::setActive(FEOITManager* mgr)  { s_activeOIT = mgr; }
    FEOITManager*   FEOITManager::active()                      { return s_activeOIT; }
    void            FEOITManager::setPass(OITPass pass)         { s_oitPass = pass; }
    FEOITManager::OITPass FEOITManager::pass()                  { return s_oitPass; }

    // ===== OITNode 定义在 FEShaderDefine.h,与 shader std430 布局完全一致 =====
    // uint colorRG (4B) + uint colorBA (4B, half HDR) + float depth (4B) + uint next (4B) = 16B
    static_assert(sizeof(OITNode) == 16, "OITNode size must match shader's std430 layout");

    struct CounterSBO
    {
        uint32_t    _count;
        uint32_t    _maxCount;
        uint32_t    _screenW;
        uint32_t    _screenH;
    };

    FEOITManager::FEOITManager(FEContext& ctx)
        : FEObject(ctx), _ctx(ctx)
    {
    }

    FEOITManager::~FEOITManager()
    {
        // SharedPtr 自动释放
    }

    bool    FEOITManager::init(uint32_t screenW, uint32_t screenH, uint32_t maxNodesPerPixel, uint sampleCount)
    {
        _screenW  = screenW;
        _screenH  = screenH;
        _maxNodes = maxNodesPerPixel * screenW * screenH;

        FEDevice& device = _ctx.device();

        // ===== 1. 创建三个 SSBO =====
        const uint64_t counterSize    =   sizeof(CounterSBO);
        const uint64_t headIndexSize  =   (uint64_t)screenW * screenH * sizeof(uint32_t);
        const uint64_t linkedListSize =   (uint64_t)_maxNodes * sizeof(OITNode);

        auto makeSBO = [&](uint64_t size, const char* logName) -> SBO
        {
            SBO sbo = device.createSBO();
            if (!sbo) { LOG_ERR("OIT: createSBO %s failed", logName); return nullptr; }
            FEGPUBuffer::CreateInfo ci;
            ci._length     = size;
            ci._memUsages  = MemoryUsage::DEVICE_DEFAULT_BIT;
            ci._bufUsages  = BufferUsage::STORAGE_BUFFER_BIT
                           | BufferUsage::TRANSFER_DST_BIT
                           | BufferUsage::TRANSFER_SRC_BIT;
            if (!sbo->create(ci))
            {
                LOG_ERR("OIT: create SBO %s failed (size=%llu)", logName, (unsigned long long)size);
                return nullptr;
            }
            return sbo;
        };

        // Counter
        _counterSBO = makeSBO(counterSize, "counter");

        // HeadIndex
        _headIndexSBO = makeSBO(headIndexSize, "headIndex");

        // LinkedList
        _linkedListSBO = makeSBO(linkedListSize, "linkedList");

        if (!_counterSBO || !_headIndexSBO || !_linkedListSBO)
            return false;

        LOG_INF("OIT: SSBO sizes — counter=%lluB, headIndex=%lluB, linkedList=%lluB (maxNodes=%u)",
                (unsigned long long)counterSize, (unsigned long long)headIndexSize,
                (unsigned long long)linkedListSize, _maxNodes);

        // ===== 2. 写入 counter UBO（count 初始化为 0）=====
        CounterSBO initialCounter = {0, _maxNodes, screenW, screenH};
        _counterSBO->update(&initialCounter, sizeof(CounterSBO), 0);

        // ===== 3. Load Shaders =====
        auto loadShader = [&](const char* path, FEShaderType type) -> Shader
        {
            String absPath = _ctx.resourcePath();
            if (path[0] == ':') absPath += (path + 1);
            else absPath = path;

            auto buf = FEBuffer::loadFile(_ctx, absPath.c_str());
            if (!buf) { LOG_ERR("OIT: load shader %s failed", path); return nullptr; }

            auto shader = device.createShader();
            FEShader::CreateInfo sci;
            sci._shaderType = type;
            sci._buffer     = buf;
            if (!shader->create(sci))
            {
                LOG_ERR("OIT: create shader %s failed", path);
                return nullptr;
            }
            return shader;
        };

        _geometryVS = loadShader(":/data/shader/cache/oit/geometry.vert.spv", ST_VERTEX_BIT);
        _geometryFS = loadShader(":/data/shader/cache/oit/geometry.frag.spv", ST_FRAGMENT_BIT);
        _colorVS    = loadShader(":/data/shader/cache/oit/color.vert.spv",    ST_VERTEX_BIT);
        _colorFS    = loadShader(":/data/shader/cache/oit/color.frag.spv",    ST_FRAGMENT_BIT);

        if (!_geometryVS || !_geometryFS || !_colorVS || !_colorFS)
            return false;

        // ===== 4. Geometry Pass RenderPass（attachmentCount=0）=====
        auto rpFactory = device.createRenderPass();
        FERenderPass::CreateInfo rpCI;
        rpCI._withColor = false;
        rpCI._withDepth = false;
        rpCI._samples   = sampleCount;
        _geometryRP = rpFactory;
        if (!_geometryRP || !_geometryRP->create(rpCI))
        {
            LOG_ERR("OIT: create geometry render pass failed");
            return false;
        }

        // ===== 5. Geometry Pipeline =====
        {
            auto gpipeline = device.createGPipeline();
            FEPipeline::CreateInfo cInfo;
            // 动态渲染(vkCmdBeginRendering)要求管线以 renderPass=VK_NULL_HANDLE 创建,
            // 否则触发 VUID-vkCmdDrawIndexedIndirect-renderPass-06198
            cInfo._renderPass = nullptr;
            cInfo._sampleCount = sampleCount;
            cInfo._shaders    = {_geometryVS, _geometryFS};
            cInfo._inputAssemblyState._primitive = PRI_TRIANGLES;
            cInfo._dynamicStates.push_back(VIEWPORT);
            cInfo._dynamicStates.push_back(SCISSOR);
            /// appDynamicState 每次绘制都会 setPrimitiveTopology/setCullMode/setDepthTest/setLineWidth，
            /// CULL_MODE/DEPTH_TEST_ENABLE/LINE_WIDTH 由 VKGPipeline 自动补全，
            /// 拓扑必须显式声明为动态，否则触发 VUID-08608（静态拓扑+动态设置命令）
            cInfo._dynamicStates.push_back(PRIMITIVE_TOPOLOGY);
            /// 顶点输入布局：与 pbr.xml 完全一致（V3 逐顶点 + 实例属性），
            /// geometry.vert 声明了 location 0~7，缺了会触发 validation 报错
            {
                auto attr = [](uint16_t binding, uint16_t loc, uint32_t off,
                               FEFormat fmt, FEInputSlot slot)
                {
                    FEInputDesc d;
                    d.binding = binding; d.location = loc; d.offset = off;
                    d.format = fmt; d.slot = slot;
                    return d;
                };
                // binding 0: 逐顶点 pos + nor（stride 24）
                FEInputBindDesc b0;
                b0.binding = 0; b0.inputRate = V_INPUT_VERTEX; b0.stride = 24;
                b0.inputs = { attr(0,0,0,  FMT_R32G32B32_FLOAT, IS_VERTEX_POS),
                              attr(0,1,12, FMT_R32G32B32_FLOAT, IS_VERTEX_NOR) };
                // binding 1: 实例 intPart/trans/scale/rot（stride 52）
                FEInputBindDesc b1;
                b1.binding = 1; b1.inputRate = V_INPUT_INSTANCE; b1.stride = 52;
                b1.inputs = { attr(1,2,0,  FMT_R32G32B32_SINT,    IS_INSTANCE_ITRANSLATE),
                              attr(1,3,12, FMT_R32G32B32_FLOAT,   IS_INSTANCE_TRANSLATE),
                              attr(1,4,24, FMT_R32G32B32_FLOAT,   IS_INSTANCE_SCALE),
                              attr(1,5,36, FMT_R32G32B32A32_FLOAT,IS_INSTANCE_ROTATE) };
                // binding 2: 实例 nodeColor（stride 4）
                FEInputBindDesc b2;
                b2.binding = 2; b2.inputRate = V_INPUT_INSTANCE; b2.stride = 4;
                b2.inputs = { attr(2,6,0, FMT_R8G8B8A8_UNORM, IS_INSTANCE_COLOR) };
                // binding 3: 实例 flagBits（stride 4）
                FEInputBindDesc b3;
                b3.binding = 3; b3.inputRate = V_INPUT_INSTANCE; b3.stride = 4;
                b3.inputs = { attr(3,7,0, FMT_R32_UINT, IS_INSTANCE_FLAG) };
                cInfo._binds = { b0, b1, b2, b3 };
            }

            // OIT Geometry Pass 状态
            cInfo._enableDepthTest       = true;   // 不透明物体仍能挡住
            cInfo._enableDepthWrite      = false;  // 透明物体不互相遮挡 → 全部进链表
            cInfo._enableColorBlend      = false;  // 无 color attachment
            cInfo._colorAttachmentCount  = 0;      // 关键！无输出 attachment
            cInfo._cullModeOverride      = 2;      // BACK

            if (!gpipeline->create(cInfo))
            {
                LOG_ERR("OIT: create geometry pipeline failed");
                return false;
            }
            _geometryPipeline = gpipeline;
            _geometryPipeline->setName("OIT/geometry");

            /// 注册到管线注册表：FEMaterialOIT::setup("oitGeometry")
            /// 按 前缀 + "/" + 图元名 查询（如 "oitGeometry/TRIANGLES"）
            auto&   plMgr   =   _ctx.device().pipelines();
            for (uint8 i = 0; i < PRI_MAX; ++i)
            {
                String  plName  =   String("oitGeometry/") + String(nameOfEnum(EPrimitive(i)));
                if (!plMgr.isExist(plName.c_str()))
                    plMgr.add(plName, _geometryPipeline);
            }
        }

        // ===== 6. Color Pipeline =====
        {
            auto cpipeline = device.createGPipeline();
            FEPipeline::CreateInfo cInfo;
            // Color Pass 用正常的 scene render pass（color + depth attachment）
            // 由外部在 draw 时指定正确的 render pass
            cInfo._renderPass = nullptr;  // 会在 scene 的 render pass 里动态渲染
            cInfo._sampleCount = sampleCount;
            cInfo._shaders    = {_colorVS, _colorFS};
            cInfo._inputAssemblyState._primitive = PRI_TRIANGLES;
            cInfo._dynamicStates.push_back(VIEWPORT);
            cInfo._dynamicStates.push_back(SCISSOR);

            // OIT Color Pass 状态
            cInfo._enableDepthTest      = false;  // 合成不做深度测试
            cInfo._enableDepthWrite     = false;
            cInfo._enableColorBlend     = true;   // SrcAlpha / OneMinusSrcAlpha
            cInfo._colorAttachmentCount = 1;
            cInfo._cullModeOverride     = 0;       // NONE（画整个屏幕）

            if (!cpipeline->create(cInfo))
            {
                LOG_ERR("OIT: create color pipeline failed");
                return false;
            }
            _colorPipeline = cpipeline;
            _colorPipeline->setName("OIT/color");
        }

        // ===== 7. Geometry DSets（从 geometry pipeline 的 reflection 创建）=====
        _oitDSets = _geometryPipeline->createDSets();
        if (_oitDSets.empty())
        {
            LOG_ERR("OIT: createDSets from geometry pipeline returned empty");
            return false;
        }

        // 共享的 dummy 材质 UBO：geometry pass 不逐物体换材质，
        // 颜色/透明度走实例属性 nodeColor，这里给个默认 PBRData 兜底
        _dummyMaterialUBO = device.createUBO();
        if (!_dummyMaterialUBO || !_dummyMaterialUBO->create({sizeof(PBRData), MemoryUsage::DEVICE_DEFAULT_BIT}))
        {
            LOG_ERR("OIT: create dummy material UBO failed");
            return false;
        }
        {
            PBRData dummy;
            dummy._diffuse  =   float4(1.0f, 1.0f, 1.0f, 0.5f);
            _dummyMaterialUBO->update(&dummy, sizeof(PBRData), 0);
        }

        auto bindOITBuffers = [&](DSet dset)
        {
            bindToDSet(dset);
        };

        // Geometry pipeline DSets：除 OIT SSBO 外，把 camera/lights/engineState/material 也绑上
        // （与 FEMaterial::autoAttach 同样的按名匹配逻辑）
        {
            DSet dset = _oitDSets[0];
            auto& binds = dset->cInfo()._binds;
            for (auto& bind : binds)
            {
                switch (bind._type)
                {
                case DT_UNIFORM_BUFFER:
                    if (bind._name == "_camera")
                        dset->setBinding(bind._binding, Objects{ _ctx.device().queryCache(FEConstUuid::CameraUBOId) });
                    else if (bind._name == "_engineState")
                        dset->setBinding(bind._binding, Objects{ _ctx.device().queryCache(FEConstUuid::EngineStateUBOId) });
                    else if (bind._objects.empty())
                        dset->setBinding(bind._binding, Objects{ Object(static_cast<FEObject*>(_dummyMaterialUBO.get())) });
                    break;
                case DT_STORAGE_BUFFER:
                    if (bind._name == "_lights")
                        dset->setBinding(bind._binding, Objects{ _ctx.device().queryCache(FEConstUuid::LightsId) });
                    break;
                default:
                    break;
                }
            }
            bindOITBuffers(dset);
        }

        // Color pipeline DSets（color.frag 只用 OIT SSBO）
        _colorDSets = _colorPipeline->createDSets();
        if (_colorDSets.empty())
        {
            LOG_ERR("OIT: createDSets from color pipeline returned empty");
            return false;
        }
        bindOITBuffers(_colorDSets[0]);

        _initialized = true;
        LOG_INF("OITManager initialized: %ux%u, maxNodes=%u", screenW, screenH, _maxNodes);
        return true;
    }

    void    FEOITManager::bindToDSet(DSet dset)
    {
        if (!dset) return;
        // SBO 继承链: FEGPUBuffer→RSObject→FENotify→FEObject
        // 用裸指针构造 Object(FERef<FEObject>)，会自动 incRef
        auto ctrObj = Object(static_cast<FEObject*>(_counterSBO));
        auto hdiObj = Object(static_cast<FEObject*>(_headIndexSBO));
        auto llObj  = Object(static_cast<FEObject*>(_linkedListSBO));
        dset->setBinding(10, Objects{ ctrObj },
                        uint64s{ _counterSBO->cInfo()._length }, uint64s{ 0 });
        dset->setBinding(11, Objects{ hdiObj },
                        uint64s{ _headIndexSBO->cInfo()._length }, uint64s{ 0 });
        dset->setBinding(12, Objects{ llObj },
                        uint64s{ _linkedListSBO->cInfo()._length }, uint64s{ 0 });
        dset->update();
    }

    void    FEOITManager::resetFrame(CMDPtr cmd)
    {
        if (!cmd || !_initialized) return;

        // 1. Counter: fillBuffer 把前 4 字节清 0（count=0）
        cmd->fillBuffer(_counterSBO, 0, sizeof(uint32_t), 0);

        // 2. HeadIndex: 全部填 0xFFFFFFFF（链表空哨兵）
        cmd->fillBuffer(_headIndexSBO, 0,
                        (uint64_t)_screenW * _screenH * sizeof(uint32_t),
                        0xFFFFFFFFu);
    }

    void    FEOITManager::barrierToGeometry(CMDPtr cmd)
    {
        if (!cmd || !_initialized) return;

        // TRANSFER_WRITE → FRAGMENT_SHADER_READ/WRITE
        // （注意：这里是 Vulkan 原始位值，与 VKCmdBuffer::pipelineBarrier 直通约定一致）
        constexpr uint32_t VK_STAGE_TRANSFER          = 0x1000;  // VK_PIPELINE_STAGE_TRANSFER_BIT
        constexpr uint32_t VK_STAGE_FRAGMENT_SHADER   = 0x0080;  // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        constexpr uint32_t VK_ACCESS_TRANSFER_WRITE   = 0x1000;  // VK_ACCESS_TRANSFER_WRITE_BIT
        constexpr uint32_t VK_ACCESS_SHADER_READ      = 0x0020;  // VK_ACCESS_SHADER_READ_BIT
        constexpr uint32_t VK_ACCESS_SHADER_WRITE     = 0x0040;  // VK_ACCESS_SHADER_WRITE_BIT

        cmd->pipelineBarrier(
            VK_STAGE_TRANSFER,
            VK_STAGE_FRAGMENT_SHADER,
            VK_ACCESS_TRANSFER_WRITE,
            VK_ACCESS_SHADER_READ | VK_ACCESS_SHADER_WRITE
        );
    }

    void    FEOITManager::barrierToColor(CMDPtr cmd)
    {
        if (!cmd || !_initialized) return;

        // FRAGMENT_SHADER_WRITE → FRAGMENT_SHADER_READ
        constexpr uint32_t VK_STAGE_FRAGMENT_SHADER   = 0x0080;  // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        constexpr uint32_t VK_ACCESS_SHADER_READ      = 0x0020;  // VK_ACCESS_SHADER_READ_BIT
        constexpr uint32_t VK_ACCESS_SHADER_WRITE     = 0x0040;  // VK_ACCESS_SHADER_WRITE_BIT

        cmd->pipelineBarrier(
            VK_STAGE_FRAGMENT_SHADER,
            VK_STAGE_FRAGMENT_SHADER,
            VK_ACCESS_SHADER_WRITE,
            VK_ACCESS_SHADER_READ
        );
    }

}

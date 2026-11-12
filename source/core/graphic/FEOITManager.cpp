#include    "graphic/FEOITManager.h"
#include    "graphic/FEDevice.h"
#include    "graphic/FECmdBuffer.h"
#include    "graphic/FEConstUuid.h"
#include    "graphic/FEShaderDefine.h"
#include    "mesh/FEPrimitive.hpp"
#include    "FELog.hpp"

namespace   FE
{
    // ===== 全局活动实例 =====
    namespace
    {
        FEOITManager*           s_activeOIT =   nullptr;
    }

    void            FEOITManager::setActive(FEOITManager* mgr)  { s_activeOIT = mgr; }
    FEOITManager*   FEOITManager::active()                      { return s_activeOIT; }

    /// <summary>
    /// 从链表合成透明层:绑定合成管线与 dsets,画全屏三角形
    /// (剔除/深度是动态状态,必须显式设置,否则沿用前面工厂绘制残留的状态)
    /// </summary>
    void            FEOITManager::composite(CMDPtr cmd)
    {
        if (cmd == nullptr || _colorPipeline == nullptr || _colorDSets.empty())
            return;

        cmd->bindPipeline(_colorPipeline);
        cmd->setCullMode(CullMode::CM_NULL);
        cmd->setDepthTest(false);

        FECmdBuffer::DSetBind   dBind;
        dBind.plBindPoint   =   PL_GRAPIC;
        dBind.plLayout      =   _colorPipeline->nativeLayout();
        dBind.firstSet      =   0;
        dBind.dSets         =   _colorDSets;
        cmd->bindDescriptors(dBind);
        cmd->draw(0, 3, 0, 1);  // 全屏三角形(顶点在 VS 里展开)
    }

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
        _screenW     = screenW;
        _screenH     = screenH;
#if OIT_SLOT_K > 0
        /// K 槽模式:每像素固定 K 层,总层数即容量(与 maxNodesPerPixel 无关)
        UNUSED(maxNodesPerPixel);
        _maxNodes    = OIT_SLOT_K * screenW * screenH;
#else
        _maxNodes    = maxNodesPerPixel * screenW * screenH;
#endif
        _sampleCount = sampleCount;
        /// 头索引/层存储按容量预分配(至少 4K):窗口在容量内变化时只更新 counter
        /// 里的屏幕尺寸,不必重建 SSBO(材质已缓存的 OIT dsets 无法重绑)
        _headPixels  = (uint64_t(screenW) * screenH > OIT_HEAD_CAPACITY)
                     ? uint64_t(screenW) * screenH
                     : OIT_HEAD_CAPACITY;

        FEDevice& device = _ctx.device();

        // ===== 1. 创建三个 SSBO =====
        const uint64_t counterSize    =   sizeof(CounterSBO);
#if OIT_SLOT_K > 0
        /// K 槽模式:每像素 1 个 uint(本帧预订到的层数)与 K 个槽(12B/槽)
        const uint64_t headIndexSize  =   _headPixels * sizeof(uint32_t);
        const uint64_t linkedListSize =   _headPixels * OIT_SLOT_K * sizeof(OITSlot);
#else
        const uint64_t headIndexSize  =   _headPixels * sizeof(uint32_t);
        const uint64_t linkedListSize =   (uint64_t)_maxNodes * sizeof(OITNode);
#endif

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
            if (path[0] == ':') 
                absPath += (path + 1);
            else 
                absPath = path;

            auto buf = FEBuffer::loadFile(_ctx, absPath.c_str());
            if (!buf) 
            { 
                LOG_ERR("OIT: load shader %s failed", path); 
                return nullptr; 
            }

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

        _colorVS    = loadShader(":/data/shader/cache/oit/color.vert.spv",    ST_VERTEX_BIT);
        _colorFS    = loadShader(":/data/shader/cache/oit/color.frag.spv",    ST_FRAGMENT_BIT);

        if (!_colorVS || !_colorFS)
            return false;

        // ===== 4. Color Pipeline =====
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
            cInfo._enableColorBlend     = true;   // 预乘 alpha: ONE / OneMinusSrcAlpha
            /// 几何趟写入的是预乘色,color.frag 按剩余透射率加权累加后仍是预乘颜色,
            /// 若按普通 alpha 混合(SRC_ALPHA)会再乘一次 alpha:
            /// 玻璃覆盖度高的地方(轮廓/边缘)被压暗、透明层整体偏弱,
            /// 边缘出现 1px 亮/暗细线
            cInfo._premultipliedAlpha   = true;
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

        // ===== 5. Color DSets（color.frag 只用 OIT SSBO）=====
        _colorDSets = _colorPipeline->createDSets();
        if (_colorDSets.empty())
        {
            LOG_ERR("OIT: createDSets from color pipeline returned empty");
            return false;
        }
        bindToDSet(_colorDSets[0], OIT_BIND_COUNTER, OIT_BIND_HEAD, OIT_BIND_LIST);

        _initialized = true;
        LOG_INF("OITManager initialized: %ux%u, maxNodes=%u", screenW, screenH, _maxNodes);
        return true;
    }

    void    FEOITManager::bindToDSet(DSet dset, uint bindCounter, uint bindHead, uint bindList)
    {
        if (!dset) return;
        // SBO 继承链: FEGPUBuffer→RSObject→FENotify→FEObject
        // 用裸指针构造 Object(FERef<FEObject>)，会自动 incRef
        auto ctrObj = Object(static_cast<FEObject*>(_counterSBO));
        auto hdiObj = Object(static_cast<FEObject*>(_headIndexSBO));
        auto llObj  = Object(static_cast<FEObject*>(_linkedListSBO));
        dset->setBinding(bindCounter, Objects{ ctrObj },
                        uint64s{ _counterSBO->cInfo()._length }, uint64s{ 0 });
        dset->setBinding(bindHead, Objects{ hdiObj },
                        uint64s{ _headIndexSBO->cInfo()._length }, uint64s{ 0 });
        dset->setBinding(bindList, Objects{ llObj },
                        uint64s{ _linkedListSBO->cInfo()._length }, uint64s{ 0 });
        dset->update();
    }

    bool    FEOITManager::resizeScreen(uint32_t screenW, uint32_t screenH)
    {
        if (!_initialized || screenW == 0 || screenH == 0)
            return  false;
        if (uint64_t(screenW) * screenH > _headPixels)
        {
            LOG_ERR("OIT: screen %ux%u exceeds head index capacity %llu pixels",
                    screenW, screenH, (unsigned long long)_headPixels);
            return  false;
        }
        if (screenW == _screenW && screenH == _screenH)
            return  true;
        /// 只改 counter 里的屏幕尺寸:pbr OIT 变体与 color shader 都用它计算
        /// 像素索引;头索引与层池的 GPU 资源保持不变(材质缓存的 dsets 仍然有效)。
        /// update 对 device-local 缓冲是提交一次 transfer(不等待),调用方
        /// 需在下一帧前 waitIdle(FEScene::resize 末尾已保证)
        CounterSBO  counter =   {0,_maxNodes,screenW,screenH};
        _counterSBO->update(&counter, sizeof(counter), 0);
        LOG_INF("OIT: screen resize %ux%u -> %ux%u", _screenW, _screenH, screenW, screenH);
        _screenW    =   screenW;
        _screenH    =   screenH;
        return  true;
    }

    void    FEOITManager::resetFrame(CMDPtr cmd)
    {
        if (!cmd || !_initialized) return;

        // 1. Counter: fillBuffer 把前 4 字节清 0（count=0）
        cmd->fillBuffer(_counterSBO, 0, sizeof(uint32_t), 0);

        // 2. 每像素状态:PPLL 填 0xFFFFFFFF(空链表哨兵);
        //    K 槽模式填 0(每像素层数清零)
#if OIT_SLOT_K > 0
        cmd->fillBuffer(_headIndexSBO, 0,
                        (uint64_t)_screenW * _screenH * sizeof(uint32_t),
                        0u);
#else
        cmd->fillBuffer(_headIndexSBO, 0,
                        (uint64_t)_screenW * _screenH * sizeof(uint32_t),
                        0xFFFFFFFFu);
#endif
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

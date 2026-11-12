#pragma     once

#include    "FEObject.h"
#include    "FECmdBuffer.h"
#include    "FEGraphicEnums.h"
#include    "FEShaderDefine.h"
#include    "FEConstUuid.h"
#include    "FEBuffer.hpp"
#include    "FEDSet.h"
#include    "FEDSetLayout.h"
#include    "FEGPUBuffer.h"
#include    "FEPipeline.h"
#include    "FERenderPass.h"
#include    "FEShader.h"

namespace   FE
{
    /// ===== OIT SSBO binding 常量 =====
    /// oit/color.frag(合成趟专用)固定用 10/11/12;
    /// pbr/shader.frag 的 10~17 已被 PBR 贴图/参数占用,18 被 pbrSkin 顶点
    /// 着色器的 SkinBlock(SB_Skin)占用,其 OIT 变体用 21/22/23
    constexpr   uint    OIT_BIND_COUNTER        =   10;
    constexpr   uint    OIT_BIND_HEAD           =   11;
    constexpr   uint    OIT_BIND_LIST           =   12;
    constexpr   uint    PBR_OIT_BIND_COUNTER    =   21;
    constexpr   uint    PBR_OIT_BIND_HEAD       =   22;
    constexpr   uint    PBR_OIT_BIND_LIST       =   23;

    /// <summary>
    /// 逐像素透明层 OIT 管理器
    ///
    /// 两种存储模式由 OIT_SLOT_K(FEShaderDefine.h)选择:
    ///   OIT_SLOT_K > 0 : 每像素固定 K 个槽,几何趟无锁预订层号、槽内无序,
    ///                    合成趟统一按深度排序(超出 K 的片元丢弃)
    ///   OIT_SLOT_K = 0 : 旧 PPLL 每像素链表(maxNodesPerPixel 决定每像素平均预算)
    ///
    /// 三条 buffer:
    ///   - CounterSBO      : atomic counter + screen dimensions
    ///   - HeadIndexSBO    : PPLL=每像素链表头(W*H uint32);
    ///                       K 槽=每像素已预订层数(W*H uint32)
    ///   - LinkedListSBO   : PPLL=节点池(maxNodes × OITNode);
    ///                       K 槽=槽池(W*H*K × OITSlot)
    ///
    /// 几何趟由 FEMaterialPBR 的 OIT 变体管线直接写这三个 buffer(21/22/23),
    /// 本管理器只负责分配/清零/屏障与合成趟(10/11/12)。
    /// Color Pass: 全屏三角形, depthTest OFF, 预乘 alpha 混合
    /// </summary>
    class   FE_API  FEOITManager : public FEObject
    {
    public:
        FEOITManager(FEContext& ctx);
        ~FEOITManager();

        /// <summary>
        /// 初始化：创建 SSBO、load shader、build pipeline、alloc DSets
        /// @param screenW/screenH 屏幕分辨率，每像素一个 head index
        /// <param name="maxNodesPerPixel">每像素最大透明层数（默认 32）</param>
        /// <param name="sampleCount">渲染目标采样数（MSAA，需与 FEScene 一致）</param>
        /// </summary>
        bool    init(uint32_t screenW, uint32_t screenH, uint32_t maxNodesPerPixel = 32, uint sampleCount = 1);

        /// <summary>
        /// 窗口尺寸变化:容量内只更新 counter 里的屏幕尺寸(shader 用它的
        /// screenW/H 计算像素索引),无需重建 SSBO。
        /// 超出预分配容量返回 false(材质已缓存的 OIT 变体 dsets 无法重绑,
        /// 不能重建 SSBO),调用方应停用 OIT 回退 alpha 混合。
        /// 调用前必须保证 GPU 空闲(不能在帧中间调用)
        /// </summary>
        bool    resizeScreen(uint32_t screenW, uint32_t screenH);

        /// <summary>
        /// 每帧开头调用：清空 counter + headIndex
        /// 在所有透明物体 draw 之前，且在 geometry pass 的 beginRender 之前
        /// </summary>
        void    resetFrame(CMDPtr cmd);

        /// <summary>
        /// Pipeline barrier：reset → Geometry Pass 之间
        /// </summary>
        void    barrierToGeometry(CMDPtr cmd);

        /// <summary>
        /// Pipeline barrier：Geometry Pass → Color Pass 之间
        /// </summary>
        void    barrierToColor(CMDPtr cmd);

        /// <summary>
        /// 把 3 个 OIT SSBO 绑定到指定 DSet 的给定 binding:
        /// 合成趟传 10/11/12(OIT_BIND_*),
        /// pbr OIT 变体管线传 21/22/23(PBR_OIT_BIND_*)
        /// </summary>
        void    bindToDSet(DSet dset,
                           uint bindCounter,
                           uint bindHead,
                           uint bindList);

        // ===== 全局活动实例 =====
        /// <summary>
        /// 设置当前活动 OIT 管理器（场景初始化时调用；
        /// FEMaterialPBR 判定是否走 OIT 变体、并据此绑定 SSBO）
        /// </summary>
        static  void            setActive(FEOITManager* mgr);
        static  FEOITManager*   active();

        // ===== 合成趟 =====
        /// <summary>
        /// 从链表合成透明层:绑定合成管线与 dsets,画全屏三角形。
        /// 调用方负责 beginRender/endRender(合成目标与附件由帧图决定),
        /// 本方法只负责"怎么合成"
        /// </summary>
        void    composite(CMDPtr cmd);

        // ===== 资源访问 =====
        Pipeline    colorPipeline()         const { return _colorPipeline; }
        DSets       colorDSets()            const { return _colorDSets; }

        uint32_t    screenWidth()           const { return _screenW; }
        uint32_t    screenHeight()          const { return _screenH; }
        /// 渲染目标采样数:OIT 变体管线克隆时需与之匹配(由 FEMaterialPBR 使用)
        uint        sampleCount()           const { return _sampleCount; }
        /// 节点池总容量:溢出告警与日志用
        uint32_t    maxNodes()              const { return _maxNodes; }

    protected:
        FEContext&  _ctx;
        uint32_t    _screenW        =   0;
        uint32_t    _screenH        =   0;
        uint32_t    _maxNodes       =   0;
        uint        _sampleCount    =   1;
        /// 头索引缓冲实际容量(像素数),初始化时按 max(W*H, OIT_HEAD_CAPACITY) 定
        uint64_t    _headPixels     =   0;

        /// 每像素头索引缓冲的预分配容量(像素数):按 4K 预留,
        /// 窗口在容量内变化时 OIT 无需重建,只更新 counter 中的屏幕尺寸
        static  constexpr   uint64_t    OIT_HEAD_CAPACITY    =   3840ull * 2160ull;

        // SSBO
        SBO         _counterSBO;    // 4 × uint32: {count, maxCount, screenW, screenH}
        SBO         _headIndexSBO;  // 容量 × uint32(按 OIT_HEAD_CAPACITY 预分配)
        SBO         _linkedListSBO; // 节点池/槽池(maxNodes × OITNode/OITSlot)

        // Pipeline
        Shader      _colorVS;
        Shader      _colorFS;
        Pipeline    _colorPipeline;

        // Descriptor sets
        DSets       _colorDSets;     // Color pipeline 的 DSets（含 3 个 SSBO）

        bool        _initialized    =   false;
    };

    using   OITManager   =   SharedPtr<FEOITManager>;
    using   OITManagers  =   std::vector<OITManager>;
}

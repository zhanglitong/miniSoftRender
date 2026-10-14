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
    /// <summary>
    /// PPLL (Per-Pixel Linked List) OIT 管理器
    ///
    /// 维护每像素链表 + 两条 pipeline:
    ///   - CounterSBO      (binding 10): atomic counter + screen dimensions
    ///   - HeadIndexSBO    (binding 11): 每像素链表头 (W*H uint32)
    ///   - LinkedListSBO   (binding 12): 链表节点池 (maxNodes × OITNode)
    ///
    /// Geometry Pass: attachmentCount=0, depthTest ON, depthWrite OFF
    /// Color Pass:    全屏三角形, depthTest OFF, blend ON (SrcAlpha/OneMinusSrcAlpha)
    /// </summary>
    class   FE_API  FEOITManager : public FEObject
    {
    public:
        /// 当前渲染趟类型:OIT 双趟过滤用
        ///   NONE: 非 OIT 渲染(所有材质正常画)
        ///   MAIN: 主趟,只画非 OIT 材质(不透明)
        ///   LIST: 几何趟,只画 OIT 材质(透明收集进链表)
        enum class  OITPass { NONE, MAIN, LIST };

    public:
        FEOITManager(FEContext& ctx);
        ~FEOITManager();

        /// 初始化：创建 SSBO、load shader、build pipeline、alloc DSets
        /// @param screenW/screenH 屏幕分辨率，每像素一个 head index
        /// @param maxNodesPerPixel  每像素最大透明层数（默认 20）
        bool    init(uint32_t screenW, uint32_t screenH, uint32_t maxNodesPerPixel = 20);

        /// 每帧开头调用：清空 counter + headIndex
        /// 在所有透明物体 draw 之前，且在 geometry pass 的 beginRender 之前
        void    resetFrame(CMDPtr cmd);

        /// Pipeline barrier：reset → Geometry Pass 之间
        void    barrierToGeometry(CMDPtr cmd);

        /// Pipeline barrier：Geometry Pass → Color Pass 之间
        void    barrierToColor(CMDPtr cmd);

        /// 把 3 个 OIT SSBO 绑定到指定 DSet 的 binding 10/11/12
        /// (geometry pass 材质与 color pass 材质共用)
        void    bindToDSet(DSet dset);

        // ===== 全局活动实例/渲染趟状态 =====
        /// 设置当前活动 OIT 管理器（sample 初始化后调用；
        /// FEMaterialOIT 构造时用它来绑定 SSBO）
        static  void            setActive(FEOITManager* mgr);
        static  FEOITManager*   active();
        /// 设置当前渲染趟（FEScene 每趟渲染前切换）
        static  void            setPass(OITPass pass);
        static  OITPass         pass();

        // ===== 资源访问 =====
        RenderPass  geometryRenderPass()    const { return _geometryRP; }
        Pipeline    geometryPipeline()      const { return _geometryPipeline; }
        Pipeline    colorPipeline()         const { return _colorPipeline; }
        DSets       geometryDSets()         const { return _oitDSets; }
        DSets       colorDSets()            const { return _colorDSets; }

        uint32_t    screenWidth()           const { return _screenW; }
        uint32_t    screenHeight()          const { return _screenH; }

    protected:
        FEContext&  _ctx;
        uint32_t    _screenW        =   0;
        uint32_t    _screenH        =   0;
        uint32_t    _maxNodes       =   0;

        // SSBO
        SBO         _counterSBO;    // 4 × uint32: {count, maxCount, screenW, screenH}
        SBO         _headIndexSBO;  // W × H × uint32
        SBO         _linkedListSBO; // maxNodes × sizeof(OITNode)

        // Geometry pass 共享的材质 UBO（geometry pass 不逐物体换材质）
        UBO         _dummyMaterialUBO;

        // Pipeline
        Shader      _geometryVS;
        Shader      _geometryFS;
        Shader      _colorVS;
        Shader      _colorFS;
        Pipeline    _geometryPipeline;
        Pipeline    _colorPipeline;
        RenderPass  _geometryRP;

        // Descriptor sets
        DSets       _oitDSets;       // Geometry pipeline 的 DSets（含 3 个 SSBO）
        DSets       _colorDSets;     // Color pipeline 的 DSets（含 3 个 SSBO，shader reflection 独立）

        bool        _initialized    =   false;
    };

    using   OITManager   =   SharedPtr<FEOITManager>;
    using   OITManagers  =   std::vector<OITManager>;
}

#pragma     once
#include    <variant>
#include    "../FEBuffer.hpp"
#include    "FERenderPass.h"
#include    "FEFrameBuffer.h"
#include    "FEPipeline.h"
#include    "FEDSet.h"
#include    "FETexture.h"
#include    "FEGPUBuffer.h"
#include    "FECmdPool.h"
#include    "../mesh/FEPrimitive.hpp"


namespace   FE
{
    enum    CullMode : uint32_t
    {
        CM_NULL             =   0,      ///< 不丢弃所有面
        CM_FRONT            =   1,      ///< 丢弃正面
        CM_BACK             =   2,      ///< 丢弃背面
        CM_FRONT_AND_BACK   =   3,      ///< 丢弃正反面，即丢弃所有面，与rasterizerDiscardEnable不同，它会执行fs
    };

    class   FEQueue;
    using   Queue       =   SharedPtr<FEQueue>;
    class   FE_API  FECmdBuffer :public RSObject
    {
    public:
        struct  CreateInfo
        {
            CMDPool _pool;
        };

        struct  BeginInfo
        {
            FrameBuffer _frameBuffer;
            RenderPass  _renderPass;
            RectU32     _rect;    
            float4      _clearColor     =   float4(0,0,0,1);
            float       _clearDepth     =   1.0f;
            uint        _clearStencil   =   0;
        };

        struct  RenderInfo
        {
            GImgView    _depth          =   nullptr;
            GImgViews   _colors;
            RectU32     _rect;    
            float4      _clearColor     =   float4(0,0,0,1);
            float       _clearDepth     =   1.0f;
            uint        _clearStencil   =   0;
            /// <summary>
            /// 跳过 color attachment（OIT Geometry Pass 无输出场景）
            /// </summary>
            bool        _skipColor      =   false;
            /// <summary>
            /// 跳过 depth attachment
            /// </summary>
            bool        _skipDepth      =   false;
            /// <summary>
            /// color attachment 用 LOAD 而非 CLEAR（OIT 合成趟保留主趟已画内容）
            /// </summary>
            bool        _loadColor      =   false;
            /// <summary>
            /// depth attachment 用 LOAD 而非 CLEAR（OIT Geometry Pass 需保留不透明物体深度做遮挡测试）
            /// </summary>
            bool        _loadDepth      =   false;
        };
        struct  Viewport
        {
            float    x;
            float    y;
            float    width;
            float    height;
            float    minDepth;
            float    maxDepth;
        };

        struct  DSetBind
        {
            PLType      plBindPoint     =   PL_GRAPIC;
            Handle      plLayout        =   nullptr;
            uint        firstSet        =   0;
            DSets       dSets           =   {};
            uint        offsetCount     =   0;
            uint*       offsets         =   nullptr;
        };
        struct  BufferCopy 
        {
            uint64_t    srcOffset;
            uint64_t    dstOffset;
            uint64_t    size;
        };
        using   BufferCopys     =   std::vector<BufferCopy>;
        using   BufferCopyss    =   std::vector<BufferCopys>;
    protected:
        CreateInfo  _cInfo;
    public:
        FECmdBuffer(FEContext& ctx)
            :RSObject(ctx)
        {}
        FECmdBuffer(const FECmdBuffer& other)
            :RSObject(other)
        {}

        virtual bool        create(const CreateInfo& cInf)      =   0;
        virtual FEResult    reset()                             =   0;
        virtual FEResult    begin(bool oneTimeSubmit = false)   =   0;

        virtual FEResult    setViewport(uint first,uint cnt,const Viewport*)    =   0;
        virtual FEResult    setScissor(uint first,uint cnt,const RectU32*)      =   0;
        virtual FEResult    setLineWidth(float width)           =   0;
        virtual FEResult    setCullMode(CullMode m)             =   0;
        virtual FEResult    setDepthTest(bool bEnabled)         =   0;
        virtual FEResult    beginRender(const RenderInfo& rs)   =   0;
        virtual FEResult    endRender(const RenderInfo& rs)     =   0;
         
        virtual FEResult    bindPipeline(Pipeline  pl)          =   0;
        virtual FEResult    bindDescriptors(const DSetBind& dss)=   0;
        virtual FEResult    bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offset)      =   0;
        virtual FEResult    bindVBO(uint first,const VBO&  vbo, uint64 offset)  =   0;
        virtual FEResult    bindIBO(IBO ibo,uint64 offset,FEIndexType typpe)   =   0;
        
        /// <summary>
        /// virtual FEResult    pushPrimitive   (PrimitiveTopology pri) =   0;
        /// </summary>
        virtual FEResult    pushTexture     (FEPipeline* pl,uint16_t set,uint16_t binding,TexPtr    ptr)    =   0;
        virtual FEResult    pushDescriptor  (FEPipeline* pl,uint16_t set,uint16_t binding,DSet      ptr)    =   0;
        virtual FEResult    pushBuffer      (FEPipeline* pl,uint16_t set,uint16_t binding,BufferPtr ptr)    =   0;
        virtual FEResult    pushConstants   (FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data) =   0;
       
        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff) =   0; 

        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges) =   0;
        virtual FEResult    updateBuffer(GPUBuffer,const void* src,uint64 length,uint64 dstOff) =   0;

        /// <summary>
        /// 用 32-bit 常量填充 buffer 的一个区间（OIT 每帧清空 counter / headIndex 用）
        /// </summary>
        virtual FEResult    fillBuffer(GPUBuffer buf, uint64 dstOff, uint64 size, uint32 value) =   0;
        /// <summary>
        /// 插入 pipeline barrier（OIT Geometry→Color 之间保证 SSBO 写入可见）
        /// </summary>
        virtual FEResult    pipelineBarrier(uint32 srcStageMask, uint32 dstStageMask,
                                             uint32 srcAccessMask, uint32 dstAccessMask) =   0;

        /// <summary>
        /// 生成离屏渲染目标纹理的 mip 链(如 transmission 场景折射纹理:
        /// pass1 渲到 mip0 后,逐级 blit downsample 填满其余 mip,
        /// 供 shader 按 roughness 选 lod 采样,实现磨砂玻璃模糊折射)。
        /// 默认空实现:不支持的后端(WebGPU 暂不处理)直接返回成功。
        /// </summary>
        virtual FEResult    generateMips(GImgView /*view*/)
        {
            return  FEResult::ER_SUCCESS;
        }
       
        virtual FEResult    draw  (   uint32_t firstVertex  
                                    , uint32_t vertexCount
                                    , uint32_t firstInstance
                                    , uint32_t instanceCount)   =   0;

        virtual FEResult    drawIndex  (  uint32_t firstIndex
                                        , uint32_t indexCount
                                        , int32_t  vertexOffset
                                        , uint32_t firstInstance
                                        , uint32_t instanceCount
                                        )   =   0;
        virtual FEResult    drawArrayIndirect(      ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    cullCount,
                                                    uint32_t    stride) =   0;
        virtual FEResult    drawIndexedIndirect(    ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    cullCount,
                                                    uint32_t    stride) =   0;

        virtual FEResult    setPrimitiveTopology(EPrimitive pri)    =   0;
        virtual FEResult    dispatch(uint x,uint y,uint z) =   0;
        virtual FEResult    end()   =   0;
        virtual FEResult    submit(Queue)=   0;
    };

    using   CMDPtr  =   SharedPtr<FECmdBuffer>;
    using   CMDPtrs =   std::vector<CMDPtr>;
}

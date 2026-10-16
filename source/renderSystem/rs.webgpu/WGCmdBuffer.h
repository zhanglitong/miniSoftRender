#pragma     once

#include    "graphic/FECmdBuffer.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGCmdBuffer :public TRSObject<WGPUCommandEncoder, FECmdBuffer>
    {
    public:
        WGCmdBuffer(FEContext& ctx)
            :TRSObject<WGPUCommandEncoder, FECmdBuffer>(ctx)
        {}
        WGCmdBuffer(const WGCmdBuffer& other)
            :TRSObject<WGPUCommandEncoder, FECmdBuffer>(other)
        {}

        virtual ~WGCmdBuffer();

        virtual bool        create(const CreateInfo& cInf) override;
        virtual FEResult    reset() override;
        virtual FEResult    begin(bool oneTimeSubmit) override;
        virtual FEResult    setViewport(uint first,uint cnt,const Viewport*) override;
        virtual FEResult    setScissor(uint first,uint cnt,const RectU32*) override;
        virtual FEResult    setLineWidth(float width)   override;
        virtual FEResult    setCullMode(CullMode m)     override;
        virtual FEResult    setDepthTest(bool bEnabled) override;
        virtual FEResult    beginRender(const RenderInfo& rs) override;
        virtual FEResult    endRender(const RenderInfo& rs) override;

        virtual FEResult    bindPipeline(Pipeline pl) override;
        virtual FEResult    bindDescriptors(const DSetBind&) override;
        virtual FEResult    bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offset) override;
        virtual FEResult    bindVBO(uint first,const VBO& vbo,uint64 offset) override;

        virtual FEResult    bindIBO(IBO ibo,uint64 offset,FEIndexType typpe) override;

        virtual FEResult    pushTexture(FEPipeline* pl,uint16_t set,uint16_t binding,TexPtr ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return FEResult::ER_FAILED;
        }
        virtual FEResult    pushDescriptor(FEPipeline* pl,uint16_t set,uint16_t binding,DSet ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return FEResult::ER_FAILED;
        }
        virtual FEResult    pushBuffer(FEPipeline* pl,uint16_t set,uint16_t binding,BufferPtr ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return FEResult::ER_FAILED;
        }
        virtual FEResult    pushConstants(FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data) override;

        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff) override;
        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges) override;

        virtual FEResult    updateBuffer(GPUBuffer,const void* src,uint64 length,uint64 dstOff) override;

        virtual FEResult    fillBuffer(GPUBuffer buf, uint64 dstOff, uint64 size, uint32 value) override
        {
            (void)buf; (void)dstOff; (void)size; (void)value;
            return FEResult::ER_FAILED;  // WebGPU OIT 暂未实现
        }
        virtual FEResult    pipelineBarrier(uint32 srcStageMask, uint32 dstStageMask,
                                             uint32 srcAccessMask, uint32 dstAccessMask) override
        {
            (void)srcStageMask; (void)dstStageMask; (void)srcAccessMask; (void)dstAccessMask;
            return FEResult::ER_FAILED;  // WebGPU OIT 暂未实现
        }

        virtual FEResult    draw(uint32_t firstVertex,uint32_t vertexCount,uint32_t firstInstance,uint32_t instanceCount) override;

        virtual FEResult    drawIndex(uint32_t firstIndex,uint32_t indexCount,int32_t vertexOffset,uint32_t firstInstance,uint32_t instanceCount) override;
        virtual FEResult    drawArrayIndirect(ITO buffer,uint64 offset,uint32_t drawCount,uint32_t stride) override;
        virtual FEResult    drawIndexedIndirect(ITO buffer,uint64 offset,uint32_t drawCount,uint32_t stride) override;

        virtual FEResult    setPrimitiveTopology(EPrimitive pri) override;
        virtual FEResult    dispatch(uint x,uint y,uint z) override;

        virtual FEResult    end() override;
        virtual FEResult    submit(Queue) override;

    public:
        WGPURenderPassEncoder       _renderPassEncoder      =   nullptr;
        WGPUComputePassEncoder      _computePassEncoder     =   nullptr;
        WGPURenderPipeline          _currentPipeline        =   nullptr;
        WGPUComputePipeline         _currentComputePipeline =   nullptr;
        WGPUBindGroup               _currentBindGroup       =   nullptr;
        std::vector<WGPUBindGroup>  _bindGroups;
    protected:
        /// WebGPU 要求管线声明的每个 vertex buffer slot 在 draw 前都被绑定,
        /// 且绑定的缓冲必须能覆盖本次 draw 的顶点/实例范围;
        /// Vulkan 允许未绑定或过小的 slot(越界读取返回零)。
        /// 文本等网格不含实例属性(引擎只绑定 slot 0),其管线声明的实例 slot
        /// 会以空缓冲被绑定,故在 draw 前用零缓冲补齐/覆盖不满足覆盖要求的 slot,
        /// 行为与 Vulkan 对齐,避免
        /// "Instance N extends beyond limit M imposed by the buffer in slot S"。
        std::vector<uint8_t>    _vbSlotBound;       ///< 每个 slot 是否已绑定
        std::vector<uint8_t>    _vbSlotInstance;    ///< 每个 slot 是否为 instance 步进
        std::vector<uint32_t>   _vbSlotStride;      ///< 每个 slot 的 stride
        std::vector<uint64_t>   _vbSlotSize;        ///< 每个 slot 已绑定缓冲的大小
        WGPUBuffer              _dummyVB        =   nullptr;
        uint64_t                _dummyVBSize    =   0;
        /// 在 draw 前补齐/覆盖未绑定或覆盖范围不足的顶点槽位(零缓冲)
        void    ensureVertexSlots(uint32 firstInstance,uint32 instanceCount);
    };
}

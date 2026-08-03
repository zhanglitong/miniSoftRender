#pragma     once

#include    "graphic/FECmdBuffer.h"

namespace   FE
{
    class   SWCmdBuffer :public TRSObject<void*,FECmdBuffer>
    {
    public:
        SWCmdBuffer(FEContext& ctx)
            :TRSObject<void*,FECmdBuffer>(ctx)
        {
        }
        SWCmdBuffer(const SWCmdBuffer& other)
            :TRSObject<void*,FECmdBuffer>(other)
        {}

        virtual ~SWCmdBuffer();

        virtual bool        create(const CreateInfo& cInf)  override;
        virtual FEResult    reset()                         override;
        virtual FEResult    begin(bool oneTimeSubmit)       override;
        virtual FEResult    setViewport(uint first,uint cnt,const Viewport*)    override;
        virtual FEResult    setScissor(uint first,uint cnt,const RectU32*)      override;

        virtual FEResult    beginRenderPass(BeginInfo& rs)  override;
        virtual FEResult    endRenderPass()                 override;

        virtual FEResult    bindPipeline(Pipeline  pl)      override;
        virtual FEResult    bindDescriptors(const DSetBind&)override;
        virtual FEResult    bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offset)    override;
        virtual FEResult    bindVBO(uint first,const VBO&  vbo,uint64 offset)  override;

        virtual FEResult    bindIBO(IBO ibo,uint64 offset,FEIndexType typpe) override;

        virtual FEResult    pushTexture     (FEPipeline* pl,uint16_t set,uint16_t binding,TexPtr    ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return  FEResult::ER_FAILED;
        }
        virtual FEResult    pushDescriptor  (FEPipeline* pl,uint16_t set,uint16_t binding,DSet      ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return  FEResult::ER_FAILED;
        }
        virtual FEResult    pushBuffer      (FEPipeline* pl,uint16_t set,uint16_t binding,BufferPtr ptr) override
        {
            (void)pl;
            (void)set;
            (void)binding;
            (void)ptr;
            return  FEResult::ER_FAILED;
        }
        virtual FEResult    pushConstants   (FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data)  override;

        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff)    override;
        virtual FEResult    copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges) override;

        virtual FEResult    updateBuffer(GPUBuffer,const void* src,uint64 length,uint64 dstOff) override;

        virtual FEResult    draw  (   uint32_t firstVertex
                                    , uint32_t vertexCount
                                    , uint32_t firstInstance
                                    , uint32_t instanceCount) override;

        virtual FEResult    drawIndex  (  uint32_t firstIndex
                                        , uint32_t indexCount
                                        , int32_t  vertexOffset
                                        , uint32_t firstInstance
                                        , uint32_t instanceCount
                                        ) override;
        virtual FEResult    drawArrayIndirect(      ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride) override;
        virtual FEResult    drawIndexedIndirect(    ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride) override;

        virtual FEResult    setPrimitiveTopology(EPrimitive pri)  override;

        virtual FEResult    end() override;
        virtual FEResult    submit(Queue) override;
    };
}

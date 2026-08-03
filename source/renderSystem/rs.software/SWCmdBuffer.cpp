#include    "SWCmdBuffer.h"
#include    "SWDevice.h"
#include    "SWCmdPool.h"
#include    "SWGPUBuffer.h"
#include    "SWQueue.h"

namespace   FE
{
    SWCmdBuffer::~SWCmdBuffer()
    {
    }

    bool    SWCmdBuffer::create(const FECmdBuffer::CreateInfo& cInf)
    {
        _cInfo  =   cInf;
        return  true;
    }

    FEResult    SWCmdBuffer::reset()
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::begin(bool oneTimeSubmit)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setViewport(uint first,uint cnt,const Viewport* vps)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setScissor(uint first,uint cnt,const RectU32* rects)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::beginRenderPass(BeginInfo& info)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::endRenderPass()
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindPipeline(Pipeline pl)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindDescriptors(const DSetBind& info)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offsets)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindVBO(uint first,const VBO& vbo,uint64 offset)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindIBO(IBO ibo,uint64 offset,FEIndexType type)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::pushConstants(FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::updateBuffer(GPUBuffer dstBuf,const void* data,uint64 length,uint64 dstOff)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::draw  (   uint32_t firstVertex
                                    , uint32_t vertexCount
                                    , uint32_t firstInstance
                                    , uint32_t instanceCount)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawIndex  ( uint32_t firstIndex
                                        , uint32_t indexCount
                                        , int32_t  vertexOffset
                                        , uint32_t firstInstance
                                        , uint32_t instanceCount)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawArrayIndirect(      ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawIndexedIndirect(   ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setPrimitiveTopology(EPrimitive pri)
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::end()
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::submit(Queue queue)
    {
        return  FEResult::ER_SUCCESS;
    }
}

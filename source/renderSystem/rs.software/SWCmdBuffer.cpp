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
        (void)oneTimeSubmit;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setViewport(uint first,uint cnt,const Viewport* vps)
    {
        (void)first;
        (void)cnt;
        (void)vps;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setScissor(uint first,uint cnt,const RectU32* rects)
    {
        (void)first;
        (void)cnt;
        (void)rects;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::beginRender(const RenderInfo& rs)
    {
        (void)rs;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::endRender(const RenderInfo& )
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindPipeline(Pipeline pl)
    {
        (void)pl;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindDescriptors(const DSetBind& info)
    {
        (void)info;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offsets)
    {
        (void)first;
        (void)cnt;
        (void)vbos;
        (void)offsets;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindVBO(uint first,const VBO& vbo,uint64 offset)
    {
        (void)first;
        (void)vbo;
        (void)offset;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::bindIBO(IBO ibo,uint64 offset,FEIndexType type)
    {
        (void)ibo;
        (void)offset;
        (void)type;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::pushConstants(FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data)
    {
        (void)pl;
        (void)shaderBits;
        (void)offset;
        (void)size;
        (void)data;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff)
    {
        (void)srcBuf;
        (void)dstBuf;
        (void)length;
        (void)srcOff;
        (void)dstOff;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges)
    {
        (void)srcBuf;
        (void)dstBuf;
        (void)ranges;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::updateBuffer(GPUBuffer dstBuf,const void* data,uint64 length,uint64 dstOff)
    {
        (void)dstBuf;
        (void)data;
        (void)length;
        (void)dstOff;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::draw  (   uint32_t firstVertex
                                    , uint32_t vertexCount
                                    , uint32_t firstInstance
                                    , uint32_t instanceCount)
    {
        (void)firstVertex;
        (void)vertexCount;
        (void)firstInstance;
        (void)instanceCount;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawIndex  ( uint32_t firstIndex
                                        , uint32_t indexCount
                                        , int32_t  vertexOffset
                                        , uint32_t firstInstance
                                        , uint32_t instanceCount)
    {
        (void)firstIndex;
        (void)indexCount;
        (void)vertexOffset;
        (void)firstInstance;
        (void)instanceCount;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawArrayIndirect(      ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        (void)buffer;
        (void)offset;
        (void)drawCount;
        (void)stride;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::drawIndexedIndirect(   ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        (void)buffer;
        (void)offset;
        (void)drawCount;
        (void)stride;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::setPrimitiveTopology(EPrimitive pri)
    {
        (void)pri;
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::end()
    {
        return  FEResult::ER_SUCCESS;
    }

    FEResult    SWCmdBuffer::submit(Queue queue)
    {
        (void)queue;
        return  FEResult::ER_SUCCESS;
    }
}

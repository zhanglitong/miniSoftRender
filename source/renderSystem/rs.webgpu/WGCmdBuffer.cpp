#include    "WGCmdBuffer.h"
#include    "WGDevice.h"
#include    "WGGPUBuffer.h"
#include    "WGPipeline.h"
#include    "WGDSet.h"
#include    "WGSemaphore.h"
#include    "WGFence.h"

namespace   FE
{
    WGCmdBuffer::~WGCmdBuffer()
    {
        if (_native)
        {
            wgpuCommandBufferRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGCmdBuffer::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        return true;
    }

    FEResult WGCmdBuffer::reset()
    {
        if (_native)
        {
            wgpuCommandBufferRelease(_native);
            _native =   nullptr;
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::begin(bool oneTimeSubmit)
    {
        (void)oneTimeSubmit;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        WGPUCommandEncoderDescriptor encoderDesc;
        encoderDesc.label = { "Command Encoder",15 };

        _encoder =   wgpuDeviceCreateCommandEncoder(wgDevice.device(),&encoderDesc);
        if (!_encoder)
            return FEResult::ER_FAILED;

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::setViewport(uint first,uint cnt,const Viewport* viewports)
    {
        (void)first;
        (void)cnt;
        (void)viewports;
        if (!_encoder)
            return FEResult::ER_FAILED;
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::setScissor(uint first,uint cnt,const RectU32* scissors)
    {
        (void)first;
        (void)cnt;
        (void)scissors;
        if (!_encoder)
            return FEResult::ER_FAILED;
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::beginRender(const RenderInfo& rs)
    {
        if (!_encoder)
            return FEResult::ER_FAILED;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain =   nullptr;

        WGPUTextureView attachments[1] = {};
        WGPUTextureView depthAttachment =   nullptr;

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view =   attachments[0];
        colorAttachment.resolveTarget =   nullptr;
        colorAttachment.clearValue = { rs._clearColor.x,rs._clearColor.y,rs._clearColor.z,rs._clearColor.w };
        colorAttachment.loadOp =   WGPULoadOp_Clear;
        colorAttachment.storeOp =   WGPUStoreOp_Store;

        renderPassDesc.colorAttachmentCount =   1;
        renderPassDesc.colorAttachments =   &colorAttachment;

        WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
        depthStencilAttachment.view =   depthAttachment;
        depthStencilAttachment.depthClearValue =   rs._clearDepth;
        depthStencilAttachment.depthLoadOp =   WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp =   WGPUStoreOp_Store;
        depthStencilAttachment.stencilClearValue =   rs._clearStencil;
        depthStencilAttachment.stencilLoadOp =   WGPULoadOp_Clear;
        depthStencilAttachment.stencilStoreOp =   WGPUStoreOp_Store;

        renderPassDesc.depthStencilAttachment =   &depthStencilAttachment;

        _renderPassEncoder =   wgpuCommandEncoderBeginRenderPass(_encoder,&renderPassDesc);
        if (!_renderPassEncoder)
            return FEResult::ER_FAILED;

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::endRender(const RenderInfo& )
    {
        if (_renderPassEncoder)
        {
            wgpuRenderPassEncoderEnd(_renderPassEncoder);
            _renderPassEncoder =   nullptr;
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::bindPipeline(Pipeline pl)
    {
        if (!_renderPassEncoder || !pl)
            return FEResult::ER_FAILED;

        auto* wgPipeline = const_cast<WGPipeline*>(static_cast<const WGPipeline*>(pl.get()));
        if (wgPipeline)
        {
            _currentPipeline =   (WGPURenderPipeline)wgPipeline->native();
            wgpuRenderPassEncoderSetPipeline(_renderPassEncoder,_currentPipeline);
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::bindDescriptors(const DSetBind& dss)
    {
        if (!_renderPassEncoder)
            return FEResult::ER_FAILED;

        for (size_t i = 0;i < dss.dSets.size();i++)
        {
            if (dss.dSets[i])
            {
                auto* wgSet = const_cast<WGDSet*>(static_cast<const WGDSet*>(dss.dSets[i].get()));
                if (wgSet)
                {
                    WGPUBindGroup bindGroup =   (WGPUBindGroup)wgSet->native();
                    wgpuRenderPassEncoderSetBindGroup(_renderPassEncoder,dss.firstSet + (uint32_t)i,bindGroup,0,nullptr);
                    _bindGroups.push_back(bindGroup);
                }
            }
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offset)
    {
        if (!_renderPassEncoder)
            return FEResult::ER_FAILED;

        for (uint i = 0;i < cnt;i++)
        {
            if (i < vbos.size() && vbos[i])
            {
                auto* wgBuf = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(vbos[i].get()));
                if (wgBuf)
                {
                    uint64_t bufferOffset = i < offset.size() ? offset[i] : 0;
                    uint64_t bufferSize = wgBuf->cInfo()._length;
                    wgpuRenderPassEncoderSetVertexBuffer(_renderPassEncoder,first + i,(WGPUBuffer)wgBuf->native(),bufferOffset,bufferSize);
                }
            }
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::bindVBO(uint first,const VBO& vbo,uint64 offset)
    {
        if (!_renderPassEncoder || !vbo)
            return FEResult::ER_FAILED;

        auto* wgBuf = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(vbo.get()));
        if (wgBuf)
        {
            uint64_t bufferSize = wgBuf->cInfo()._length;
            wgpuRenderPassEncoderSetVertexBuffer(_renderPassEncoder,first,(WGPUBuffer)wgBuf->native(),offset,bufferSize);
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::bindIBO(IBO ibo,uint64 offset,FEIndexType typpe)
    {
        if (!_renderPassEncoder || !ibo)
            return FEResult::ER_FAILED;

        auto* wgBuf = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(ibo.get()));
        if (wgBuf)
        {
            WGPUIndexFormat indexFormat =   (typpe == INDEX_UINT16)
                ? WGPUIndexFormat_Uint16
                : WGPUIndexFormat_Uint32;
            uint64_t bufferSize = wgBuf->cInfo()._length;
            wgpuRenderPassEncoderSetIndexBuffer(_renderPassEncoder,(WGPUBuffer)wgBuf->native(),indexFormat,offset,bufferSize);
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::pushConstants(FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data)
    {
        (void)pl;
        (void)shaderBits;
        (void)offset;
        (void)size;
        (void)data;
        return FEResult::ER_FAILED;
    }

    FEResult WGCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff)
    {
        if (!_encoder || !srcBuf || !dstBuf)
            return FEResult::ER_FAILED;

        auto* srcWg = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(srcBuf.get()));
        auto* dstWg = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(dstBuf.get()));
        if (srcWg && dstWg)
        {
            wgpuCommandEncoderCopyBufferToBuffer(_encoder,
                (WGPUBuffer)srcWg->native(),srcOff,
                (WGPUBuffer)dstWg->native(),dstOff,
                length);
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges)
    {
        for (const auto& range : ranges)
        {
            auto result = copyBuffer(srcBuf,dstBuf,range.size,range.srcOffset,range.dstOffset);
            if (result != FEResult::ER_SUCCESS)
                return result;
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::updateBuffer(GPUBuffer dstBuf,const void* src,uint64 length,uint64 dstOff)
    {
        if (!dstBuf || !src)
            return FEResult::ER_FAILED;

        auto* dstWg = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(dstBuf.get()));
        if (dstWg)
        {
            auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));
            auto queue = wgDevice.queue();
            wgpuQueueWriteBuffer(queue,(WGPUBuffer)dstWg->native(),dstOff,src,length);
        }

        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::draw(uint32_t firstVertex,uint32_t vertexCount,uint32_t firstInstance,uint32_t instanceCount)
    {
        if (!_renderPassEncoder)
            return FEResult::ER_FAILED;

        wgpuRenderPassEncoderDraw(_renderPassEncoder,vertexCount,instanceCount,firstVertex,firstInstance);
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::drawIndex(uint32_t firstIndex,uint32_t indexCount,int32_t vertexOffset,uint32_t firstInstance,uint32_t instanceCount)
    {
        if (!_renderPassEncoder)
            return FEResult::ER_FAILED;

        wgpuRenderPassEncoderDrawIndexed(_renderPassEncoder,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::drawArrayIndirect(ITO buffer,uint64 offset,uint32_t drawCount,uint32_t stride)
    {
        (void)drawCount;
        (void)stride;
        if (!_renderPassEncoder || !buffer)
            return FEResult::ER_FAILED;

        auto* wgBuf = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(buffer.get()));
        if (wgBuf)
        {
            wgpuRenderPassEncoderDrawIndirect(_renderPassEncoder,(WGPUBuffer)wgBuf->native(),offset);
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::drawIndexedIndirect(ITO buffer,uint64 offset,uint32_t drawCount,uint32_t stride)
    {
        (void)drawCount;
        (void)stride;
        if (!_renderPassEncoder || !buffer)
            return FEResult::ER_FAILED;

        auto* wgBuf = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(buffer.get()));
        if (wgBuf)
        {
            wgpuRenderPassEncoderDrawIndexedIndirect(_renderPassEncoder,(WGPUBuffer)wgBuf->native(),offset);
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::setPrimitiveTopology(EPrimitive pri)
    {
        (void)pri;
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::end()
    {
        if (_encoder)
        {
            _native =   wgpuCommandEncoderFinish(_encoder,nullptr);
            _encoder =   nullptr;
        }
        return _native ? FEResult::ER_SUCCESS : FEResult::ER_FAILED;
    }

    FEResult WGCmdBuffer::submit(Queue queue)
    {
        if (!_native || !queue)
            return FEResult::ER_FAILED;

        FEQueue::SubmitInfo submitInfo;
        submitInfo._cmds.push_back(this);
        return queue->submit(1,&submitInfo,nullptr) ? FEResult::ER_SUCCESS : FEResult::ER_FAILED;
    }
}
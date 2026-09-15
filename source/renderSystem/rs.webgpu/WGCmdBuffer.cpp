#include    "WGCmdBuffer.h"
#include    "WGDevice.h"
#include    "WGGPUBuffer.h"
#include    "WGPipeline.h"
#include    "WGCPipeline.h"
#include    "WGDSet.h"
#include    "WGSemaphore.h"
#include    "WGFence.h"

namespace   FE
{
    WGCmdBuffer::~WGCmdBuffer()
    {
        if (_computePassEncoder)
        {
            wgpuComputePassEncoderRelease(_computePassEncoder);
            _computePassEncoder    =   nullptr;
        }
        if (_renderPassEncoder)
        {
            wgpuRenderPassEncoderRelease(_renderPassEncoder);
            _renderPassEncoder     =   nullptr;
        }
        if (_native)
        {
            wgpuCommandEncoderRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGCmdBuffer::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;

        auto&   wgDevice    =   (WGDevice&)(_ctx.device());
        _native             =   wgpuDeviceCreateCommandEncoder(wgDevice.device(), nullptr);

        return true;
    }

    FEResult WGCmdBuffer::reset()
    {
        auto&   wgDevice    =   (WGDevice&)(_ctx.device());
        if (_computePassEncoder)
        {
            wgpuComputePassEncoderRelease(_computePassEncoder);
            _computePassEncoder    =   nullptr;
        }
        if (_renderPassEncoder)
        {
            wgpuRenderPassEncoderRelease(_renderPassEncoder);
            _renderPassEncoder     =   nullptr;
        }
        if (_native)
            wgpuCommandEncoderRelease(_native);
        _native     =   wgpuDeviceCreateCommandEncoder(wgDevice.device(), nullptr);
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::begin(bool oneTimeSubmit)
    {
        (void)oneTimeSubmit;
        if (_native == nullptr)
            return FEResult::ER_FAILED;
        else
            return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::setViewport(uint first,uint cnt,const Viewport* viewports)
    {
        (void)first;
        (void)cnt;
        (void)viewports;
        if (viewports)
        {
            auto&   viewport    =   viewports[0];
            wgpuRenderPassEncoderSetViewport(   _renderPassEncoder
                                                ,viewport.x
                                                ,viewport.y
                                                ,viewport.width
                                                ,viewport.height
                                                ,viewport.minDepth
                                                ,viewport.maxDepth);
        }
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::setScissor(uint first,uint cnt,const RectU32* scissors)
    {
        (void)first;
        (void)cnt;
        (void)scissors;
        if (!_native || !_renderPassEncoder)
            return FEResult::ER_FAILED;
        
        wgpuRenderPassEncoderSetScissorRect(_renderPassEncoder
                                            ,scissors->left()
                                            ,scissors->top()
                                            ,scissors->width()
                                            ,scissors->height());
        return FEResult::ER_SUCCESS;

    }
    FEResult    WGCmdBuffer::setLineWidth(float )
    {
        return FEResult::ER_FAILED;
    }

    FEResult    WGCmdBuffer::setCullMode(CullMode mode) 
    {
        UNUSED(mode);
        switch (mode)
        {
        case CullMode::CM_NULL:
            break;
        case CullMode::CM_FRONT:
            break;
        case CullMode::CM_BACK:
            break;
        case CullMode::CM_FRONT_AND_BACK:
            break;
        default:
            break;
        }
        return FEResult::ER_FAILED;
    }

    FEResult    WGCmdBuffer::setDepthTest(bool )
    {
        return FEResult::ER_FAILED;
    }

    FEResult    WGCmdBuffer::beginRender(const RenderInfo& rs)
    {
        if (!_native)
            return FEResult::ER_FAILED;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain      =   nullptr;

        WGPUTextureView attachments[1]  =   {};
        WGPUTextureView depthAttachment =   rs._depth ? (WGPUTextureView)rs._depth->native() : nullptr;
        if (!rs._colors.empty())
        {
            attachments[0]  =   (WGPUTextureView)rs._colors.front()->native();
        }

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view                =   attachments[0];
        colorAttachment.resolveTarget       =   nullptr;
        colorAttachment.clearValue          =   { rs._clearColor.x,rs._clearColor.y,rs._clearColor.z,rs._clearColor.w };
        colorAttachment.loadOp              =   WGPULoadOp_Clear;
        colorAttachment.storeOp             =   WGPUStoreOp_Store;
        /// depthSlice 必须显式设为 WGPU_DEPTH_SLICE_UNDEFINED,否则默认 0 会被当作
        /// "使用第 0 层切片",而 swapchain 颜色附件是 2D 纹理视图,触发验证错误:
        /// "Depth slice was provided but the color attachment's view is not 3D"。
        colorAttachment.depthSlice          =   WGPU_DEPTH_SLICE_UNDEFINED;

        renderPassDesc.colorAttachmentCount =   1;
        renderPassDesc.colorAttachments     =   &colorAttachment;

        WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
        depthStencilAttachment.view             =   depthAttachment;
        depthStencilAttachment.depthClearValue  =   rs._clearDepth;
        depthStencilAttachment.depthLoadOp      =   WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp     =   WGPUStoreOp_Store;
        depthStencilAttachment.stencilClearValue=   rs._clearStencil;
        depthStencilAttachment.stencilLoadOp    =   WGPULoadOp_Clear;
        depthStencilAttachment.stencilStoreOp   =   WGPUStoreOp_Store;

        renderPassDesc.depthStencilAttachment =   &depthStencilAttachment;

        _renderPassEncoder =   wgpuCommandEncoderBeginRenderPass(_native,   &renderPassDesc);
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
        if (!pl || !_native)
            return FEResult::ER_FAILED;

        /// compute 管线:开始 compute pass 并绑定 WGPUComputePipeline
        if (pl->type() == PL_COMPUTE)
        {
            if (!_computePassEncoder)
            {
                WGPUComputePassDescriptor computePassDesc    =   {};
                computePassDesc.nextInChain                  =   nullptr;
                computePassDesc.label                        =   { nullptr,0 };
                computePassDesc.timestampWrites              =   nullptr;
                _computePassEncoder  =   wgpuCommandEncoderBeginComputePass(_native,&computePassDesc);
                if (!_computePassEncoder)
                {
                    LOG_ERR("WGCmdBuffer.bindPipeline: wgpuCommandEncoderBeginComputePass failed");
                    return  FEResult::ER_FAILED;
                }
            }
            auto*   wgCPipeline =   const_cast<WGCPipeline*>(static_cast<const WGCPipeline*>(pl.get()));
            if (wgCPipeline)
            {
                _currentComputePipeline    =   (WGPUComputePipeline)wgCPipeline->native();
                wgpuComputePassEncoderSetPipeline(_computePassEncoder,_currentComputePipeline);
            }
            return  FEResult::ER_SUCCESS;
        }

        /// 图形管线
        if (!_renderPassEncoder)
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
        for (size_t i = 0;i < dss.dSets.size();i++)
        {
            if (dss.dSets[i])
            {
                auto* wgSet = const_cast<WGDSet*>(static_cast<const WGDSet*>(dss.dSets[i].get()));
                if (wgSet)
                {
                    WGPUBindGroup bindGroup =   (WGPUBindGroup)wgSet->native();
                    /// bind group 可能因对象未全部关联而推迟创建(_native 为 null),
                    /// 此时跳过绑定以避免向 WebGPU 提交无效 handle。
                    if (bindGroup == nullptr)
                    {
                        LOG_ERR("WGCmdBuffer.bindDescriptors: bind group at set[%zu] is null (deferred), skip",
                                i);
                        continue;
                    }
                    /// 根据 plBindPoint 选择 pass encoder,
                    /// PL_COMPUTE 路由到 _computePassEncoder,PL_GRAPIC 路由到 _renderPassEncoder
                    if (dss.plBindPoint == PL_COMPUTE && _computePassEncoder)
                    {
                        wgpuComputePassEncoderSetBindGroup(_computePassEncoder,dss.firstSet + (uint32_t)i,bindGroup,0,nullptr);
                    }
                    else if (_renderPassEncoder)
                    {
                        wgpuRenderPassEncoderSetBindGroup(_renderPassEncoder,dss.firstSet + (uint32_t)i,bindGroup,0,nullptr);
                    }
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
                    if (bufferOffset >= bufferSize)
                        continue;
                    bufferSize -= bufferOffset;
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
        /// WebGPU Immediates(var<immediate>)是 push_constant 的原生等价物。
        /// 数据通过 wgpuRenderPassEncoderSetImmediates 下发到当前绑定的 render pipeline,
        /// 其 layout 的 immediateSize 必须 >= offset + size。
        /// shaderBits 在 WebGPU 中无意义(immediate 对 layout 内所有 stage 可见),忽略。
        (void)pl;
        (void)shaderBits;
        if (!_renderPassEncoder || size == 0 || data == nullptr)
            return FEResult::ER_SUCCESS;
        wgpuRenderPassEncoderSetImmediates(_renderPassEncoder,offset,data,(size_t)size);
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff)
    {
        if (!_native || !srcBuf || !dstBuf)
            return FEResult::ER_FAILED;

        auto* srcWg = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(srcBuf.get()));
        auto* dstWg = const_cast<WGGPUBuffer*>(static_cast<const WGGPUBuffer*>(dstBuf.get()));
        if (srcWg && dstWg)
        {
            wgpuCommandEncoderCopyBufferToBuffer(_native,
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

    FEResult    WGCmdBuffer::dispatch(uint x,uint y,uint z)
    {
        if (!_computePassEncoder)
        {
            LOG_ERR("WGCmdBuffer.dispatch: no active compute pass encoder");
            return  FEResult::ER_FAILED;
        }
        wgpuComputePassEncoderDispatchWorkgroups(_computePassEncoder,x,y,z);
        return FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::end()
    {
        if (!_native)
            return FEResult::ER_FAILED;
        /// 若仍有未结束的 compute pass,在此结束
        if (_computePassEncoder)
        {
            wgpuComputePassEncoderEnd(_computePassEncoder);
            wgpuComputePassEncoderRelease(_computePassEncoder);
            _computePassEncoder    =   nullptr;
        }
        return  FEResult::ER_SUCCESS;
    }

    FEResult WGCmdBuffer::submit(Queue queue)
    {
        if (!_native || !queue)
            return FEResult::ER_FAILED;
        /// wgpuCommandEncoderFinish 会消费 encoder(内部引用计数 -1),
        /// 调用后 _native 不再属于本对象,置空以避免析构时二次释放。
        auto    cmdBuf  =   wgpuCommandEncoderFinish(_native,nullptr);
        _native         =   nullptr;
        wgpuQueueSubmit((WGPUQueue)queue->native(), 1, &cmdBuf);
        wgpuCommandBufferRelease(cmdBuf);
        return  FEResult::ER_SUCCESS;
    }
}

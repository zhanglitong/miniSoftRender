

#include    "VKCmdBuffer.h"
#include    "VKDevice.h"
#include    "VKCmdPool.h"
#include    "VKGPUBuffer.h"
#include    "VKQueue.h"

#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace   FE
{
    VKCmdBuffer::~VKCmdBuffer()
    {
        if (_native != nullptr)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkFreeCommandBuffers(device, (VkCommandPool)_cInfo._pool->native(), 1, &_native);
        }
    }
    FEResult    VKCmdBuffer::reset() 
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        else
            vkResetCommandBuffer(_native, 0);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::begin(bool oneTimeSubmit) 
    {
        if (!isValid())
            return  FEResult::ER_FAILED;

        VkCommandBufferBeginInfo cmdBufInfo =   {};
        cmdBufInfo.sType    =   VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.flags    =   oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;

        auto    result  =   vkBeginCommandBuffer(_native, &cmdBufInfo);
        return  (result == VK_SUCCESS) ? FEResult::ER_SUCCESS : FEResult::ER_FAILED;
    }

    FEResult    VKCmdBuffer::setViewport(uint first,uint cnt,const Viewport* vps) 
    {
        if (!isValid() || cnt == 0 || vps == nullptr || cnt > 8)
            return  FEResult::ER_FAILED;

        VkViewport  viewports[8]    =   {};
        VkViewport viewport{};
        constexpr bool invertY  =   true;
        if (true)
        {
            for (uint i = first; i < first + cnt; ++i)
            {
                viewports[i].x          =   vps[i].x;
                viewports[i].y          =   vps[i].height - vps[i].y;
                viewports[i].width      =   vps[i].width;
                viewports[i].height     =   -vps[i].height;
                viewports[i].minDepth   =   vps[i].minDepth;
                viewports[i].maxDepth   =   vps[i].maxDepth;
            }
        }
        else
        {
            for (uint i = first; i < first + cnt; ++i)
            {
                viewports[i].x          =   vps[i].x;
                viewports[i].y          =   vps[i].y;
                viewports[i].width      =   vps[i].width;
                viewports[i].height     =   vps[i].height;
                viewports[i].minDepth   =   vps[i].minDepth;
                viewports[i].maxDepth   =   vps[i].maxDepth;
            }
        }
        
        vkCmdSetViewport(_native, first, cnt, viewports);

        return  FEResult::ER_SUCCESS;

    }
    FEResult    VKCmdBuffer::setScissor(uint first,uint cnt,const RectU32* rects) 
    {
        if (!isValid() || cnt == 0 || rects == nullptr || cnt > 8)
            return  FEResult::ER_FAILED;

        VkRect2D  scissor[8]    =   {};
        for (uint i = first; i < first + cnt; ++i)
        {
            scissor[i].offset.x          =   rects[i].left();
            scissor[i].offset.y          =   rects[i].top();
            scissor[i].extent.width      =   rects[i].width();
            scissor[i].extent.height     =   rects[i].height();
        }
        vkCmdSetScissor(_native, first, cnt, scissor);

        return  FEResult::ER_SUCCESS;
    }
    FEResult    VKCmdBuffer::beginRenderPass(BeginInfo& info)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;

        VkClearValue    clearValues[2]  =   {};
      

        clearValues[0].color        = { info._clearColor[0],info._clearColor[1],info._clearColor[2],info._clearColor[3]};
        clearValues[1].depthStencil = { info._clearDepth, info._clearStencil };

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType                       =   VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext                       =   nullptr;
        renderPassBeginInfo.renderPass                  =   (info._renderPass != nullptr) ? (VkRenderPass)(info._renderPass->native()) : nullptr;
        renderPassBeginInfo.renderArea.offset.x         =   info._rect.left();
        renderPassBeginInfo.renderArea.offset.y         =   info._rect.top();
        renderPassBeginInfo.renderArea.extent.width     =   info._rect.width();
        renderPassBeginInfo.renderArea.extent.height    =   info._rect.height();
        renderPassBeginInfo.clearValueCount             =   2;
        renderPassBeginInfo.pClearValues                =   clearValues;
        renderPassBeginInfo.framebuffer                 =   (info._frameBuffer != nullptr) ? (VkFramebuffer)(info._frameBuffer->native()) : nullptr;;

        /// vkCmdBeginRenderPass(_native, &renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

        return  FEResult::ER_SUCCESS;

    }
    
    FEResult    VKCmdBuffer::beginRender(const RenderInfo& rs)
    {
        constexpr   size_t  nMax    =   32;
        RectU32     rect            =   rs._rect;
        VkImageView cViews[nMax]    =   {};
        size_t      nCnt            =   (std::min<size_t>)(nMax,rs._colors.size());

        for (size_t i = 0; i < nCnt; i++)
        {
            cViews[i]   =   (VkImageView)rs._colors[i]->native();
        }
        VkImageView dView       =   rs._depth == nullptr ? nullptr: (VkImageView)rs._depth->native();
        auto        depthImage  =   rs._depth->cInfo()._image;

        /// With dynamic rendering there are no subpass dependencies, so we need to take care of proper layout transitions by using barriers
        /// This set of barriers prepares the color and depth images for output
        for (size_t i = 0; i < nCnt; i++)
        {
            auto    colorImage =   rs._colors[i]->cInfo()._image;
            if (colorImage && colorImage->cInfo()._layout != IL_COLOR_ATTACHMENT_OPTIMAL)
            {
                auto    cImage  =   (VkImage)(colorImage->native());
                insertImageMemoryBarrier(
                    _native,
                    cImage,
                    0,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    system2Native(colorImage->cInfo()._layout),
                    system2Native(IL_COLOR_ATTACHMENT_OPTIMAL),
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
                colorImage->setLayout(IL_COLOR_ATTACHMENT_OPTIMAL);
            }
        }
        
        if (depthImage &&  depthImage->cInfo()._layout != IL_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            auto    dImage  =   (VkImage)(depthImage->native());
            insertImageMemoryBarrier(
                _native,
                dImage,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                system2Native(depthImage->cInfo()._layout),
                system2Native(IL_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });
            depthImage->setLayout(IL_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        VkRenderingAttachmentInfoKHR cAttachments[nMax] =   {};
        for (size_t i = 0; i < nCnt; i++)
        {
            cAttachments[i].sType               =   VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            cAttachments[i].imageView           =   cViews[i];
            cAttachments[i].imageLayout         =   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAttachments[i].loadOp              =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            cAttachments[i].storeOp             =   VK_ATTACHMENT_STORE_OP_STORE;
            cAttachments[i].clearValue.color    =   
            {
                rs._clearColor.r,
                rs._clearColor.g,
                rs._clearColor.b,
                rs._clearColor.a
            };
        }
			
        VkRenderingAttachmentInfoKHR dsAttachment   =   {};
        {
            dsAttachment.sType                      =   VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            dsAttachment.imageView                  =   dView;
            dsAttachment.imageLayout                =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            dsAttachment.loadOp                     =   VK_ATTACHMENT_LOAD_OP_CLEAR;
            dsAttachment.storeOp                    =   VK_ATTACHMENT_STORE_OP_STORE;
            dsAttachment.clearValue.depthStencil    =   {1.0f,  0};
        }

        VkRenderingInfoKHR  rInfo   =   {};
        {
            rInfo.sType                 =   VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            rInfo.renderArea            =   {(int)rect.left(), (int)rect.top(), rect.width(), rect.height() };
            rInfo.layerCount            =   1;
            rInfo.colorAttachmentCount  =   (uint32_t)nCnt;
            rInfo.pColorAttachments     =   cAttachments;
            rInfo.pDepthAttachment      =   &dsAttachment;
            rInfo.pStencilAttachment    =   &dsAttachment;
        };

        // Begin dynamic rendering
        vkCmdBeginRendering(_native, &rInfo);

        return  FEResult::ER_SUCCESS;   
    }

    FEResult    VKCmdBuffer::endRender(const RenderInfo& rs)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        else
            vkCmdEndRendering(_native);

        /// 布局转换
        constexpr   size_t  nMax    =   32;
        size_t      nCnt            =   (std::min<size_t>)(nMax,rs._colors.size());

        for (size_t i = 0; i < nCnt; i++)
        {
            auto    colorImage =   rs._colors[i]->cInfo()._image;
            if (colorImage && colorImage->cInfo()._layout != IL_PRESENT_SRC)
            {
                auto    cImage  =   (VkImage)(colorImage->native());
                insertImageMemoryBarrier(
                    _native,
                    cImage,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_ACCESS_MEMORY_READ_BIT,
                    system2Native(colorImage->cInfo()._layout),
                    system2Native(IL_PRESENT_SRC),
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
                colorImage->setLayout(IL_PRESENT_SRC);
            }
        }
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::endRenderPass()
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        else
            vkCmdEndRenderPass(_native);
        return  FEResult::ER_SUCCESS;
    }
    FEResult    VKCmdBuffer::bindPipeline(Pipeline pl) 
    {
        if (!isValid() || pl == nullptr || pl->native() == nullptr)
            return  FEResult::ER_FAILED;
        switch(pl->type())
        {
        case PL_GRAPIC:
            vkCmdBindPipeline(_native, VK_PIPELINE_BIND_POINT_GRAPHICS,         (VkPipeline)pl->native());
            break;
        case PL_COMPUTE:
            vkCmdBindPipeline(_native, VK_PIPELINE_BIND_POINT_COMPUTE,          (VkPipeline)pl->native());
            break;
        case PL_RAY_TRACING:
            vkCmdBindPipeline(_native, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,  (VkPipeline)pl->native());
            break;
        default:
            return  FEResult::ER_FAILED;
        }

        return  FEResult::ER_SUCCESS;
    }
    FEResult    VKCmdBuffer::bindDescriptors(const DSetBind& info) 
    {
        using   VkDescriptorSets    =   std::vector<VkDescriptorSet>;
        if (!isValid() || info.dSets.empty())
            return  FEResult::ER_FAILED;

        VkDescriptorSets    dynamicDset;
        VkDescriptorSet     dSets[8]    =   {};
        VkDescriptorSet*    pDsets      =   dSets;
        uint                dSetCount   =   (uint)info.dSets.size();
        if (info.dSets.size() > 8)
        {
            dynamicDset.resize(info.dSets.size());
            for (uint i = info.firstSet; i < info.firstSet + dSetCount; ++i)
            {
                dynamicDset[i]  =   (VkDescriptorSet)info.dSets[i]->native();
            }
            pDsets  =   dynamicDset.data();
        }
        else
        {
            for (uint i = 0; i < dSetCount; ++i)
            {
                dSets[i]        =   (VkDescriptorSet)info.dSets[i]->native();
            }
        }
       
        vkCmdBindDescriptorSets(      _native
                                    , system2Native(info.plBindPoint)
                                    , (VkPipelineLayout)info.plLayout 
                                    , info.firstSet
                                    , dSetCount
                                    , pDsets
                                    , info.offsetCount
                                    , info.offsets);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::bindVBO(uint first,uint cnt,const VBOs& vbos,const uint64s& offsets)
    {
        using   VkBuffers   =   std::vector<VkBuffer>;

        if (!isValid() || cnt == 0 || vbos.empty())
            return  FEResult::ER_FAILED;
        VkBuffer        buffers[16] =   {};
        VkDeviceSize    offset[16]  =   {};

        for (uint i = 0; i < vbos.size(); ++i)
        {
            buffers[i]  =   (VkBuffer)vbos[i]->native();
            offset[i]   =   offsets.empty() ? 0: offsets[i];
        }
        vkCmdBindVertexBuffers(_native,first,cnt,buffers,offset);

        return  FEResult::ER_SUCCESS;  
    }

    FEResult    VKCmdBuffer::bindVBO(uint first,const VBO& vbo,uint64  offset)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        VkBuffer        buffers[1]  =   {(VkBuffer)vbo->native()};
        VkDeviceSize    offsets[1]  =   {offset};
        vkCmdBindVertexBuffers(_native,first,1,buffers,offsets);

        return  FEResult::ER_SUCCESS;  
    }

    FEResult    VKCmdBuffer::bindIBO(IBO ibo,uint64 offset,FEIndexType type)
    {
       
        if (!isValid() || ibo == nullptr && ibo->native() != nullptr)
            return  FEResult::ER_FAILED;
        VkBuffer        buffer      =   (VkBuffer)ibo->native();
        VkDeviceSize    vkOffset    =   offset;
        
        vkCmdBindIndexBuffer(_native,buffer,vkOffset,system2Native(type));

        return  FEResult::ER_SUCCESS;  
    }

    bool        VKCmdBuffer::create(const FECmdBuffer::CreateInfo& cInf) 
    {
        if (isValid())
            return    true;
        _cInfo              =   cInf;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        VkCommandBufferAllocateInfo cmdInfo {};
        cmdInfo.sType               =    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdInfo.commandPool         =    (VkCommandPool)cInf._pool->native();
        cmdInfo.level               =    VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdInfo.commandBufferCount  =    1;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdInfo, &_native));
        return    _native != nullptr;
    }
    FEResult    VKCmdBuffer::pushConstants   (FEPipeline* pl,uint32_t shaderBits,uint32_t offset,uint32_t size,const void* data)
    {
        vkCmdPushConstants(_native,(VkPipelineLayout)pl->nativeLayout(),shaderBits,offset,size,data);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,uint64 length,uint64 srcOff,uint64 dstOff) 
    {
        if (   !isValid() 
            || srcBuf  == nullptr 
            || dstBuf  == nullptr 
            || length  == 0)
            return  FEResult::ER_FAILED;

        VKGPUBuffer* pDst   =   dynamic_cast<VKGPUBuffer*>(dstBuf.get());
        if (pDst == nullptr)
            return  FEResult::ER_FAILED;
        
        VkBufferCopy    copy;
        copy.dstOffset  =   dstOff;
        copy.srcOffset  =   srcOff;
        copy.size       =   length;
        vkCmdCopyBuffer(_native,(VkBuffer)srcBuf->native(),(VkBuffer)dstBuf->native(),1,&copy);

        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::copyBuffer(GPUBuffer srcBuf,GPUBuffer dstBuf,const BufferCopys& ranges)
    {
        if (   !isValid() 
            || srcBuf  == nullptr 
            || dstBuf  == nullptr )
            return  FEResult::ER_FAILED;

        VkBufferCopy    copy;
        copy.dstOffset  =   0;
        copy.srcOffset  =   0;
        copy.size       =   (std::min)(srcBuf->cInfo()._length,dstBuf->cInfo()._length);

        if (ranges.empty())
            vkCmdCopyBuffer(_native,(VkBuffer)srcBuf->native(),(VkBuffer)dstBuf->native(),1,&copy);
        else
            vkCmdCopyBuffer(_native,(VkBuffer)srcBuf->native(),(VkBuffer)dstBuf->native(),(uint32)ranges.size(),(VkBufferCopy*)ranges.data());

        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::updateBuffer(GPUBuffer dstBuf,const void* data,uint64 length,uint64 dstOff)
    {
        if (   !isValid() 
            || dstBuf  == nullptr 
            || data    == nullptr 
            || length  == 0)
            return  FEResult::ER_FAILED;
        vkCmdUpdateBuffer(_native,(VkBuffer)dstBuf->native(),dstOff,length,data);
        return  FEResult::ER_SUCCESS;
    }


    FEResult    VKCmdBuffer::draw  (   uint32_t firstVertex  
                                    , uint32_t vertexCount
                                    , uint32_t firstInstance
                                    , uint32_t instanceCount)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdDraw(_native,vertexCount,instanceCount,firstVertex,firstInstance);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::drawIndex  ( uint32_t firstIndex
                                        , uint32_t indexCount
                                        , int32_t  vertexOffset
                                        , uint32_t firstInstance
                                        , uint32_t instanceCount)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdDrawIndexed(_native,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);

        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::drawArrayIndirect(      ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdDrawIndirect(_native,(VkBuffer)buffer->native(),offset,drawCount,stride);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::drawIndexedIndirect(   ITO         buffer,
                                                    uint64      offset,
                                                    uint32_t    drawCount,
                                                    uint32_t    stride)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdDrawIndexedIndirect(_native,(VkBuffer)buffer->native(),offset,drawCount,stride);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::setPrimitiveTopology(EPrimitive pri) 
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdSetPrimitiveTopology(_native,system2Native(pri));
        return  FEResult::ER_SUCCESS;
    }
    FEResult    VKCmdBuffer::dispatch(uint x,uint y,uint z) 
    {
        vkCmdDispatch(_native,x,y,z);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::end()
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        auto    result  =   vkEndCommandBuffer(_native);
        assert(result == VK_SUCCESS);
        return (result == VK_SUCCESS) ? FEResult::ER_SUCCESS : FEResult::ER_FAILED;
    }

    FEResult    VKCmdBuffer::submit(Queue queue)
    {
        auto&       vkDevice    =   (VKDevice&)_ctx.device();
        auto        device      =   vkDevice.logicalDevice();
        if (queue == nullptr)
            queue   =   vkDevice.queueGraphic();
        VkCommandBuffer cmdBufs[1]      =   {_native};
        VkSubmitInfo    submitInfo      =   {};
        submitInfo.sType                =   VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   =   1;
        submitInfo.pCommandBuffers      =   cmdBufs;

        VkFenceCreateInfo fenceCI       =   {};
        fenceCI.sType   =   VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags   =   0;
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

        VK_CHECK_RESULT(vkQueueSubmit((VkQueue)queue->native(), 1, &submitInfo, fence));
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(device, fence, nullptr);
        VK_CHECK_RESULT(vkDeviceWaitIdle(device));

        return  FEResult::ER_SUCCESS;
    }

}

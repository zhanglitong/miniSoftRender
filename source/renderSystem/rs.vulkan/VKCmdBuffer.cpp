

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
        VkViewport  viewport{};

        #ifdef  INVERT_VIEWPORT
            for (uint i = first; i < first + cnt; ++i)
            {
                viewports[i].x          =   vps[i].x;
                viewports[i].y          =   vps[i].height - vps[i].y;
                viewports[i].width      =   vps[i].width;
                viewports[i].height     =   -vps[i].height;
                viewports[i].minDepth   =   vps[i].minDepth;
                viewports[i].maxDepth   =   vps[i].maxDepth;
            }
        #else
            for (uint i = first; i < first + cnt; ++i)
            {
                viewports[i].x          =   vps[i].x;
                viewports[i].y          =   vps[i].y;
                viewports[i].width      =   vps[i].width;
                viewports[i].height     =   vps[i].height;
                viewports[i].minDepth   =   vps[i].minDepth;
                viewports[i].maxDepth   =   vps[i].maxDepth;
            }
        #endif
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
    FEResult    VKCmdBuffer::setLineWidth(float width) 
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdSetLineWidth(_native,width);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::setDepthTest(bool bEnabled)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        vkCmdSetDepthTestEnable(_native,bEnabled ? VK_TRUE : VK_FALSE);
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

    FEResult    VKCmdBuffer::setCullMode(CullMode mode)
    {
        switch (mode)
        {
        case CullMode::CM_NULL:
            vkCmdSetCullMode(_native,VK_CULL_MODE_NONE);
            break;
        case CullMode::CM_FRONT:
            vkCmdSetCullMode(_native,VK_CULL_MODE_FRONT_BIT);
            break;
        case CullMode::CM_BACK:
            vkCmdSetCullMode(_native,VK_CULL_MODE_BACK_BIT);
            break;
        case CullMode::CM_FRONT_AND_BACK:
            vkCmdSetCullMode(_native,VK_CULL_MODE_FRONT_AND_BACK);
            break;
        default:
            break;
        }
        return  FEResult::ER_SUCCESS;
    }
    
    FEResult    VKCmdBuffer::beginRender(const RenderInfo& rs)
    {
        constexpr   size_t  nMax    =   32;
        RectU32     rect            =   rs._rect;
        VkImageView cViews[nMax]    =   {};
        size_t      nCnt            =   (std::min<size_t>)(nMax,rs._colors.size());

        const bool  skipColor       =   rs._skipColor;
        const bool  skipDepth       =   rs._skipDepth || rs._depth == nullptr;

        for (size_t i = 0; i < nCnt; i++)
        {
            cViews[i]   =   (VkImageView)rs._colors[i]->native();
        }
        VkImageView dView       =   skipDepth ? nullptr : (VkImageView)rs._depth->native();

        /// 无 color attachment 时跳过 color 相关 barrier
        if (!skipColor)
        {
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
        }
        
        if (!skipDepth && rs._depth)
        {
            auto    depthImage  =   rs._depth->cInfo()._image;
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
        }

        VkRenderingAttachmentInfoKHR cAttachments[nMax] =   {};
        for (size_t i = 0; i < nCnt; i++)
        {
            cAttachments[i].sType               =   VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            cAttachments[i].imageView           =   cViews[i];
            cAttachments[i].imageLayout         =   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAttachments[i].loadOp              =   rs._loadColor ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                              : VK_ATTACHMENT_LOAD_OP_CLEAR;
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
        if (!skipDepth)
        {
            dsAttachment.sType                      =   VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            dsAttachment.imageView                  =   dView;
            dsAttachment.imageLayout                =   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            dsAttachment.loadOp                     =   rs._loadDepth ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                                      : VK_ATTACHMENT_LOAD_OP_CLEAR;
            dsAttachment.storeOp                    =   VK_ATTACHMENT_STORE_OP_STORE;
            dsAttachment.clearValue.depthStencil    =   {1.0f,  0};
        }

        VkRenderingInfoKHR  rInfo   =   {};
        {
            rInfo.sType                 =   VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            rInfo.renderArea            =   {(int)rect.left(), (int)rect.top(), rect.width(), rect.height() };
            rInfo.layerCount            =   1;
            rInfo.colorAttachmentCount  =   skipColor ? 0 : (uint32_t)nCnt;
            rInfo.pColorAttachments     =   skipColor ? nullptr : cAttachments;
            rInfo.pDepthAttachment      =   skipDepth ? nullptr : &dsAttachment;
            rInfo.pStencilAttachment    =   skipDepth ? nullptr : &dsAttachment;
        };

        vkCmdBeginRendering(_native, &rInfo);

        return  FEResult::ER_SUCCESS;   
    }

    FEResult    VKCmdBuffer::endRender(const RenderInfo& rs)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        else
            vkCmdEndRendering(_native);

        if (rs._skipColor)
            return  FEResult::ER_SUCCESS;

        /// 布局转换
        constexpr   size_t  nMax    =   32;
        size_t      nCnt            =   (std::min<size_t>)(nMax,rs._colors.size());

        for (size_t i = 0; i < nCnt; i++)
        {
            auto    colorImage =   rs._colors[i]->cInfo()._image;
            /// 离屏渲染目标(color attachment + sampled,如 transmission 场景纹理)
            /// 渲染结束后转入 SHADER_READ_ONLY 供后续趟采样;
            /// swapchain 图像(包装 cInfo 无 COLOR_ATTACHMENT 标志)转 PRESENT
            const bool  sampled =   colorImage
                                    && colorImage->cInfo()._usage.hasFlag(USAGE_SAMPLED)
                                    && colorImage->cInfo()._usage.hasFlag(USAGE_COLOR_ATTACHMENT);
            const auto  dstLayout   =   sampled ? IL_SHADER_READ_ONLY_OPTIMAL : IL_PRESENT_SRC;
            if (colorImage && colorImage->cInfo()._layout != dstLayout)
            {
                auto    cImage  =   (VkImage)(colorImage->native());
                insertImageMemoryBarrier(
                    _native,
                    cImage,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    sampled ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_MEMORY_READ_BIT,
                    system2Native(colorImage->cInfo()._layout),
                    system2Native(dstLayout),
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    sampled ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                            : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
                colorImage->setLayout(dstLayout);
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

        VkBufferMemoryBarrier barrier = {};
        barrier.sType           =   VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask   =   VK_ACCESS_TRANSFER_WRITE_BIT;   /// 等待写入完成
        barrier.dstAccessMask   =   VK_ACCESS_TRANSFER_READ_BIT;    /// 允许后续读取
        barrier.buffer          =   (VkBuffer)dstBuf->native();
        barrier.offset          =   dstOff;
        barrier.size            =   length;
        ///
        /// 注意：如果是在同一个Queue家族，src/dstQueueFamilyIndex 设为 VK_QUEUE_FAMILY_IGNORED
        /// 避免没有拷贝完，就被读取使用
        vkCmdPipelineBarrier(_native,
            VK_PIPELINE_STAGE_TRANSFER_BIT,   // 源阶段：传输写入
            VK_PIPELINE_STAGE_TRANSFER_BIT,   // 目标阶段：传输读取
            0,
            0, 
            nullptr,
            1, 
            &barrier,
            0, 
            nullptr);

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

    FEResult    VKCmdBuffer::fillBuffer(GPUBuffer buf, uint64 dstOff, uint64 size, uint32 value)
    {
        if (   !isValid()
            || buf     == nullptr
            || size    == 0)
            return  FEResult::ER_FAILED;
        // size 必须是 4 的倍数（vkCmdFillBuffer 要求）
        size = size & ~(uint64_t(3));
        vkCmdFillBuffer(_native, (VkBuffer)buf->native(), dstOff, size, value);
        return  FEResult::ER_SUCCESS;
    }

    FEResult    VKCmdBuffer::pipelineBarrier(uint32 srcStageMask, uint32 dstStageMask,
                                             uint32 srcAccessMask, uint32 dstAccessMask)
    {
        if (!isValid())
            return  FEResult::ER_FAILED;
        VkMemoryBarrier barrier = {};
        barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask   = (VkAccessFlags)srcAccessMask;
        barrier.dstAccessMask   = (VkAccessFlags)dstAccessMask;
        vkCmdPipelineBarrier(_native,
                             (VkPipelineStageFlags)srcStageMask,
                             (VkPipelineStageFlags)dstStageMask,
                             0,
                             1, &barrier,
                             0, nullptr,
                             0, nullptr);
        return  FEResult::ER_SUCCESS;
    }
    FEResult    VKCmdBuffer::generateMips(GImgView view)
    {
        if (!isValid() || view == nullptr)
            return  FEResult::ER_FAILED;
        auto    image   =   view->cInfo()._image;
        if (image == nullptr || image->native() == nullptr)
            return  FEResult::ER_FAILED;
        const auto& imgInfo =   image->cInfo();
        const uint  mipLevels   =   imgInfo._mips;
        if (mipLevels < 2)
            return  FEResult::ER_SUCCESS;

        auto    vkImage =   (VkImage)image->native();

        /// 进入时状态(pass1 endRender 之后):mip0 = SHADER_READ_ONLY,
        /// 其余 mip 尚未触碰过(UNDEFINED)。流程:
        ///   0. mip0 SHADER_READ_ONLY -> TRANSFER_SRC(为第 1 轮 blit 准备 src)
        ///   1. 对每级 mip i=1..N:
        ///      a) 若 i>1,上轮已把 mip i-1 恢复到 SHADER_READ,这里再切回 TRANSFER_SRC
        ///      b) mip i UNDEFINED -> TRANSFER_DST
        ///      c) blit(mip i-1 -> mip i,线性过滤)
        ///      d) mip i-1 TRANSFER_SRC -> SHADER_READ(src 用完立刻恢复)
        ///      e) mip i   TRANSFER_DST -> SHADER_READ
        ///   循环结束后 mip 0..N 全部 SHADER_READ_ONLY
        insertImageMemoryBarrier(
            _native, vkImage,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        uint    srcW    =   imgInfo._width;
        uint    srcH    =   imgInfo._height;
        for (uint mip = 1; mip < mipLevels; ++mip)
        {
            const uint  dstW    =   srcW > 1 ? srcW >> 1 : 1;
            const uint  dstH    =   srcH > 1 ? srcH >> 1 : 1;

            /// i>1 时上轮末尾已把 src(mip i-1) 恢复到 SHADER_READ,这里再切回 TRANSFER_SRC
            if (mip > 1)
            {
                insertImageMemoryBarrier(
                    _native, vkImage,
                    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, 1, 0, 1 });
            }

            /// dst(mip i) UNDEFINED -> TRANSFER_DST
            insertImageMemoryBarrier(
                _native, vkImage,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mip, 1, 0, 1 });

            VkImageBlit blit                =   {};
            blit.srcSubresource.aspectMask  =   VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel    =   mip - 1;
            blit.srcSubresource.layerCount  =   1;
            blit.srcOffsets[0]              =   { 0, 0, 0 };
            blit.srcOffsets[1]              =   { (int32_t)srcW, (int32_t)srcH, 1 };
            blit.dstSubresource.aspectMask  =   VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel    =   mip;
            blit.dstSubresource.layerCount  =   1;
            blit.dstOffsets[0]              =   { 0, 0, 0 };
            blit.dstOffsets[1]              =   { (int32_t)dstW, (int32_t)dstH, 1 };
            vkCmdBlitImage(_native,
                           vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit, VK_FILTER_LINEAR);

            /// blit 完成:src 和 dst 都用完了,各自恢复 SHADER_READ
            /// src(mip i-1):TRANSFER_SRC -> SHADER_READ
            insertImageMemoryBarrier(
                _native, vkImage,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, 1, 0, 1 });

            /// dst(mip i):TRANSFER_DST -> SHADER_READ
            insertImageMemoryBarrier(
                _native, vkImage,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mip, 1, 0, 1 });

            srcW    =   dstW;
            srcH    =   dstH;
        }

        /// 循环结束后 mip 0..N 全部 SHADER_READ_ONLY,同步引擎侧状态
        image->setLayout(IL_SHADER_READ_ONLY_OPTIMAL);
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

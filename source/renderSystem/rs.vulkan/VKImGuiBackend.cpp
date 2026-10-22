
#include    "VKImGuiBackend.h"
#include    "VKDevice.h"
#include    "VKImGuiSpv.h"
#include    "VKCmdPool.h"
#include    "VKQueue.h"

#include    <cstring>
#include    <algorithm>

namespace   FE
{
    /// ImGui 顶点:pos(2f) + uv(2f) + col(1u) = 20 字节
    static  constexpr   uint32  kVertexStride   =   20;

    VKImGuiBackend::VKImGuiBackend(FEContext& ctx,VKDevice& device)
        :_ctx(ctx)
        ,_device(device)
    {
    }

    VKImGuiBackend::~VKImGuiBackend()
    {
        destroy();
    }

    bool    VKImGuiBackend::createMappedBuffer(VkDeviceSize size,VkBufferUsageFlags usage,
                                               VkBuffer& buffer,VkDeviceMemory& memory,void*& mapped)
    {
        auto    device  =   _device.logicalDevice();

        VkBufferCreateInfo  bufInfo     =   {};
        bufInfo.sType                   =   VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size                    =   size;
        bufInfo.usage                   =   usage;
        bufInfo.sharingMode             =   VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device,&bufInfo,nullptr,&buffer) != VK_SUCCESS)
            return  false;

        VkMemoryRequirements    reqs    =   {};
        vkGetBufferMemoryRequirements(device,buffer,&reqs);

        VkMemoryAllocateInfo    alloc   =   {};
        alloc.sType                     =   VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize            =   reqs.size;
        alloc.memoryTypeIndex           =   _device.getMemoryTypeIndex(reqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device,&alloc,nullptr,&memory) != VK_SUCCESS)
            return  false;
        if (vkBindBufferMemory(device,buffer,memory,0) != VK_SUCCESS)
            return  false;
        if (vkMapMemory(device,memory,0,size,0,&mapped) != VK_SUCCESS)
            return  false;
        return  true;
    }

    bool    VKImGuiBackend::createDescriptorSet()
    {
        auto    device  =   _device.logicalDevice();

        /// set=0 binding=0 : 字体图集(combined image sampler),片元阶段采样
        VkDescriptorSetLayoutBinding  binding =   {};
        binding.binding                 =   0;
        binding.descriptorType          =   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount         =   1;
        binding.stageFlags               =   VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo  =   {};
        layoutInfo.sType                =   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount         =   1;
        layoutInfo.pBindings            =   &binding;
        if (vkCreateDescriptorSetLayout(device,&layoutInfo,nullptr,&_dsLayout) != VK_SUCCESS)
            return  false;

        VkDescriptorPoolSize    poolSize    =   {};
        poolSize.type                       =   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount             =   1;

        VkDescriptorPoolCreateInfo  poolInfo    =   {};
        poolInfo.sType                          =   VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets                        =   1;
        poolInfo.poolSizeCount                  =   1;
        poolInfo.pPoolSizes                     =   &poolSize;
        if (vkCreateDescriptorPool(device,&poolInfo,nullptr,&_dsPool) != VK_SUCCESS)
            return  false;

        VkDescriptorSetAllocateInfo allocInfo   =   {};
        allocInfo.sType                         =   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool                =   _dsPool;
        allocInfo.descriptorSetCount            =   1;
        allocInfo.pSetLayouts                   =   &_dsLayout;
        return  vkAllocateDescriptorSets(device,&allocInfo,&_dsSet) == VK_SUCCESS;
    }

    bool    VKImGuiBackend::createPipeline()
    {
        auto    device      =   _device.logicalDevice();

        VkShaderModuleCreateInfo    vertInfo    =   {};
        vertInfo.sType                          =   VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertInfo.codeSize                       =   sizeof(kImGuiVertSpv);
        vertInfo.pCode                          =   kImGuiVertSpv;
        if (vkCreateShaderModule(device,&vertInfo,nullptr,&_vertModule) != VK_SUCCESS)
            return  false;

        VkShaderModuleCreateInfo    fragInfo    =   {};
        fragInfo.sType                          =   VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragInfo.codeSize                       =   sizeof(kImGuiFragSpv);
        fragInfo.pCode                          =   kImGuiFragSpv;
        if (vkCreateShaderModule(device,&fragInfo,nullptr,&_fragModule) != VK_SUCCESS)
            return  false;

        VkPipelineShaderStageCreateInfo  stages[2]  =   {};
        stages[0].sType                 =   VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage                 =   VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module                =   _vertModule;
        stages[0].pName                 =   "main";
        stages[1].sType                 =   VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage                 =   VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module                =   _fragModule;
        stages[1].pName                 =   "main";

        /// 顶点输入:loc0 pos(2f),loc1 uv(2f),loc2 col(4*u8)
        VkVertexInputBindingDescription      vtxBinding  =   {};
        vtxBinding.binding              =   0;
        vtxBinding.stride               =   kVertexStride;
        vtxBinding.inputRate            =   VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription    attrs[3]    =   {};
        attrs[0].location   =   0;  attrs[0].binding = 0;  attrs[0].format = VK_FORMAT_R32G32_SFLOAT;        attrs[0].offset = 0;
        attrs[1].location   =   1;  attrs[1].binding = 0;  attrs[1].format = VK_FORMAT_R32G32_SFLOAT;        attrs[1].offset = 8;
        attrs[2].location   =   2;  attrs[2].binding = 0;  attrs[2].format = VK_FORMAT_R8G8B8A8_UNORM;       attrs[2].offset = 16;

        VkPipelineVertexInputStateCreateInfo vtxInfo   =   {};
        vtxInfo.sType                   =   VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vtxInfo.vertexBindingDescriptionCount        =   1;
        vtxInfo.pVertexBindingDescriptions            =   &vtxBinding;
        vtxInfo.vertexAttributeDescriptionCount       =   3;
        vtxInfo.pVertexAttributeDescriptions          =   attrs;

        VkPipelineInputAssemblyStateCreateInfo  iaInfo =   {};
        iaInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        iaInfo.topology                 =   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo   vpInfo  =   {};
        vpInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vpInfo.viewportCount            =   1;
        vpInfo.scissorCount             =   1;

        VkPipelineRasterizationStateCreateInfo  rsInfo  =   {};
        rsInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rsInfo.polygonMode              =   VK_POLYGON_MODE_FILL;
        rsInfo.cullMode                 =   VK_CULL_MODE_NONE;
        rsInfo.frontFace                =   VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rsInfo.lineWidth                =   1.0f;

        VkPipelineMultisampleStateCreateInfo    msInfo  =   {};
        msInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msInfo.rasterizationSamples     =   VK_SAMPLE_COUNT_1_BIT;

        /// 关闭深度/模板(GUI 不参与深度测试)
        VkPipelineDepthStencilStateCreateInfo    dsInfo  =   {};
        dsInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsInfo.depthTestEnable          =   VK_FALSE;
        dsInfo.depthWriteEnable         =   VK_FALSE;
        dsInfo.depthCompareOp           =   VK_COMPARE_OP_ALWAYS;
        dsInfo.stencilTestEnable        =   VK_FALSE;

        /// alpha 混合(ImGui 纹理带 alpha)
        VkPipelineColorBlendAttachmentState  blend =   {};
        blend.blendEnable               =   VK_TRUE;
        blend.srcColorBlendFactor       =   VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor       =   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp              =   VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor       =   VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor       =   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alphaBlendOp              =   VK_BLEND_OP_ADD;
        blend.colorWriteMask            =   VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cbInfo   =   {};
        cbInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cbInfo.attachmentCount           =   1;
        cbInfo.pAttachments              =   &blend;

        const   VkDynamicState  dynamicStates[] =   { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo    dynInfo =   {};
        dynInfo.sType                   =   VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynInfo.dynamicStateCount       =   2;
        dynInfo.pDynamicStates          =   dynamicStates;

        VkPushConstantRange pcRange     =   {};
        pcRange.stageFlags              =   VK_SHADER_STAGE_VERTEX_BIT;
        pcRange.offset                  =   0;
        pcRange.size                    =   16;   ///< vec2 scale + vec2 translate

        VkPipelineLayoutCreateInfo  plInfo  =   {};
        plInfo.sType                    =   VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plInfo.setLayoutCount            =   1;
        plInfo.pSetLayouts              =   &_dsLayout;
        plInfo.pushConstantRangeCount   =   1;
        plInfo.pPushConstantRanges      =   &pcRange;
        if (vkCreatePipelineLayout(device,&plInfo,nullptr,&_plLayout) != VK_SUCCESS)
            return  false;

        /// 动态渲染:颜色附件格式必须与渲染目标一致;
        /// 深度/模板附件格式必须与当前渲染作用域实际提供的附件一致(引擎为 D32_SFLOAT_S8_UINT,
        /// 见 VKGPipeline.cpp),否则校验层报 dynamicRenderingUnusedAttachments 错误。
        VkPipelineRenderingCreateInfo    rendering   =   {};
        rendering.sType                     =   VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering.colorAttachmentCount       =   1;
        rendering.pColorAttachmentFormats   =   &_colorFormat;
        rendering.depthAttachmentFormat     =   VK_FORMAT_D32_SFLOAT_S8_UINT;
        rendering.stencilAttachmentFormat   =   VK_FORMAT_D32_SFLOAT_S8_UINT;

        VkGraphicsPipelineCreateInfo    plCI    =   {};
        plCI.sType                  =   VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        plCI.pNext                  =   &rendering;
        plCI.stageCount             =   2;
        plCI.pStages                =   stages;
        plCI.pVertexInputState      =   &vtxInfo;
        plCI.pInputAssemblyState    =   &iaInfo;
        plCI.pViewportState         =   &vpInfo;
        plCI.pRasterizationState    =   &rsInfo;
        plCI.pMultisampleState      =   &msInfo;
        plCI.pDepthStencilState     =   &dsInfo;
        plCI.pColorBlendState       =   &cbInfo;
        plCI.pDynamicState          =   &dynInfo;
        plCI.layout                 =   _plLayout;
        plCI.renderPass             =   VK_NULL_HANDLE;
        return  vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&plCI,nullptr,&_pipeline) == VK_SUCCESS;
    }

    bool    VKImGuiBackend::createFontImage(uint32 width,uint32 height,const void* pixels)
    {
        auto    device  =   _device.logicalDevice();

        VkImageCreateInfo   imgInfo =   {};
        imgInfo.sType                   =   VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType               =   VK_IMAGE_TYPE_2D;
        imgInfo.format                  =   VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.extent                  =   { width,height,1 };
        imgInfo.mipLevels               =   1;
        imgInfo.arrayLayers             =   1;
        imgInfo.samples                 =   VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling                  =   VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage                   =   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imgInfo.sharingMode             =   VK_SHARING_MODE_EXCLUSIVE;
        imgInfo.initialLayout           =   VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(device,&imgInfo,nullptr,&_fontImage) != VK_SUCCESS)
            return  false;

        VkMemoryRequirements    reqs    =   {};
        vkGetImageMemoryRequirements(device,_fontImage,&reqs);
        VkMemoryAllocateInfo    alloc   =   {};
        alloc.sType                     =   VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize            =   reqs.size;
        alloc.memoryTypeIndex           =   _device.getMemoryTypeIndex(reqs.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device,&alloc,nullptr,&_fontMemory) != VK_SUCCESS)
            return  false;
        if (vkBindImageMemory(device,_fontImage,_fontMemory,0) != VK_SUCCESS)
            return  false;

        VkImageViewCreateInfo   viewInfo    =   {};
        viewInfo.sType                      =   VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                      =   _fontImage;
        viewInfo.viewType                   =   VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                     =   VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask     =   VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount     =   1;
        viewInfo.subresourceRange.layerCount     =   1;
        if (vkCreateImageView(device,&viewInfo,nullptr,&_fontView) != VK_SUCCESS)
            return  false;

        uploadImage(pixels,VkDeviceSize(width) * height * 4);

        /// 写描述符
        VkDescriptorImageInfo   di  =   {};
        di.sampler          =   _sampler;
        di.imageView        =   _fontView;
        di.imageLayout      =   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet    write    =   {};
        write.sType             =   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet            =   _dsSet;
        write.dstBinding        =   0;
        write.descriptorCount   =   1;
        write.descriptorType    =   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo        =   &di;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        return  true;
    }

    void    VKImGuiBackend::uploadImage(const void* pixels,VkDeviceSize size)
    {
        auto    device  =   _device.logicalDevice();

        /// 临时 staging 缓冲(宿主可见)
        VkBuffer        stageBuf    =   VK_NULL_HANDLE;
        VkDeviceMemory  stageMem    =   VK_NULL_HANDLE;
        void*           stagePtr    =   nullptr;
        if (!createMappedBuffer(size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,stageBuf,stageMem,stagePtr))
            return;
        memcpy(stagePtr,pixels,size_t(size));

        /// 一次性命令缓冲(用设备图形池)
        auto*   vkPool  =   dynamic_cast<VKCmdPool*>(_device.graphicCmdPool().get());
        if (vkPool == nullptr)
            return;
        VkCommandPool   pool    =   (VkCommandPool)vkPool->native();

        VkCommandBufferAllocateInfo allocInfo   =   {};
        allocInfo.sType             =   VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool       =   pool;
        allocInfo.level             =   VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount=   1;
        VkCommandBuffer cmd         =   VK_NULL_HANDLE;
        vkAllocateCommandBuffers(device,&allocInfo,&cmd);

        VkCommandBufferBeginInfo    begin   =   {};
        begin.sType                 =   VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags                 =   VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd,&begin);

        auto    barrier =   [](VkCommandBuffer c,VkImage img,VkImageLayout oldL,VkImageLayout newL,
                               VkAccessFlags srcA,VkAccessFlags dstA,VkPipelineStageFlags srcS,VkPipelineStageFlags dstS)
        {
            VkImageMemoryBarrier b  =   {};
            b.sType                 =   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.oldLayout             =   oldL;
            b.newLayout             =   newL;
            b.srcQueueFamilyIndex   =   VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex   =   VK_QUEUE_FAMILY_IGNORED;
            b.image                 =   img;
            b.subresourceRange.aspectMask    =   VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.levelCount    =   1;
            b.subresourceRange.layerCount    =   1;
            b.srcAccessMask         =   srcA;
            b.dstAccessMask         =   dstA;
            vkCmdPipelineBarrier(c,srcS,dstS,0,0,nullptr,0,nullptr,1,&b);
        };

        barrier(cmd,_fontImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0,VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy   region  =   {};
        region.imageSubresource.aspectMask   =   VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount   =   1;
        region.imageExtent                   =   { uint32(_fontExtent.width), uint32(_fontExtent.height), 1 };
        vkCmdCopyBufferToImage(cmd,stageBuf,_fontImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

        barrier(cmd,_fontImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo    submit  =   {};
        submit.sType                =   VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount   =   1;
        submit.pCommandBuffers      =   &cmd;
        vkQueueSubmit((VkQueue)_device.queueGraphic()->native(),1,&submit,VK_NULL_HANDLE);
        vkQueueWaitIdle((VkQueue)_device.queueGraphic()->native());

        vkFreeCommandBuffers(device,pool,1,&cmd);
        vkDestroyBuffer(device,stageBuf,nullptr);
        vkFreeMemory(device,stageMem,nullptr);
    }

    bool    VKImGuiBackend::init(FEFormat colorFormat)
    {
        _colorFormat    =   system2Native(colorFormat);

        /// 采样器:线性过滤 + clamp
        VkSamplerCreateInfo samplerInfo =   {};
        samplerInfo.sType                   =   VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               =   VK_FILTER_LINEAR;
        samplerInfo.minFilter               =   VK_FILTER_LINEAR;
        samplerInfo.mipmapMode              =   VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU            =   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV            =   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW            =   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (vkCreateSampler(_device.logicalDevice(),&samplerInfo,nullptr,&_sampler) != VK_SUCCESS)
            return  false;

        if (!createDescriptorSet())
            return  false;
        if (!createPipeline())
            return  false;

        /// 常驻映射的顶点/索引缓冲
        if (!createMappedBuffer(VkDeviceSize(kMaxVertices) * kVertexStride,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                _vtxBuffer,_vtxMemory,_vtxMapped))
            return  false;
        if (!createMappedBuffer(VkDeviceSize(kMaxIndices) * sizeof(uint16),VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                _idxBuffer,_idxMemory,_idxMapped))
            return  false;
        return  true;
    }

    bool    VKImGuiBackend::uploadFont(const FEImGuiFontData& font)
    {
        if (font._pixels == nullptr || font._width == 0 || font._height == 0)
            return  false;
        _fontExtent =   { font._width,font._height,1 };
        return  createFontImage(font._width,font._height,font._pixels);
    }

    void    VKImGuiBackend::render(const FEImGuiDrawData& drawData,FECmdBuffer* cmd)
    {
        if (_pipeline == nullptr || cmd == nullptr || _vtxMapped == nullptr)
            return;
        if (drawData._vertexCount > kMaxVertices || drawData._indexCount > kMaxIndices)
        {
            LOG_ERR("VKImGuiBackend::render vertex/index overflow (%u/%u).",drawData._vertexCount,drawData._indexCount);
            return;
        }
        if (drawData._displaySizeX <= 0.0f || drawData._displaySizeY <= 0.0f)
            return;

        if (drawData._vertexCount > 0)
            memcpy(_vtxMapped,drawData._vertexData,size_t(drawData._vertexCount) * kVertexStride);
        if (drawData._indexCount > 0)
            memcpy(_idxMapped,drawData._indexData,size_t(drawData._indexCount) * sizeof(uint16));

        auto    vkCmd   =   (VkCommandBuffer)cmd->native();
        if (vkCmd == VK_NULL_HANDLE)
            return;

        vkCmdBindPipeline(vkCmd,VK_PIPELINE_BIND_POINT_GRAPHICS,_pipeline);
        vkCmdBindDescriptorSets(vkCmd,VK_PIPELINE_BIND_POINT_GRAPHICS,_plLayout,0,1,&_dsSet,0,nullptr);

        VkDeviceSize    offset  =   0;
        vkCmdBindVertexBuffers(vkCmd,0,1,&_vtxBuffer,&offset);
        vkCmdBindIndexBuffer(vkCmd,_idxBuffer,0,VK_INDEX_TYPE_UINT16);

        float   fbW =   drawData._displaySizeX * drawData._scaleX;
        float   fbH =   drawData._displaySizeY * drawData._scaleY;
        VkViewport  vp  =   { 0.0f,0.0f,fbW,fbH,0.0f,1.0f };
        vkCmdSetViewport(vkCmd,0,1,&vp);

        /// 正交投影:逻辑像素 -> NDC。
        /// Vulkan 帧缓冲原点在左上,viewport 正高度时 NDC y=-1 对应屏幕顶部,
        /// 与 ImGui(y=0 在顶部)一致,故 scale.y 取正、translate.y = -1。
        float   scale[2]     =   { 2.0f / drawData._displaySizeX, 2.0f / drawData._displaySizeY };
        float   translate[2] =   { -1.0f - drawData._displayPosX * scale[0],
                                   -1.0f - drawData._displayPosY * scale[1] };
        float   pushConst[4] =   { scale[0],scale[1],translate[0],translate[1] };
        vkCmdPushConstants(vkCmd,_plLayout,VK_SHADER_STAGE_VERTEX_BIT,0,16,pushConst);

        for (uint32 i = 0; i < drawData._cmdCount; ++i)
        {
            const FEImGuiDrawCmd&   dc  =   drawData._cmds[i];
            /// 裁剪矩形(逻辑像素)-> 帧缓冲像素
            float   cx  =   (dc._clipX - drawData._displayPosX) * drawData._scaleX;
            float   cy  =   (dc._clipY - drawData._displayPosY) * drawData._scaleY;
            float   cw  =   (dc._clipW - drawData._displayPosX) * drawData._scaleX - cx;
            float   ch  =   (dc._clipH - drawData._displayPosY) * drawData._scaleY - cy;
            if (cx < 0) { cw += cx; cx = 0; }
            if (cy < 0) { ch += cy; cy = 0; }
            if (cw < 0 || ch < 0)
                continue;
            cw  =   (std::min)(cw,fbW - cx);
            ch  =   (std::min)(ch,fbH - cy);
            if (cw <= 0 || ch <= 0)
                continue;

            VkRect2D    scissor =   { { int32(cx),int32(cy) },{ uint32(cw),uint32(ch) } };
            vkCmdSetScissor(vkCmd,0,1,&scissor);
            vkCmdDrawIndexed(vkCmd,dc._indexCount,1,dc._indexOffset,dc._vertexOffset,0);
        }

        /// scissor 是动态状态:ImGui 逐命令设置裁剪矩形,绘制完必须复位。
        /// 否则同一帧内其后绘制的工厂会继承 ImGui 最后一个命令的裁剪矩形而被裁掉
        /// (例如天空工厂的 USAGE_Classic 含 GUI/Overlay,会在 ImGui 之后再画一遍)。
        VkRect2D    fullScissor =   { { 0,0 },{ uint32(fbW),uint32(fbH) } };
        vkCmdSetScissor(vkCmd,0,1,&fullScissor);
    }

    void    VKImGuiBackend::destroy()
    {
        auto    device      =   _device.logicalDevice();
        if (device == VK_NULL_HANDLE)
            return;
        if (_vtxBuffer)     vkDestroyBuffer(device,_vtxBuffer,nullptr);
        if (_vtxMemory)     vkFreeMemory(device,_vtxMemory,nullptr);
        if (_idxBuffer)     vkDestroyBuffer(device,_idxBuffer,nullptr);
        if (_idxMemory)     vkFreeMemory(device,_idxMemory,nullptr);
        _vtxBuffer  =   VK_NULL_HANDLE;  _vtxMemory  =   VK_NULL_HANDLE;  _vtxMapped  =   nullptr;
        _idxBuffer  =   VK_NULL_HANDLE;  _idxMemory  =   VK_NULL_HANDLE;  _idxMapped  =   nullptr;

        if (_fontView)      vkDestroyImageView(device,_fontView,nullptr);
        if (_fontImage)     vkDestroyImage(device,_fontImage,nullptr);
        if (_fontMemory)    vkFreeMemory(device,_fontMemory,nullptr);
        _fontView   =   VK_NULL_HANDLE;  _fontImage  =   VK_NULL_HANDLE;  _fontMemory =   VK_NULL_HANDLE;

        if (_pipeline)      vkDestroyPipeline(device,_pipeline,nullptr);            _pipeline   =   VK_NULL_HANDLE;
        if (_plLayout)      vkDestroyPipelineLayout(device,_plLayout,nullptr);      _plLayout   =   VK_NULL_HANDLE;
        if (_dsPool)        vkDestroyDescriptorPool(device,_dsPool,nullptr);        _dsPool     =   VK_NULL_HANDLE;
        if (_dsLayout)      vkDestroyDescriptorSetLayout(device,_dsLayout,nullptr); _dsLayout   =   VK_NULL_HANDLE;
        if (_sampler)       vkDestroySampler(device,_sampler,nullptr);              _sampler    =   VK_NULL_HANDLE;
        if (_vertModule)    vkDestroyShaderModule(device,_vertModule,nullptr);      _vertModule =   VK_NULL_HANDLE;
        if (_fragModule)    vkDestroyShaderModule(device,_fragModule,nullptr);      _fragModule =   VK_NULL_HANDLE;
    }
}

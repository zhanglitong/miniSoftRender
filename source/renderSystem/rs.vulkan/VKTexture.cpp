
#include    "VKTexture.h"
#include    "VKDevice.h"
#include    "VKGPUBuffer.h"
#include    "VKQueue.h"
#include    "FEFormatHelper.hpp"

namespace   FE
{
    VKTexture::~VKTexture()
    {
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        if (_sampler)
        {
            vkDestroySampler(device,_sampler,nullptr);
            _sampler    =   nullptr;
        }
        _view       =   nullptr;
        _image      =   nullptr;
        _native     =   nullptr;
    }

    bool    VKTexture::create(const FETexture::CreateInfo& info)
    {
        _cInfo  =   info;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        /// 1. 创建图像(设备本地内存,用于传输目标 + 采样)
        _image  =   new VKGImage(_ctx);
        FEGImage::CreateInfo    imgInfo     =   {};
        imgInfo._format                     =   info._format;
        imgInfo._type                       =   IT_2D;
        imgInfo._layout                     =   IL_UNDEFINED;
        /// 渲染目标纹理(transmission 场景折射):color attachment + sampled 双用途
        imgInfo._usage                      =   info._renderTarget
                                                ? (USAGE_COLOR_ATTACHMENT | USAGE_SAMPLED)
                                                : (USAGE_TRANSFER_DST | USAGE_SAMPLED);
        /// 多 mip 渲染目标:pass1 渲 mip0 后逐级 blit downsample 生成 mip 链
        /// (VKCmdBuffer::generateMips),需传输源/目标用法
        if (info._renderTarget && info._mips > 1)
            imgInfo._usage                  |=  (USAGE_TRANSFER_SRC | USAGE_TRANSFER_DST);
        imgInfo._width                      =   info._width;
        imgInfo._height                     =   info._height;
        imgInfo._depth                      =   1;
        imgInfo._mips                       =   info._mips > 0 ? info._mips : 1;
        imgInfo._layer                      =   info._cube ? 6 : 1;
        if (!_image->create(imgInfo))
        {
            LOG_ERR("VKTexture.create: image create failed");
            return  false;
        }

        /// 2. 创建图像视图
        _view   =   _image->createView();
        if (_view == nullptr)
        {
            LOG_ERR("VKTexture.create: view create failed");
            return  false;
        }
        _native =   (VkImageView)_view->native();

        /// 3. 创建采样器
        const float maxLod  =   float(imgInfo._mips > 1 ? imgInfo._mips - 1 : 1);
        VkSamplerCreateInfo samplerCI   =   {};
        samplerCI.sType                 =   VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter             =   info._magFilter == FILTER_NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        samplerCI.minFilter             =   info._minFilter == FILTER_NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        samplerCI.mipmapMode            =   imgInfo._mips > 1 ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCI.addressModeU          =   system2Native(info._addressMode);
        samplerCI.addressModeV          =   system2Native(info._addressMode);
        samplerCI.addressModeW          =   system2Native(info._addressMode);
        samplerCI.mipLodBias            =   0.0f;
        samplerCI.maxAnisotropy         =   1.0f;
        samplerCI.minLod                =   0.0f;
        samplerCI.maxLod                =   maxLod;
        samplerCI.borderColor           =   VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        VK_CHECK_RESULT(vkCreateSampler(device,&samplerCI,nullptr,&_sampler));

        LOG_DBG("VKTexture.create %ux%u ok",info._width,info._height);
        return  true;
    }

    bool    VKTexture::update(const void* data,uint64 size)
    {
        if (_image == nullptr || _native == nullptr || data == nullptr || size == 0)
            return  false;

        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        /// 1. 主机可见 staging buffer,作为拷贝源
        GPUBuffer   staging =   vkDevice.createFromBuffer(data,size);
        if (staging == nullptr)
            return  false;

        /// 2. 从传输命令池分配一次性命令缓冲,执行布局切换 + 拷贝
        VkCommandPool       cmdPool =   (VkCommandPool)vkDevice.transferCmdPool()->native();
        VkCommandBufferAllocateInfo cmdAI   =   {};
        cmdAI.sType                 =   VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAI.commandPool           =   cmdPool;
        cmdAI.level                 =   VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAI.commandBufferCount    =   1;
        VkCommandBuffer cmd =   nullptr;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device,&cmdAI,&cmd));

        VkCommandBufferBeginInfo beginInfo  =   {};
        beginInfo.sType                     =   VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags                     =   VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd,&beginInfo));

        /// 布局切换范围(覆盖全部 mip/面)
        const uint  mipLevels   =   _image->cInfo()._mips;
        const uint  layerCount  =   _image->cInfo()._layer;

        /// UNDEFINED -> TRANSFER_DST_OPTIMAL
        VkImageSubresourceRange range   =   {};
        range.aspectMask                =   VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel              =   0;
        range.levelCount                =   mipLevels;
        range.baseArrayLayer            =   0;
        range.layerCount                =   layerCount;

        VkImageMemoryBarrier barrier            =   {};
        barrier.sType                           =   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       =   VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       =   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex             =   VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             =   VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           =   image();
        barrier.subresourceRange                =   range;
        barrier.srcAccessMask                   =   0;
        barrier.dstAccessMask                   =   VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd
                            ,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                            ,VK_PIPELINE_STAGE_TRANSFER_BIT
                            ,0,0,nullptr,0,nullptr,1,&barrier);

        /// staging buffer -> image
        /// 数据排布:面主序、面内 mip 顺序(与 FETexture::CreateInfo 注释一致);
        /// 每像素字节数由格式决定(RGBA8=4,RGBA32F=16 等)。
        std::vector<VkBufferImageCopy>  regions;
        if (layerCount > 1 || mipLevels > 1)
        {
            const uint  bpp     =   FEFormatHelper::sizeOf(_cInfo._format);
            uint64  offset  =   0;
            for (uint layer = 0; layer < layerCount; ++layer)
            {
                uint    mipW    =   _cInfo._width;
                uint    mipH    =   _cInfo._height;
                for (uint mip = 0; mip < mipLevels; ++mip)
                {
                    VkBufferImageCopy   region      =   {};
                    region.bufferOffset                 =   offset;
                    region.imageSubresource.aspectMask  =   VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel    =   mip;
                    region.imageSubresource.baseArrayLayer  =   layer;
                    region.imageSubresource.layerCount  =   1;
                    region.imageExtent                  =   { mipW,mipH,1 };
                    regions.push_back(region);
                    offset  +=   uint64(mipW) * mipH * bpp;
                    mipW    =   mipW > 1 ? mipW >> 1 : 1;
                    mipH    =   mipH > 1 ? mipH >> 1 : 1;
                }
            }
            if (offset != size)
            {
                LOG_ERR("VKTexture.update: size mismatch (%llu != %llu)",(uint64)offset,(uint64)size);
                vkFreeCommandBuffers(device,cmdPool,1,&cmd);
                return  false;
            }
        }
        else
        {
            VkBufferImageCopy   region      =   {};
            region.imageSubresource.aspectMask  =   VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel    =   0;
            region.imageSubresource.baseArrayLayer  =   0;
            region.imageSubresource.layerCount  =   1;
            region.imageExtent                  =   { _cInfo._width,_cInfo._height,1 };
            regions.push_back(region);
        }
        vkCmdCopyBufferToImage(cmd
                            ,(VkBuffer)staging->native()
                            ,image()
                            ,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                            ,(uint32_t)regions.size(),regions.data());

        /// TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout       =   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout       =   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask   =   VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask   =   VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd
                            ,VK_PIPELINE_STAGE_TRANSFER_BIT
                            ,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                            ,0,0,nullptr,0,nullptr,1,&barrier);

        VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

        /// 3. 提交并等待完成(staging buffer 随后即可释放)
        VkQueue queue   =   (VkQueue)vkDevice.queueTransfer()->native();
        VkSubmitInfo    submitInfo  =   {};
        submitInfo.sType                =   VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   =   1;
        submitInfo.pCommandBuffers      =   &cmd;
        VK_CHECK_RESULT(vkQueueSubmit(queue,1,&submitInfo,nullptr));
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device,cmdPool,1,&cmd);
        staging =   nullptr;

        /// 记录当前布局
        _image->setLayout(IL_SHADER_READ_ONLY_OPTIMAL);

        LOG_DBG("VKTexture.update %llu bytes ok",(uint64)size);
        return  true;
    }
}

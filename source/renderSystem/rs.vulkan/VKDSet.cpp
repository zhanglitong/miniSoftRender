

#include    "VKDSet.h"
#include    "VKDevice.h"
#include    "VKDSetLayout.h"
#include    "VKDSetPool.h"
#include    "VKGPUBuffer.h"
#include    "VKTexture.h"

namespace   FE
{

    VKDSet::~VKDSet()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            auto    pool        =   _cInfo._pool->as<VKDSetPool>();
            vkFreeDescriptorSets(device,(VkDescriptorPool)_cInfo._pool->native(),1,&_native);
            pool->free();
        }
    }

    bool    VKDSet::create(const FEDSet::CreateInfo& cInf) 
    {
        _cInfo  =   cInf;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
   
        VkDescriptorSetLayout   layout      =   (VkDescriptorSetLayout)cInf._layout->native();
        const FEDSetBindings&   layoutBinds =   cInf._layout->cInfo()._bindings;
        _cInfo._binds.resize(layoutBinds.size());
        for (size_t i = 0; i < layoutBinds.size(); i++)
        {
            _cInfo._binds[i]._binding   =   layoutBinds[i]._binding;
            _cInfo._binds[i]._type      =   layoutBinds[i]._descriptorType;
            _cInfo._binds[i]._name      =   layoutBinds[i]._name;
            _cInfo._binds[i]._typeName  =   layoutBinds[i]._typeName;
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType                 =   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool        =   (VkDescriptorPool)cInf._pool->native();
        allocInfo.descriptorSetCount    =   1;
        allocInfo.pSetLayouts           =   &layout;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &_native));

        auto    pool        =   _cInfo._pool->as<VKDSetPool>();
        pool->alloc();

        return  _native != nullptr;
    }

    bool    VKDSet::update()
    {
        if (!_dirty)
            return  false;
        else
            _dirty  =   false;
        
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        assert(_cInfo._binds.size() <= 16);
        
        size_t  ncnt    =   0;
        for (size_t i = 0; i < _cInfo._binds.size(); i++)
        {
            ncnt    +=  _cInfo._binds[i]._objects.size();
        }
        /// _buffers 与 _images 是按描述符类型分开的两个数组，各自 push_back 独立增长，
        /// 必须用各自的局部下标定位元素；不能共用一个全局计数器，
        /// 否则后续再次进入同一类型分支时会越界写（Debug 触发 vector 下标断言）
        size_t  nBufIndex   =   0;
        size_t  nImgIndex   =   0;
        _buffers.clear();
        _buffers.reserve(ncnt);
        _images.clear();
        _images.reserve(ncnt);
        _wdss.clear();
        _wdss.reserve(_cInfo._binds.size());

        for (size_t c = 0; c < _cInfo._binds.size(); c++)
        {
            auto&           binding     =   _cInfo._binds[c];
            /// 稀疏绑定：允许 layout 中存在尚未绑定的 descriptor（如 OIT 几何趟 DSets）
            if (binding._objects.empty())
                continue;
            FEGPUBuffer*    pBuffer     =   nullptr;
            switch(binding._type)
            {
            case DT_UNIFORM_BUFFER:
                {
                    size_t  nStart  =   nBufIndex;
                    for (size_t i = 0; i < binding._objects.size(); ++i,++nBufIndex)
                    {
                        _buffers.push_back({});
                        pBuffer                     =   dynamic_cast<FEGPUBuffer*>(binding._objects[i].get());
                        _buffers[nBufIndex].buffer  =   (VkBuffer)pBuffer->native();
                        _buffers[nBufIndex].offset  =   binding._offsets.empty() ? 0    : binding._offsets[i];
                        _buffers[nBufIndex].range   =   binding._ranges.empty() ? ~0ULL : binding._ranges[i];
                    }

                    VkWriteDescriptorSet    wds  =   {};
                    wds.sType            =   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wds.dstSet           =   _native;
                    wds.descriptorCount  =   uint(nBufIndex - nStart);
                    wds.descriptorType   =   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    wds.pBufferInfo      =   _buffers.data() + nStart;
                    wds.dstBinding       =   binding._binding;
                    _wdss.emplace_back(wds);
                }
                break;
            case DT_STORAGE_BUFFER:
            case DT_STORAGE_BUFFER_READ:
                {
                    size_t  nStart  =   nBufIndex;
                    for (size_t i = 0; i < binding._objects.size(); ++i,++nBufIndex)
                    {
                        _buffers.push_back({});
                        pBuffer                     =   dynamic_cast<FEGPUBuffer*>(binding._objects[i].get());
                        _buffers[nBufIndex].buffer  =   (VkBuffer)pBuffer->native();
                        _buffers[nBufIndex].offset  =   binding._offsets.empty() ? 0    : binding._offsets[i];
                        _buffers[nBufIndex].range   =   binding._ranges.empty() ? ~0ULL : binding._ranges[i];
                    }
                    VkWriteDescriptorSet    wds  =   {};
                    wds.sType           =  VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wds.dstSet          =  _native;
                    wds.descriptorCount =   uint(nBufIndex - nStart);
                    wds.descriptorType  =  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    wds.pBufferInfo     =  _buffers.data() + nStart;
                    wds.dstBinding      =  binding._binding;
                    _wdss.emplace_back(wds);
                }
                break;
            case DT_COMBINED_IMAGE_SAMPLER:
            case DT_SAMPLED_IMAGE:
            case DT_SAMPLER:
                {
                    size_t  nStart  =   nImgIndex;
                    for (size_t i = 0; i < binding._objects.size(); ++i,++nImgIndex)
                    {
                        VKTexture*  pTexture    =   dynamic_cast<VKTexture*>(binding._objects[i].get());
                        /// 注意:Release 下 assert 不生效,必须用运行时判空,
                        /// 否则 dynamic_cast 失败会直接空指针解引用
                        if (pTexture == nullptr || pTexture->sampler() == nullptr)
                        {
                            LOG_ERR("VKDSet.update: binding %u bind invalid texture object",binding._binding);
                            return  false;
                        }
                        _images.push_back({});
                        /// combined sampler 需要 sampler + view;
                        /// 纯 DT_SAMPLED_IMAGE 时 sampler 置空,纯 DT_SAMPLER 时 imageView 置空
                        _images[nImgIndex].sampler =   binding._type == DT_SAMPLED_IMAGE ? nullptr : pTexture->sampler();
                        _images[nImgIndex].imageView   =   binding._type == DT_SAMPLER ? nullptr : (VkImageView)pTexture->native();
                        _images[nImgIndex].imageLayout =   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    VkWriteDescriptorSet    wds     =   {};
                    wds.sType               =   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wds.dstSet              =   _native;
                    wds.descriptorCount     =   uint(nImgIndex - nStart);
                    wds.descriptorType      =   binding._type == DT_COMBINED_IMAGE_SAMPLER ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                                : (binding._type == DT_SAMPLED_IMAGE ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLER);
                    wds.pImageInfo          =   _images.data() + nStart;
                    wds.dstBinding          =   binding._binding;
                    _wdss.emplace_back(wds);
                }
                break;
            }
        }
        if (_wdss.empty())
           return   false;
        else
            vkUpdateDescriptorSets(device, uint(_wdss.size()), _wdss.data(), 0, nullptr);

        return  true;
    }
}

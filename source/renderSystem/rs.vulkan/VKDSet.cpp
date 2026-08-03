

#include    "VKDSet.h"
#include    "VKDevice.h"
#include    "VKDSetLayout.h"
#include    "VKDSetPool.h"
#include    "VKGPUBuffer.h"

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
        assert(_cInfo._binds.size() <= 8);
        
        size_t  ncnt    =   0;
        size_t  nIndex  =   0;
        for (size_t i = 0; i < _cInfo._binds.size(); i++)
        {
            ncnt    +=  _cInfo._binds[i]._objects.size();
        }
        _buffers.clear();
        _buffers.reserve(ncnt);
        _wdss.clear();
        _wdss.reserve(_cInfo._binds.size());

        for (size_t c = 0; c < _cInfo._binds.size(); c++)
        {
            auto&           binding     =   _cInfo._binds[c];
            FEGPUBuffer*    pBuffer     =   dynamic_cast<FEGPUBuffer*>(binding._objects.front().get());
            switch(binding._type)
            {
            case DT_UNIFORM_BUFFER:
                {
                    size_t  nStart  =   nIndex;
                    for (size_t i = 0; i < binding._objects.size(); ++i,++nIndex)
                    {
                        _buffers.push_back({});
                        pBuffer                     =   dynamic_cast<FEGPUBuffer*>(binding._objects[i].get());
                        _buffers[nIndex].buffer        =   (VkBuffer)pBuffer->native();
                        _buffers[nIndex].offset        =   binding._offsets.empty() ? 0    : binding._offsets[i];
                        _buffers[nIndex].range         =   binding._ranges.empty() ? ~0ULL : binding._ranges[i];
                    }

                    VkWriteDescriptorSet    wds  =   {};
                    wds.sType            =   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wds.dstSet           =   _native;
                    wds.descriptorCount  =   uint(nIndex - nStart);
                    wds.descriptorType   =   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    wds.pBufferInfo      =   _buffers.data() + nStart;
                    wds.dstBinding       =   binding._binding;
                    _wdss.emplace_back(wds);
                }
                break;
            case DT_STORAGE_BUFFER:
                {
                    size_t  nStart  =   nIndex;
                    for (size_t i = 0; i < binding._objects.size(); ++i,++nIndex)
                    {
                        _buffers.push_back({});
                        pBuffer                     =   dynamic_cast<FEGPUBuffer*>(binding._objects[i].get());
                        _buffers[nIndex].buffer     =   (VkBuffer)pBuffer->native();
                        _buffers[nIndex].offset     =   binding._offsets.empty() ? 0    : binding._offsets[i];
                        _buffers[nIndex].range      =   binding._ranges.empty() ? ~0ULL : binding._ranges[i];
                    }
                    VkWriteDescriptorSet    wds  =   {};
                    wds.sType           =  VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wds.dstSet          =  _native;
                    wds.descriptorCount =   uint(nIndex - nStart);
                    wds.descriptorType  =   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    wds.pBufferInfo     =   _buffers.data() + nStart;
                    wds.dstBinding      =   binding._binding;
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


#include    "VKDSetPool.h"
#include    "VKDevice.h"
#include    "VKDSetLayout.h"
#include    "VKDSet.h"

namespace   FE
{
    VKDSetPool::~VKDSetPool()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyDescriptorPool(device,_native,nullptr);
        }
    }

    bool    VKDSetPool::create(const FEDSetPool::CreateInfo& cInf) 
    {
        _cInfo = cInf;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        std::map<VkDescriptorType, uint32_t> typeCounts;

        for (auto& layout : cInf._layouts)
        {
            auto&   cInfo   =   layout->cInfo();

            for (const auto& binding : cInfo._bindings)
            {
                VkDescriptorType type   =   (VkDescriptorType)binding._descriptorType;
                typeCounts[type]        +=  binding._descriptorCount;
            }
        }
        
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto& pair : typeCounts)
        {
            VkDescriptorPoolSize size{};
            size.type               =   pair.first;
            size.descriptorCount    =   pair.second * cInf._maxSets;
            poolSizes.push_back(size);
        }

        if (poolSizes.empty())
        {
            VkDescriptorPoolSize defaultSize{};
            defaultSize.type                =   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            defaultSize.descriptorCount     =   2;
            poolSizes.push_back(defaultSize);
        }

        VkDescriptorPoolCreateInfo descriptorPoolCI{};
        descriptorPoolCI.sType          =   VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCI.pNext          =   nullptr;
        descriptorPoolCI.poolSizeCount  =   (uint32_t)poolSizes.size();
        descriptorPoolCI.pPoolSizes     =   poolSizes.data();
        descriptorPoolCI.maxSets        =   cInf._maxSets;
        descriptorPoolCI.flags          =   VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCI.flags          |=  VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &_native));
        return  _native != nullptr;
    }
}

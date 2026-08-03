

#include    "VKDSetLayout.h"
#include    "VKDevice.h"
#include    "VKDSet.h"

namespace   FE
{

    VKDSetLayout::~VKDSetLayout()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyDescriptorSetLayout(device,_native,nullptr);
        }
    }
    bool    VKDSetLayout::create(const FEDSetLayout::CreateInfo& cInf) 
    {
        _cInfo = cInf;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const auto& bind : _cInfo._bindings)
        {
            VkDescriptorSetLayoutBinding vkBind{};
            vkBind.binding              =   bind._binding;
            vkBind.descriptorType       =   (VkDescriptorType)bind._descriptorType;
            vkBind.descriptorCount      =   bind._descriptorCount;
            vkBind.stageFlags           =   (VkShaderStageFlags)bind._stageFlags.data();
            vkBind.pImmutableSamplers   =   nullptr;
            vkBindings.push_back(vkBind);
        }
   
        VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
        descriptorLayoutCI.sType        =   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutCI.pNext        =   nullptr;
        descriptorLayoutCI.bindingCount =   (uint32_t)vkBindings.size();
        descriptorLayoutCI.pBindings    =   vkBindings.data();
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &_native));
        return  _native != nullptr;
    }
    DSet    VKDSetLayout::createDSet()
    {
        return  {};
    }
}
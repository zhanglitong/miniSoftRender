

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
            /// DT_STORAGE_BUFFER_READ 与 DT_STORAGE_BUFFER 在 Vulkan 中都映射为
            /// VK_DESCRIPTOR_TYPE_STORAGE_BUFFER(Vulkan 不区分 read/read_write)
            if (bind._descriptorType == DT_STORAGE_BUFFER_READ)
                vkBind.descriptorType   =   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            else
                vkBind.descriptorType   =   (VkDescriptorType)bind._descriptorType;
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

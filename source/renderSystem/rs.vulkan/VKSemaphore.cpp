
#include    "VKSemaphore.h"
#include    "VKDevice.h"
namespace   FE
{

    VKSemaphore::~VKSemaphore()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroySemaphore(device,_native,nullptr);
        }
    }
    bool    VKSemaphore::create(const CreateInfo& cInfo)
    {
        if (isValid())
            return	true;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &_native));
        assert(_native != nullptr);
        return  _native != nullptr;
    }
}
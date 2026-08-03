
#include    "VKFence.h"
#include    "VKDevice.h"
namespace   FE
{

    VKFence::~VKFence()
    {
        if (_native)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyFence(device,_native,nullptr);
        }
    }
    void    VKFence::reset() 
    {
        if (!isValid())
            return;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        VK_CHECK_RESULT(vkResetFences(device, 1, &_native));
    }
    void    VKFence::wait(uint64 tm)
    {
        if (!isValid())
            return;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &_native,VK_TRUE,tm));
    }


    bool    VKFence::create(const CreateInfo& cInfo)
    {
        (void)cInfo;
        if (isValid())
            return	true;
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();

        VkFenceCreateInfo   fenceCI {};
        fenceCI.sType   =   VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags   =   VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &_native));
        assert(_native != nullptr);
        return  _native != nullptr;
    }
}


#include    "VKCmdPool.h"
#include    "VKDevice.h"
#include    "VKCmdBuffer.h"

namespace   FE
{
    VKCmdPool::~VKCmdPool()
    {
        if (_native)
        {   
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyCommandPool(device,_native,nullptr);
        }
    }

    bool    VKCmdPool::create(const FECmdPool::CreateInfo& cInf) 
    {
        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
   
        VkCommandPoolCreateInfo cmdInfo{};
        cmdInfo.sType               =    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdInfo.queueFamilyIndex    =    cInf._queueFamilyIndex;
        cmdInfo.flags               =    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(device, &cmdInfo, nullptr, &_native);
        assert(_native != nullptr);
        return    _native != nullptr;
    }

    CMDPtr      VKCmdPool::createCmd() 
    {
        if (!isValid())
            return    nullptr;
        CMDPtr                  cmd     =    new VKCmdBuffer(_ctx);
        FECmdBuffer::CreateInfo cInfo   =    {this};
        if (cmd->create(cInfo))
            return  cmd;
        else
            return  nullptr;
    }
}
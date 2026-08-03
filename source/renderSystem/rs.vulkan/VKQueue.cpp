
#include    "VKQueue.h"
#include    "VKDevice.h"

namespace   FE
{
    VKQueue::~VKQueue()
    {
    }
    bool    VKQueue::submit(uint cnt,const FEQueue::SubmitInfo* pInfo, Fence fence)
    {
        if (!isValid() || cnt == 0 || pInfo == nullptr)
            return  false;
       
        VkCommandBuffer vkCmds[8]   =    {};
        uint            cmdCnt      =    (std::min<uint>)(8,    (uint)pInfo->_cmds.size());
        for (size_t i = 0; i < cmdCnt; i++)
        {
            vkCmds[i]    =    (VkCommandBuffer)pInfo->_cmds[i]->native();
        }
        VkFence         nativeFence =   fence ? (VkFence)fence->native() : nullptr;
        VkSemaphore     waitSem[8]  =   {};
        VkSemaphore     signSem[8]  =   {};
        uint            waitCnt     =   (std::min<uint>)(8, (uint)pInfo->_presentCompleteSems.size());
        uint            signCnt     =   (std::min<uint>)(8, (uint)pInfo->_renderCompleteSems.size());

        for (size_t i = 0; i < waitCnt; i++)
        {
            waitSem[i]    =    (VkSemaphore)pInfo->_presentCompleteSems[i]->native();
        }
        for (size_t i = 0; i < signCnt; i++)
        {
            signSem[i]    =    (VkSemaphore)pInfo->_renderCompleteSems[i]->native();
        }

        VkPipelineStageFlags    waitStageMask   =    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo            submitInfo      =    {};
        submitInfo.sType                =    VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask    =    &waitStageMask;
        submitInfo.pCommandBuffers      =    vkCmds;
        submitInfo.commandBufferCount   =    cmdCnt;

        submitInfo.pWaitSemaphores      =    waitSem;
        submitInfo.waitSemaphoreCount   =    waitCnt;
        submitInfo.pSignalSemaphores    =    signSem;
        submitInfo.signalSemaphoreCount =    signCnt;

        auto&           vkDevice    =   (VKDevice&)_ctx.device();
        auto            device      =   vkDevice.logicalDevice();
        auto    result  =   vkQueueSubmit(_native, 1, &submitInfo, nativeFence);
        if (result == VK_SUCCESS && nativeFence != nullptr)
        {
            vkWaitForFences(device, 1, &nativeFence, VK_TRUE, UINT64_MAX);
        }

        return  result == VK_SUCCESS;
    }
}
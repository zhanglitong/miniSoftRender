
#include    "VKQueue.h"
#include    "VKDevice.h"

namespace   FE
{
    VKQueue::~VKQueue()
    {
    }
    bool    VKQueue::submit(uint cnt,const FEQueue::SubmitInfo* pInfo)
    {
        if (!isValid() || cnt == 0 || pInfo == nullptr)
            return  false;
       
        VkCommandBuffer vkCmds[4]   =    {};
        uint            cmdCnt      =    pInfo ? 1:0;
        if (pInfo && pInfo->_frame->_cmd)
        {
            vkCmds[0]    =    (VkCommandBuffer)pInfo->_frame->_cmd->native();
        }
        VkFence         nativeFence =   (pInfo && pInfo->_frame && pInfo->_frame->_fenceWait) ? (VkFence)pInfo->_frame->_fenceWait->native() : nullptr;
        VkSemaphore     waitSem[4]  =   {};
        VkSemaphore     signSem[4]  =   {};
        uint            waitCnt     =  (pInfo && pInfo->_frame && pInfo->_frame->_semPresentComplete) ? 1:0;
        uint            signCnt     =  (pInfo && pInfo->_frame && pInfo->_frame->_semRenderComplete)  ? 1:0;

        if(waitCnt != 0)
        {
            waitSem[0]    =    (VkSemaphore)pInfo->_frame->_semPresentComplete->native();
        }
        if(signCnt)
        {
            signSem[0]    =    (VkSemaphore)pInfo->_frame->_semRenderComplete->native();
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

        auto&   vkDevice    =   (VKDevice&)_ctx.device();
        auto    device      =   vkDevice.logicalDevice();
        auto    result      =   vkQueueSubmit(_native, 1, &submitInfo, nativeFence);
        if (result == VK_SUCCESS && nativeFence != nullptr)
        {
            vkWaitForFences(device, 1, &nativeFence, VK_TRUE, UINT64_MAX);
        }

        return  result == VK_SUCCESS;
    }
}
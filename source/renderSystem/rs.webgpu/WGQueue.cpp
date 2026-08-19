#include    "WGQueue.h"
#include    "WGDevice.h"
#include    "WGCmdBuffer.h"
#include    "WGFence.h"

namespace   FE
{
    WGQueue::~WGQueue()
    {
        if (_native)
        {
            wgpuQueueRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGQueue::submit(uint cnt,const SubmitInfo* pInfo)
    {
        (void)cnt;
        if (!_native || !pInfo)
            return false;

        std::vector<WGPUCommandBuffer> commandBuffers;
        
        if (pInfo && pInfo->_frame->_cmd)
        {
            auto    wgCmd =   (WGCmdBuffer*)(pInfo->_frame->_cmd.get());
            if (wgCmd)
            {
                auto    cmdBuf  =   wgpuCommandEncoderFinish((WGPUCommandEncoder)wgCmd->native(),nullptr);
                commandBuffers.push_back(cmdBuf);
            }
        }
        WGPUCommandBuffer*  pBuffer =   commandBuffers.empty() ? nullptr : commandBuffers.data();
        uint32_t            nCount  =   (uint32_t)commandBuffers.size();

        wgpuQueueSubmit(_native,nCount,pBuffer);

        if (pInfo && pInfo->_frame->_fenceWait)
        {
            auto* wgFence = const_cast<WGFence*>(static_cast<const WGFence*>(pInfo->_frame->_fenceWait.get()));
            if (wgFence)
            {
                wgFence->signal();
            }
        }

        return true;
    }
}
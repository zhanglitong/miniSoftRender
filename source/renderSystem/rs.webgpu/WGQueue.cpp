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

    bool WGQueue::submit(uint cnt,const SubmitInfo* pInfo,Fence fence)
    {
        (void)cnt;
        if (!_native || !pInfo)
            return false;

        std::vector<WGPUCommandBuffer> commandBuffers;
        for (const auto& cmd : pInfo->_cmds)
        {
            if (cmd)
            {
                auto* wgCmd = const_cast<WGCmdBuffer*>(static_cast<const WGCmdBuffer*>(cmd.get()));
                if (wgCmd)
                {
                    commandBuffers.push_back((WGPUCommandBuffer)wgCmd->native());
                }
            }
        }

        WGPUCommandBuffer* cBuffers =   commandBuffers.empty() ? nullptr : commandBuffers.data();
        uint32_t cBufferCount =   (uint32_t)commandBuffers.size();

        wgpuQueueSubmit(_native,cBufferCount,cBuffers);

        if (fence)
        {
            auto* wgFence = const_cast<WGFence*>(static_cast<const WGFence*>(fence.get()));
            if (wgFence)
            {
                wgFence->signal();
            }
        }

        return true;
    }
}
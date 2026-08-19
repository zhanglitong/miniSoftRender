#pragma     once

#include    "graphic/FEQueue.h"
#include    "graphic/RSObject.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGQueue :public TRSObject<WGPUQueue, FEQueue>
    {
    public:
        WGQueue(FEContext& ctx,WGPUQueue queue)
            :TRSObject<WGPUQueue, FEQueue>(ctx,queue)
        {}

        WGQueue(const WGQueue& other)
            :TRSObject<WGPUQueue, FEQueue>(other)
        {}
        virtual ~WGQueue();

        virtual bool    submit(uint cnt,const SubmitInfo* pInfo) override;
    };
}

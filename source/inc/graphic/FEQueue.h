#pragma     once

#include    "FEPipeline.h"
#include    "FEFence.h"
#include    "FECmdBuffer.h"
#include    "FESemaphore.h"
#include    "FEFrame.h"

namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   FE_API  FEQueue :public RSObject
    {
    public:
        struct  CreateInfo
        {};
        struct  SubmitInfo
        {
            Frame       _frame;
            uint        _mask   =   0;
        };
    public:
        FEQueue(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEQueue(const FEQueue& other)
            :RSObject(other)
        {}
        virtual bool    submit(uint cnt,const SubmitInfo* pInfo) =   0;
    };
    using   QCreateInfo =   FEQueue::CreateInfo;
    using   Queue       =   SharedPtr<FEQueue>;
    using   QueuePtr    =   SharedPtr<FEQueue>;
}
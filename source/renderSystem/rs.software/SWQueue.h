#pragma     once
#include    "graphic/FEQueue.h"
#include    "graphic/RSObject.h"

namespace   FE
{
    class   SWQueue :public TRSObject<void*,FEQueue>
    {
    public:
        SWQueue(FEContext& ctx,void* queue)
            :TRSObject<void*,FEQueue>(ctx,queue)
        {}

        SWQueue(const SWQueue& other)
            :TRSObject<void*,FEQueue>(other)
        {}
        virtual ~SWQueue();

        virtual bool    submit(uint cnt,const SubmitInfo* pInfo) override;
    };
}

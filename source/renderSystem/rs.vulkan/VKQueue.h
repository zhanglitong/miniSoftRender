#pragma     once
#include    "graphic/FEQueue.h"
#include    "graphic/RSObject.h"
#include    "FEVulkan.h"

namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   VKQueue :public TRSObject<VkQueue,FEQueue>
    {
    public:
        VKQueue(FEContext& ctx,VkQueue queue)
            :TRSObject<VkQueue,FEQueue>(ctx,queue)
        {}

        VKQueue(const VKQueue& other)
            :TRSObject<VkQueue,FEQueue>(other)
        {}
        virtual ~VKQueue();

        virtual bool    submit(uint cnt,const SubmitInfo* pInfo) override;
    };
}

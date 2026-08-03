#pragma     once

#include    "graphic/FESemaphore.h"
#include    "FEVulkan.h"
namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   VKSemaphore :public TRSObject<VkSemaphore,FESemaphore>
    {
    public:
        VKSemaphore(FEContext& ctx)
            :TRSObject<VkSemaphore,FESemaphore>(ctx)
        {}
        VKSemaphore(const VKSemaphore& other)
            :TRSObject<VkSemaphore,FESemaphore>(other)
        {}
        virtual ~VKSemaphore();
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& cInfo) override;
    };
}
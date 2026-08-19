#pragma     once

#include    "graphic/FEFence.h"
#include    "FEVulkan.h"
namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   VKFence :public TRSObject<VkFence,FEFence>
    {
    public:
        VKFence(FEContext& ctx)
            :TRSObject<VkFence,FEFence>(ctx)
        {}
        VKFence(const VKFence& other)
            :TRSObject<VkFence,FEFence>(other)
        {}
        virtual ~VKFence();

        virtual void    reset() override;
        virtual void    wait(uint64 tm) override;
        virtual bool    create(const CreateInfo& cInfo)  override;
    };
}

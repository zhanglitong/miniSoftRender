#pragma     once

#include    "graphic/FECmdPool.h"

#include    "FEVulkan.h"

namespace   FE
{
    class   VKCmdPool :public TRSObject<VkCommandPool,FECmdPool>
    {
    public:
        VKCmdPool(FEContext& ctx)
            :TRSObject<VkCommandPool,FECmdPool>(ctx)
        {
        }
        VKCmdPool(const VKCmdPool& other)
            :TRSObject<VkCommandPool,FECmdPool>(other)
        {}

        virtual ~VKCmdPool();

        virtual bool        create(const CreateInfo& cInf)  override;
        virtual CMDPtr      createCmd()    override;
    };
}

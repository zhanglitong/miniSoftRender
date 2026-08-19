#pragma     once

#include    "graphic/FERenderPass.h"

namespace   FE
{
    class   SWRenderPass :public TRSObject<void*,FERenderPass>
    {
    public:
        SWRenderPass(FEContext& ctx)
            :TRSObject<void*,FERenderPass>(ctx)
        {
        }
        SWRenderPass(const SWRenderPass& other)
            :TRSObject<void*,FERenderPass>(other)
        {}
        virtual ~SWRenderPass();

        virtual bool    create(const CreateInfo& cInfo)   override;
    };
}

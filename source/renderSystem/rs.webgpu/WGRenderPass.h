#pragma     once

#include    "graphic/FERenderPass.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGRenderPass :public TRSObject<void*, FERenderPass>
    {
    public:
        WGRenderPass(FEContext& ctx)
            :TRSObject<void*, FERenderPass>(ctx)
        {}
        WGRenderPass(const WGRenderPass& other)
            :TRSObject<void*, FERenderPass>(other)
        {}

        virtual ~WGRenderPass();

        virtual bool    create(const CreateInfo& cInf) override;
    };
}

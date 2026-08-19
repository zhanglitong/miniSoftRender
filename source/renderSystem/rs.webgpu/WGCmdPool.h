#pragma     once

#include    "graphic/FECmdPool.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGCmdPool :public TRSObject<void*, FECmdPool>
    {
    public:
        WGCmdPool(FEContext& ctx)
            :TRSObject<void*, FECmdPool>(ctx)
        {}
        WGCmdPool(const WGCmdPool& other)
            :TRSObject<void*, FECmdPool>(other)
        {}

        virtual ~WGCmdPool();

        virtual bool    create(const CreateInfo& cInf) override;
        virtual CMDPtr    createCmd() override;
    };
}

#pragma     once

#include    "graphic/FECmdPool.h"

namespace   FE
{
    class   SWCmdPool :public TRSObject<void*,FECmdPool>
    {
    public:
        SWCmdPool(FEContext& ctx)
            :TRSObject<void*,FECmdPool>(ctx)
        {
        }
        SWCmdPool(const SWCmdPool& other)
            :TRSObject<void*,FECmdPool>(other)
        {}

        virtual ~SWCmdPool();

        virtual bool        create(const CreateInfo& cInf)  override;
        virtual CMDPtr      createCmd()    override;
    };
}

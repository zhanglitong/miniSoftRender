#pragma     once

#include    "graphic/FEDSet.h"

namespace   FE
{
    class   SWDSet :public TRSObject<void*,FEDSet>
    {
    public:
        SWDSet(FEContext& ctx)
            :TRSObject<void*,FEDSet>(ctx)
        {
        }
        SWDSet(const SWDSet& other)
            :TRSObject<void*,FEDSet>(other)
        {}
        virtual~SWDSet();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual bool    update()    override;
    };
}

#pragma     once

#include    "graphic/FEDSetLayout.h"

namespace   FE
{
    class   SWDSetLayout :public TRSObject<void*,FEDSetLayout>
    {
    public:
        SWDSetLayout(FEContext& ctx)
            :TRSObject<void*,FEDSetLayout>(ctx)
        {
        }
        SWDSetLayout(const SWDSetLayout& other)
            :TRSObject<void*,FEDSetLayout>(other)
        {}

        virtual ~SWDSetLayout();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual DSet    createDSet()    override ;
    };
}

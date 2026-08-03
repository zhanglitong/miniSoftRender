#pragma     once

#include    "graphic/FEFence.h"
namespace   FE
{
    class   SWFence :public TRSObject<void*,FEFence>
    {
    public:
        SWFence(FEContext& ctx)
            :TRSObject<void*,FEFence>(ctx)
        {}
        SWFence(const SWFence& other)
            :TRSObject<void*,FEFence>(other)
        {}
        virtual ~SWFence();

        virtual void    reset() override;
        virtual void    wait(uint64 tm) override;
        virtual bool    create(const CreateInfo& cInfo)  override;
    };
}

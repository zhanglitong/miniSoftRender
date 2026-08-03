#include    "graphic/FEFence.h"

namespace   FE
{
    class   WGFence :public FEFence
    {
    public:
        WGFence(FEContext& ctx)
            :FEFence(ctx)
        {}
        WGFence(const WGFence& other)
            :FEFence(other)
        {}
        virtual ~WGFence();

        virtual bool    create(const CreateInfo& cInfo) override;
        virtual void    reset() override;
        virtual void    wait(uint64 tm) override;

        void    signal();

    protected:
        bool    _signaled    =   false;
    };
}
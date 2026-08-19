#include    "graphic/FESemaphore.h"

namespace   FE
{
    class   WGSemaphore :public FESemaphore
    {
    public:
        WGSemaphore(FEContext& ctx)
            :FESemaphore(ctx)
        {}
        WGSemaphore(const WGSemaphore& other)
            :FESemaphore(other)
        {}
        virtual ~WGSemaphore();

        virtual bool    create(const CreateInfo& cInfo) override;
    };
}

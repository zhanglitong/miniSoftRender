#pragma     once

#include    "graphic/FESemaphore.h"
namespace   FE
{
    class   SWSemaphore :public TRSObject<void*,FESemaphore>
    {
    public:
        SWSemaphore(FEContext& ctx)
            :TRSObject<void*,FESemaphore>(ctx)
        {}
        SWSemaphore(const SWSemaphore& other)
            :TRSObject<void*,FESemaphore>(other)
        {}
        virtual ~SWSemaphore();
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& cInfo) override;
    };
}

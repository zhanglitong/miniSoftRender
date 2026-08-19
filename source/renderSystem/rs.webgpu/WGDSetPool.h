#pragma     once

#include    "graphic/FEDSetPool.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGDSetPool :public TRSObject<void*, FEDSetPool>
    {
    public:
        WGDSetPool(FEContext& ctx)
            :TRSObject<void*, FEDSetPool>(ctx)
        {}
        WGDSetPool(const WGDSetPool& other)
            :TRSObject<void*, FEDSetPool>(other)
        {}

        virtual ~WGDSetPool();

        virtual bool    create(const CreateInfo& cInf) override;
        void    alloc();
        void    free();
    protected:
        uint32_t    _count  =   0;
    };
}

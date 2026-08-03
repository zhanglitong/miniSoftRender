#pragma     once

#include    "graphic/FEDSetPool.h"

namespace   FE
{
    class   SWDSetPool :public TRSObject<void*,FEDSetPool>
    {
    public:
        SWDSetPool(FEContext& ctx)
            :TRSObject<void*,FEDSetPool>(ctx)
        {
        }
        SWDSetPool(const SWDSetPool& other)
            :TRSObject<void*,FEDSetPool>(other)
        {}

        virtual ~SWDSetPool();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual bool    hasFree()
        {
            return  _hasAlloc < _cInfo._maxSets;
        }
        virtual void    alloc()
        {
            ++_hasAlloc;
        }
        virtual void    free()
        {
            --_hasAlloc;
        }
    protected:
        uint32_t    _hasAlloc       =   0;
    };

    using   DSetPoolSW  =   SharedPtr<SWDSetPool>;
}

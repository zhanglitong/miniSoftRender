#pragma     once

#include    "RSObject.h"
#include    "FEDSetLayout.h"
#include    "FEDSet.h"
namespace   FE
{
    class   FEDSetPool :public RSObject
    {
    public:
        struct  CreateInfo
        {
            DSetLayouts     _layouts;
            uint32_t        _maxSets    =   64;
        };
    public:
        FEDSetPool(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEDSetPool(const FEDSetPool& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;
    protected:
        CreateInfo  _cInfo;
    };

    using   DSetPool    =   SharedPtr<FEDSetPool>;
    using   DSetPools   =   SharedPtr<DSetPool>;
}
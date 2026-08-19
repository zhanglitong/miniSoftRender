#pragma     once

#include    "RSObject.h"

namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   FEFence :public RSObject
    {
    protected:
        struct  CreateInfo
        {};
    public:
        FEFence(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEFence(const FEFence& other)
            :RSObject(other)
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual void    reset() =   0;
        virtual void    wait(uint64 tm) =   0;
        virtual bool    create(const CreateInfo& cInfo)     =   0;
    protected:
        CreateInfo  _cInfo;
    };
    using   Fence   =   SharedPtr<FEFence>;
}

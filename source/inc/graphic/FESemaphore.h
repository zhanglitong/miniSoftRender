#pragma     once

#include    "RSObject.h"

namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   FESemaphore :public RSObject
    {
    protected:
        struct  CreateInfo
        {};
    public:
        FESemaphore(FEContext& ctx)
            :RSObject(ctx)
        {}
        FESemaphore(const FESemaphore& other)
            :RSObject(other)
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& cInfo) =   0;
    protected:
        CreateInfo  _cInfo;
    };
    using   Semaphore   =   SharedPtr<FESemaphore>;

    using   Semaphores  =   std::vector<Semaphore>;

}
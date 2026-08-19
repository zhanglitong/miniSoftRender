#pragma     once
#include    "RSObject.h"

namespace   FE
{
    class   FE_API  FETexture : public RSObject
    {
    public:
        FETexture(FEContext& ctx)
            :RSObject(ctx)
        {}
        FETexture(const FETexture& other)
            :RSObject(other)
        {}
    };

    using   TexPtr  =   SharedPtr<FETexture>;
    using   TexPtrs =   std::vector<FETexture>;
}

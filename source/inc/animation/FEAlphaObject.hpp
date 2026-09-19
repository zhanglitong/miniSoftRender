#pragma     once
#include    "TValueArray.hpp"


namespace FE
{   
    DEFINE_CLASS_UUID(FEAlphaObject, "{9089721A-3877-4CA4-8ECD-79983A22603F}");
    class   FE_API  FEAlphaObject :public TValueArray<uint8>
    {
        IMPLEMENT_CLASS_REFLECT(FEAlphaObject)
    public:
        using   ValueType   =   TValueArray<uint8>::ValueType;
    public:
        FEAlphaObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEAlphaObject(const FEAlphaObject& other)
            :TValueArray(other)
        {}
        ~FEAlphaObject()   = default;
    };
    using   AlphaObject =   SharedPtr<FEAlphaObject>;
}

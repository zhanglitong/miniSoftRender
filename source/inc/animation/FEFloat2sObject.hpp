#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEFloat2sObject, "{4ED63D9F-EEB4-4506-8A77-50BE2CD4529D}");
    class   FE_API  FEFloat2sObject :public TValueArray<float2>
    {
        IMPLEMENT_CLASS_REFLECT(FEFloat2sObject)
    public:
        using   ValueType   =   TValueArray<float2>::ValueType;
    public:
        FEFloat2sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEFloat2sObject(const FEFloat2sObject& other)
            :TValueArray(other)
        {}
        ~FEFloat2sObject()   = default;
    };
    using   Float2sObject     =   SharedPtr<FEFloat2sObject>;
}

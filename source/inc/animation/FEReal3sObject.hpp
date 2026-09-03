#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEReal3sObject, "{39994444-4CD5-41A6-BEC9-A18F863EEC85}");
    class   FEReal3sObject :public TValueArray<real3>
    {
        IMPLEMENT_CLASS_REFLECT(FEReal3sObject)
    public:
        using   ValueType   =   TValueArray<real3>::ValueType;
    public:
        FEReal3sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEReal3sObject(const FEReal3sObject& other)
            :TValueArray(other)
        {}
        ~FEReal3sObject()  =   default;
    };
    using   Real3sObject     =   SharedPtr<FEReal3sObject>;
}

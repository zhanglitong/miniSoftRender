#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEUint3sObject, "{A1B2C3D4-1001-4001-8001-000000000007}");
    class   FE_API  FEUint3sObject :public TValueArray<uint3>
    {
        IMPLEMENT_CLASS_REFLECT(FEUint3sObject)
    public:
        using   ValueType   =   TValueArray<uint3>::ValueType;
    public:
        FEUint3sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEUint3sObject(const FEUint3sObject& other)
            :TValueArray(other)
        {}
        ~FEUint3sObject()  = default;
    };
    using   Uint3sObject    =   SharedPtr<FEUint3sObject>;
}

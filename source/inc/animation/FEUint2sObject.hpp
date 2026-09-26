#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEUint2sObject, "{A1B2C3D4-1001-4001-8001-000000000006}");
    class   FE_API  FEUint2sObject :public TValueArray<uint2>
    {
        IMPLEMENT_CLASS_REFLECT(FEUint2sObject)
    public:
        using   ValueType   =   TValueArray<uint2>::ValueType;
    public:
        FEUint2sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEUint2sObject(const FEUint2sObject& other)
            :TValueArray(other)
        {}
        ~FEUint2sObject()  = default;
    };
    using   Uint2sObject    =   SharedPtr<FEUint2sObject>;
}

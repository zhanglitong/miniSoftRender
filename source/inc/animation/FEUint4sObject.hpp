#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEUint4sObject, "{A1B2C3D4-1001-4001-8001-000000000008}");
    class   FE_API  FEUint4sObject :public TValueArray<uint4>
    {
        IMPLEMENT_CLASS_REFLECT(FEUint4sObject)
    public:
        using   ValueType   =   TValueArray<uint4>::ValueType;
    public:
        FEUint4sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEUint4sObject(const FEUint4sObject& other)
            :TValueArray(other)
        {}
        ~FEUint4sObject()  = default;
    };
    using   Uint4sObject    =   SharedPtr<FEUint4sObject>;
}

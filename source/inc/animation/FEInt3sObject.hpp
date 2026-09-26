#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEInt3sObject, "{A1B2C3D4-1001-4001-8001-000000000003}");
    class   FE_API  FEInt3sObject :public TValueArray<int3>
    {
        IMPLEMENT_CLASS_REFLECT(FEInt3sObject)
    public:
        using   ValueType   =   TValueArray<int3>::ValueType;
    public:
        FEInt3sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEInt3sObject(const FEInt3sObject& other)
            :TValueArray(other)
        {}
        ~FEInt3sObject()  = default;
    };
    using   Int3sObject     =   SharedPtr<FEInt3sObject>;
}

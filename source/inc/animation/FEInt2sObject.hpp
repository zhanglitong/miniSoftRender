#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEInt2sObject, "{A1B2C3D4-1001-4001-8001-000000000002}");
    class   FE_API  FEInt2sObject :public TValueArray<int2>
    {
        IMPLEMENT_CLASS_REFLECT(FEInt2sObject)
    public:
        using   ValueType   =   TValueArray<int2>::ValueType;
    public:
        FEInt2sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEInt2sObject(const FEInt2sObject& other)
            :TValueArray(other)
        {}
        ~FEInt2sObject()  = default;
    };
    using   Int2sObject     =   SharedPtr<FEInt2sObject>;
}

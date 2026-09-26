#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEInt4sObject, "{A1B2C3D4-1001-4001-8001-000000000004}");
    class   FE_API  FEInt4sObject :public TValueArray<int4>
    {
        IMPLEMENT_CLASS_REFLECT(FEInt4sObject)
    public:
        using   ValueType   =   TValueArray<int4>::ValueType;
    public:
        FEInt4sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEInt4sObject(const FEInt4sObject& other)
            :TValueArray(other)
        {}
        ~FEInt4sObject()  = default;
    };
    using   Int4sObject     =   SharedPtr<FEInt4sObject>;
}

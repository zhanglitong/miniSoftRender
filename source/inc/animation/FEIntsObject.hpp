#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEIntsObject, "{A1B2C3D4-1001-4001-8001-000000000001}");
    class   FE_API  FEIntsObject :public TValueArray<int>
    {
        IMPLEMENT_CLASS_REFLECT(FEIntsObject)
    public:
        using   ValueType   =   TValueArray<int>::ValueType;
    public:
        FEIntsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEIntsObject(const FEIntsObject& other)
            :TValueArray(other)
        {}
        ~FEIntsObject()  = default;
    };
    using   IntsObject      =   SharedPtr<FEIntsObject>;
}

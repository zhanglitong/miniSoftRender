#pragma     once
#include    "TValueArray.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEUintsObject, "{A1B2C3D4-1001-4001-8001-000000000005}");
    class   FE_API  FEUintsObject :public TValueArray<uint>
    {
        IMPLEMENT_CLASS_REFLECT(FEUintsObject)
    public:
        using   ValueType   =   TValueArray<uint>::ValueType;
    public:
        FEUintsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEUintsObject(const FEUintsObject& other)
            :TValueArray(other)
        {}
        ~FEUintsObject()  = default;
    };
    using   UintsObject     =   SharedPtr<FEUintsObject>;
}

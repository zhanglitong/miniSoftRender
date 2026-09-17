#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEFloatsObject, "{16D4E9B9-5FD5-4ECC-854A-B0326BAF3808}");
    class   FE_API  FEFloatsObject :public TValueArray<float>
    {
        IMPLEMENT_CLASS_REFLECT(FEFloatsObject)
    public:
        using   ValueType   =   TValueArray<float>::ValueType;
    public:
        FEFloatsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEFloatsObject(const FEFloatsObject& other)
            :TValueArray(other)
        {}
        ~FEFloatsObject()   = default;
    };
    using   FloatsObject     =   SharedPtr<FEFloatsObject>;
}

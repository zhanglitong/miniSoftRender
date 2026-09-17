#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEBoolsObject, "{44C6E51E-DFF9-4F06-8E36-F9BFB06BF9B4}");
    class   FE_API  FEBoolsObject :public TValueArray<uint8>
    {
        IMPLEMENT_CLASS_REFLECT(FEBoolsObject)
    public:
        using   ValueType   =   TValueArray<uint8>::ValueType;
    public:
        FEBoolsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEBoolsObject(const FEBoolsObject& other)
            :TValueArray(other)
        {}
        ~FEBoolsObject()    =   default;
    };
    using   BoolsObject     =   SharedPtr<FEBoolsObject>;
}

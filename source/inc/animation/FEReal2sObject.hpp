#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEReal2sObject, "{DA8B20FA-F296-4AEC-9C3C-A162BF5ED89A}");
    class   FEReal2sObject :public TValueArray<real2>
    {
        IMPLEMENT_CLASS_REFLECT(FEReal2sObject)
    public:
        using   ValueType   =   TValueArray<real2>::ValueType;
    public:
        FEReal2sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEReal2sObject(const FEReal2sObject& other)
            :TValueArray(other)
        {}
        ~FEReal2sObject()  =   default;
    };
    using   Real2sObject     =   SharedPtr<FEReal2sObject>;
}

#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEFloat3sObject, "{024FE907-6DA1-4A65-A7BD-6899D7CE98C5}");
    class   FE_API  FEFloat3sObject :public TValueArray<float3>
    {
        IMPLEMENT_CLASS_REFLECT(FEFloat3sObject)
    public:
        using   ValueType   =   TValueArray<float3>::ValueType;
    public:
        FEFloat3sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEFloat3sObject(const FEFloat3sObject& other)
            :TValueArray(other)
        {}
        ~FEFloat3sObject()  =   default;
    };
    using   Float3sObject     =   SharedPtr<FEFloat3sObject>;
}

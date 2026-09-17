#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEFloat4sObject, "{6FE87B8C-F5EB-420D-8164-7D53842C9841}");
    class   FE_API  FEFloat4sObject :public TValueArray<float4>
    {
        IMPLEMENT_CLASS_REFLECT(FEFloat4sObject)
    public:
        using   ValueType   =   TValueArray<float4>::ValueType;
    public:
        FEFloat4sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEFloat4sObject(const FEFloat4sObject& other)
            :TValueArray(other)
        {}
        ~FEFloat4sObject()  =   default;
    };
    using   Float4sObject     =   SharedPtr<FEFloat4sObject>;
}

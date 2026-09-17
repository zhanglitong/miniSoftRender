#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEReal4sObject, "{E3F86B94-D7E3-4A8E-B85E-BC880C551588}");
    class   FE_API  FEReal4sObject :public TValueArray<real4>
    {
        IMPLEMENT_CLASS_REFLECT(FEReal4sObject)
    public:
        using   ValueType   =   TValueArray<real4>::ValueType;
    public:
        FEReal4sObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEReal4sObject(const FEReal4sObject& other)
            :TValueArray(other)
        {}
        ~FEReal4sObject()  =   default;
    };
    using   Real4sObject     =   SharedPtr<FEReal4sObject>;
}

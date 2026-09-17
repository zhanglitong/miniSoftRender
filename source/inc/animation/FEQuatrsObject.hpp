#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEQuatrsObject, "{2D4104C9-3103-4DEA-910A-B7891F2F9C24}");
    class   FE_API  FEQuatrsObject :public TValueArray<quatr>
    {
        IMPLEMENT_CLASS_REFLECT(FEQuatrsObject)
    public:
        using   ValueType   =   TValueArray<quatr>::ValueType;
    public:
        FEQuatrsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEQuatrsObject(const FEQuatrsObject& other)
            :TValueArray(other)
        {}
        ~FEQuatrsObject()= default;
    };
    using   QuatrsObject     =   SharedPtr<FEQuatrsObject>;
}

#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FEQuatfsObject, "{66E5B138-056B-4596-9689-D425682E5430}");
    class   FE_API  FEQuatfsObject :public TValueArray<quatf>
    {
        IMPLEMENT_CLASS_REFLECT(FEQuatfsObject)
    public:
        using   ValueType   =   TValueArray<quatf>::ValueType;
    public:
        FEQuatfsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEQuatfsObject(const FEQuatfsObject& other)
            :TValueArray(other)
        {}
        ~FEQuatfsObject()   =   default;
    };
    using   QuatfsObject     =   SharedPtr<FEQuatfsObject>;
}

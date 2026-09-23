#pragma     once
#include    "TValueArray.hpp"

namespace FE
{   
    DEFINE_CLASS_UUID(FERealsObject, "{81E552B1-58DE-4394-8B1F-D95FB092DBDF}");
    class   FE_API  FERealsObject 
        :public TValueArray<real>
    {
        IMPLEMENT_CLASS_REFLECT(FERealsObject)
    public:
        using   ValueType   =   TValueArray<real>::ValueType;
    public:
        FERealsObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FERealsObject(const FERealsObject& other)
            :TValueArray(other)
        {}
        ~FERealsObject()  =   default;
    };
    using   RealsObject     =   SharedPtr<FERealsObject>;
}

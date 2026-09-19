#pragma     once
#include    "TValueArray.hpp"


namespace FE
{   
    DEFINE_CLASS_UUID(FEColorObject, "{63370FBC-85AC-4784-B5DC-2D78AB9A69AF}");
    class   FE_API  FEColorObject :public TValueArray<uint8x4>
    {
        IMPLEMENT_CLASS_REFLECT(FEColorObject)
    public:
        using   ValueType   =   TValueArray<uint8x4>::ValueType;
    public:
        FEColorObject(FEContext& ctx)
            :TValueArray(ctx)
        {}
        FEColorObject(const FEColorObject& other)
            :TValueArray(other)
        {}
        ~FEColorObject()   = default;
    };
    using   RgbaObject     =   SharedPtr<FEColorObject>;
}

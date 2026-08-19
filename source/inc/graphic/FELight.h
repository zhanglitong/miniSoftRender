#pragma     once
#include    "FEObject.h"
#include    "FEShaderDefine.h"

namespace   FE
{
    class   FE_API  FELight : public FEObject
    {
    public:
        enum  LightType
        {
            LT_Dir      =   LightTypeDir,
            LT_Point    =   LightTypePoint,
            LT_Spot     =   LightTypeSpot,
        };
    public:
        FELight(FEContext& ctx)
            :FEObject(ctx)
        {}
        FELight(const FELight& other)
            :FEObject(other)
        {}
        LightType   type() const
        {
            return  _type;
        }
    protected:
        LightType   _type;
    };

    using   Light       =   SharedPtr<FELight>;
    using   Lights      =   std::vector<Light>;
}

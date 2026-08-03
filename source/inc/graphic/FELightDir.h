#pragma     once
#include    "FELight.h"

namespace   FE
{
    class   FELightDir : public FELight
    {
    public:
        FELightDir(FEContext& ctx)
            :FELight(ctx)
        {
            _type   =   LT_Dir;
        }
        FELightDir(const FELightDir& other)
            :FELight(other)
        {}
    public:
        float3  _color      =   float3(1,1,1);
        float3  _dir        =   float3(0,0,1);
        float   _radius     =   5;
    };

    using   Light       =   SharedPtr<FELight>;
    using   Lights      =   std::vector<Light>;
}
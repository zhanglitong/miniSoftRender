#pragma     once
#include    "FELight.h"

namespace   FE
{
    class   FELightPoint : public FELight
    {
    public:
        FELightPoint(FEContext& ctx)
            :FELight(ctx)
        {
            _type   =   LT_Point;
        }
        FELightPoint(const FELightPoint& other)
            :FELight(other)
        {}
    public:
        float3  _color      =   float3(1,1,1);
        float3  _pos        =   float3(0,0,0);
        float   _radius     =   5;
    };

    using   Light       =   SharedPtr<FELight>;
    using   Lights      =   std::vector<Light>;
}

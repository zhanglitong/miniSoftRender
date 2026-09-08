#pragma     once

#include    "FEAxis.h"
#include    <vector>
namespace   FE
{
    class FE_API FEBrowseAxisMove:public FEBrowseAxis
    {
    private:
        static  constexpr   float   COLOR_ALPHA    =   0.7f;
        static  constexpr   float   COLOR_VALUE    =   0.6f;
        static  constexpr   float   AXIS_WIDTH     =   6.0f;
        ///轴
        FE::float3                  _moveAxis[4];
        ///箭头
        std::vector<float3>         _axisAr;
    public:
        FEBrowseAxisMove(FEContext& context);
        ~FEBrowseAxisMove();
    public:
        void    updateAxisVerties(FEContext& context);
    };
}

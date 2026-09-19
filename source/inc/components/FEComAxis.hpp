#pragma     once

#include    "../FEInputCom.hpp"
/// <summary>
/// FEComAxis.h
/// 定义移动编辑坐标轴
/// </summary>
namespace   FE
{
    class   FE_API  FEComAxis: public FEInputCom
    {
    public:
        FEComAxis(FEContext& ctx);

        FEComAxis(const FEComAxis& other);

        ~FEComAxis();
       
    };

    using   ComAxis    =   SharedPtr<FEComAxis>;
}

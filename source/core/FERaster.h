#pragma     once

#include    "../inc/FEDefine.h"
#include    "../inc/FEObject.h"
#include    "FERasterState.h"


namespace   FE
{
    /// <summary>
    /// 该类负责传统光栅化绘制
    /// 接口可以被重写，例如GPU绘制，光线追踪等
    /// </summary>
    class   FE_API  FERaster :public FEObject
    {
    public:
        FERaster();
    };

    using   RasterPtr   =   SharedPtr<FERaster>;
}
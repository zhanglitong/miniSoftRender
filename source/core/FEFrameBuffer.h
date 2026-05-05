#pragma     once

#include    "FEPBuffer.h"
#include    "FERenderPass.h"

namespace   FE
{
    /// <summary>
    /// 用来重载绘制结果的画布
    /// </summary>
    class   FE_API  FEFrameBuffer :public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEFrameBuffer)
    };

    using   FBOPtr  =   SharedPtr<FEFrameBuffer>;
}
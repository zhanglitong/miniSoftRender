#pragma     once

#include    "FEPBuffer.h"
#include    "FERenderPass.h"

namespace   FE
{
    /// <summary>
    /// 用来重载绘制结果的画布
    /// </summary>
    class   FE_API  FEFrameBuffer
    {
    public:
        FEPBuffer*  _color;
        FEPBuffer*  _depth;
    };

    using   FBOPtr  =   SharedPtr<FEFrameBuffer>;
}
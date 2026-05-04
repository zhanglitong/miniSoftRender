#pragma     once

#include    "FEFrameBuffer.h"
#include    "FEPipeline.h"
#include    "FEStruct.h"

namespace   FE
{
    /// <summary>
    /// 描述FBO等绘制关系，以及资源关系
    /// </summary>
    class   FE_API  FERenderPass :public FEObject
    {
    public:
    };

    using   RenderPassPtr   =   SharedPtr<FERenderPass>;
}
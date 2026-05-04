#pragma     once

#include    "FERenderContext.h"
#include    "FECmdBuffer.h"
#include    "FEBuffer.h"
#include    "FERenderPass.h"

namespace   FE
{
    /// <summary>
    /// 该类是整个系统的入口，通过该接口可以访问整个系统的所有资源数据
    /// </summary>
    class   FE_API  FERenderSystem
    {
    public:
        BufferPtr   createBuffer();
        CMDPtr      createCmdBuffer();
    };

    using   RenderSysPtr    =   SharedPtr<FERenderSystem>;
}
#pragma     once

#include    "../tools/shader/FEProgram.h"
#include    "FEStruct.h"
#include    "../inc/FEObject.h"
namespace   FE
{
    /// <summary>
    /// 用来描述如何绘制算法
    /// </summary>
    class   FE_API  FEPipeline
    {
    public:
        CallResult  create(const FEPipelineInfo& info);
    protected:
        FEPipelineInfo  _cInfo;
    };

    using   PipelinePtr =   SharedPtr<FEPipeline>;
}
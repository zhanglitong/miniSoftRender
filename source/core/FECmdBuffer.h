#pragma     once

#include    "FEBuffer.h"
#include    "FERenderPass.h"
#include    "FEFrameBuffer.h"
#include    "FEStruct.h"
#include    "FEPipeline.h"
#include    "FEDescriptor.h"

namespace   FE
{
    class   FE_API  FECmdBuffer 
        :public FEBuffer
    {
    public:
        FECmdBuffer();

        CallResult  reset();
        CallResult  begin(const FECmdBufferInfo& infor);

        CallResult  cmdBeginRenderPass(const FERPBeginInfo& info);
        CallResult  cmdEndRenderPass();

        CallResult  cmdBindPipeline(const FEPLBindInfo& info);
        CallResult  cmdBindDescriptor(const FEDescInfo& info);

        CallResult  end();
    protected:
        /// <summary>
        /// 
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        virtual CallResult  create(const FEBufferInfor&) override;
    };

    using   CMDPtr  =   SharedPtr<FECmdBuffer>;
}
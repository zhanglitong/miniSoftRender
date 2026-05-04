
#include    "FECmdBuffer.h"


namespace   FE
{
    FECmdBuffer::FECmdBuffer()
    {}

    CallResult  FECmdBuffer::reset()
    {
        _buffer.clear();
        return  CR_SUCCESS;
    }
    CallResult  FECmdBuffer::begin(const FECmdBufferInfo& infor)
    {
        return  CR_SUCCESS;
    }


    CallResult  FECmdBuffer::cmdBeginRenderPass(const FERPBeginInfo& info)
    {
        return  CR_SUCCESS;
    }
    CallResult  FECmdBuffer::cmdEndRenderPass()
    {
        return  CR_SUCCESS;
    }
    CallResult  FECmdBuffer::cmdBindPipeline(const FEPLBindInfo& info)
    {
        return  CR_SUCCESS;
    }

    CallResult  FECmdBuffer::cmdBindDescriptor(const FEDescInfo& info)
    {
        return  CR_SUCCESS;
    }

    CallResult  FECmdBuffer::end()
    {
        return  CR_SUCCESS;
    }


    CallResult  FECmdBuffer::create(const FEBufferInfor& infor) 
    {
        _buffer.reserve(infor._size);
        return  CallResult::CR_SUCCESS;
    }
}
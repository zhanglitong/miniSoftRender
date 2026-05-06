
#include    "FECmdBuffer.h"


namespace   FE
{
    FECmdBuffer::FECmdBuffer()
    {}

    CallResult  FECmdBuffer::reset()
    {
        _buffer.clear();
        return  CallResult::CR_SUCCESS;
    }
    CallResult  FECmdBuffer::begin(const FECmdBufferInfo& infor)
    {
        return  CallResult::CR_FAILED; 
    }


    CallResult  FECmdBuffer::cmdBeginRenderPass(const FERPBeginInfo& info)
    {
        return  CallResult::CR_FAILED; 
    }
    CallResult  FECmdBuffer::cmdEndRenderPass()
    {
        return  CallResult::CR_FAILED; 
    }
    CallResult  FECmdBuffer::cmdBindPipeline(const FEPLBindInfo& info)
    {
        return  CallResult::CR_FAILED; 
    }

    CallResult  FECmdBuffer::cmdBindDescriptor(const FEDescInfo& info)
    {
        return  CallResult::CR_FAILED; 
    }

    CallResult  FECmdBuffer::cmdDraw( uint32_t vertexCount
                                    , uint32_t instanceCount
                                    , uint32_t firstVertex
                                    , uint32_t firstInstance)
    {
        return  CallResult::CR_FAILED; 
    }

    CallResult  FECmdBuffer::cmdDrawIndexed(  uint32_t indexCount
                                            , uint32_t instanceCount
                                            , uint32_t firstIndex
                                            , int32_t  vertexOffset
                                            , uint32_t firstInstance)
    {
        return  CallResult::CR_FAILED; 
    }

    CallResult  FECmdBuffer::end()
    {
        return  CallResult::CR_FAILED; 
    }


    CallResult  FECmdBuffer::create(const FEBufferInfor& infor) 
    {
        _buffer.reserve(infor._size);
        return  CallResult::CR_SUCCESS;
    }
}
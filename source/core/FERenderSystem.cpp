#include    "FERenderSystem.h"
#include    "FECmdBuffer.h"

namespace   FE
{
    BufferPtr   FERenderSystem::createBuffer()
    {
        return  BufferPtr(new FEBuffer());
    }
    CMDPtr      FERenderSystem::createCmdBuffer()
    {
        return  CMDPtr(new FECmdBuffer());
    }
}
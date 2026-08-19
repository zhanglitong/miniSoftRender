#include    "SWCmdPool.h"
#include    "SWDevice.h"
#include    "SWCmdBuffer.h"

namespace   FE
{
    SWCmdPool::~SWCmdPool()
    {
    }

    bool    SWCmdPool::create(const FECmdPool::CreateInfo& cInf)
    {
        (void)cInf;
        return  true;
    }

    CMDPtr  SWCmdPool::createCmd()
    {
        CMDPtr                  cmd     =    new SWCmdBuffer(_ctx);
        FECmdBuffer::CreateInfo cInfo   =    {this};
        if (cmd->create(cInfo))
            return  cmd;
        else
            return  nullptr;
    }
}

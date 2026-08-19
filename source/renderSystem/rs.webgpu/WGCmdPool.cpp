#include    "WGCmdPool.h"
#include    "WGDevice.h"
#include    "WGCmdBuffer.h"

namespace   FE
{
    WGCmdPool::~WGCmdPool()
    {
    }

    bool WGCmdPool::create(const CreateInfo& cInf)
    {
        (void)cInf;
        return true;
    }

    CMDPtr WGCmdPool::createCmd()
    {
        auto cmd = new WGCmdBuffer(_ctx);
        FECmdBuffer::CreateInfo cmdInfo;
        cmdInfo._pool =   this;
        if (cmd->create(cmdInfo))
        {
            return cmd;
        }
        delete cmd;
        return nullptr;
    }
}

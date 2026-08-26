
#include    "../inc/graphic/FEFrustumCull.h"

namespace   FE
{
    void    FEFrustumCull::doCull(Camera camera,const RFactorys& factorys)
    {
        UNUSED(camera);
        Materials   mats;
        for (auto  var : factorys)
        {
            if (!var->supportGPUCull())
                continue;
            auto    result  =   var->getOrCreateCullMaterials();
            if (result.empty()) 
                continue;
            mats.insert(mats.end(),result.begin(),result.end());
        }
        if (mats.empty())
            return;
        Material    mat     =   mats.front();
        auto        pl      =   mat->pipeline(PRI_POINTS);
        auto        cmdPool =   _ctx.device().computeCmdPool();
        CMDPtr      cmd     =   cmdPool->createCmd();
        cmd->begin(true);
        {
            cmd->bindPipeline(pl);
            for (auto& var: mats)
            {
                cmd->dispatch(1,1,1);
            }
        }
        cmd->end();
        cmd->submit(_ctx.device().queueCompute());
    }

}

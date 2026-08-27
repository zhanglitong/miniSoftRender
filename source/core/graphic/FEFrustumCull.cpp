
#include    "../inc/graphic/FEFrustumCull.h"
#include    "../inc/material/FEMaterialCull.hpp"

namespace   FE
{
    void    FEFrustumCull::compute(Camera camera,const RFactorys& factorys)
    {
        Materials   mats;
        for (auto  var : factorys)
        {
            if (!var->supportGPUCull())
                continue;
            auto    result  =   var->getOrCreateCullMaterials(camera);
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
                auto    cullMat =   var->as<FEMaterialCull>();
                uint    nLength =   cullMat->_cullParam._value._count;
                uint    xGrp    =   nLength/uint(CULL_GROUP_X);
                if ( nLength % uint(CULL_GROUP_X))
                    xGrp    +=  1;

                FECmdBuffer::DSetBind   binds   =   {};
                binds.dSets         =   var->dsets();
                binds.firstSet      =   0;
                binds.offsetCount   =   0;
                binds.offsets       =   nullptr;
                binds.plBindPoint   =   PL_COMPUTE;
                binds.plLayout      =   pl->nativeLayout();
                cmd->bindDescriptors(binds);
                cmd->dispatch(xGrp,1,1);
            }
        }
        cmd->end();
        cmd->submit(_ctx.device().queueCompute());
    }

}

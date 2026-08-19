
#include    "FEProgram.h"

namespace   FE
{
    bool    FEProgram::create(const FEProgramCreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }
    bool    FEProgram::link(ShaderPtr vs,ShaderPtr ps)
    {
        _cInfo._ps  =   ps;
        _cInfo._vs  =   vs;
        return  true;
    }

    ProgramPtr  FEProgram::create(const char* vs,const char* ps)
    {
        auto    pVS   =   ShaderPtr(new FEShader());
        if (!pVS->compile({vs}))
            return  nullptr;
        auto    pPS   =   ShaderPtr(new FEShader());
        if (!pPS->compile({ps}))
            return  nullptr;

        FEProgramCreateInfo   info;
        info._vs    =   pVS;
        info._ps    =   pPS;
        ProgramPtr  prg =   ProgramPtr(new FEProgram());
        if (prg->create(info))
            return  prg;
        else
            return  nullptr;
    }
}


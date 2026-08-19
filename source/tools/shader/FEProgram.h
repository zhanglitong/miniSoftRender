#pragma     once

#include    "FEShader.h"
#include    "../../inc/FELog.h"

namespace   FE
{
    struct   FEProgramCreateInfo
    {
        ShaderPtr   _vs;
        ShaderPtr   _ps;
    };
    class   FE_API  FEProgram :public FEObject
    {
    public:
        using   ProgramPtr  =   SharedPtr<FEProgram>;
    public:
        bool    create(const FEProgramCreateInfo& info);
        bool    link(ShaderPtr vs,ShaderPtr ps);
    protected:
        FEProgramCreateInfo _cInfo;
    public:
        static  ProgramPtr  create(const char* vs,const char* ps);
    
    };
    using   ProgramPtr  =   SharedPtr<FEProgram>;
    using   Programs    =   std::vector<ProgramPtr>;
}

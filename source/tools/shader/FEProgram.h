#pragma     once

#include    "FEShader.h"
#include    "../../inc/FELog.h"

namespace   FE
{
    struct   FEProgramInfo
    {
        ShaderPtr   _vs;
        ShaderPtr   _ps;
    };
    class   FE_API  FEProgram :public FEObject
    {
    public:
        using   ProgramPtr  =   SharedPtr<FEProgram>;
    public:
        bool    create(const FEProgramInfo& info);
        bool    link(ShaderPtr vs,ShaderPtr ps);
    protected:
        FEProgramInfo   _cInfo;
    public:
        static  ProgramPtr  create(const char* vs,const char* ps);
    
    };
    using   ProgramPtr  =   SharedPtr<FEProgram>;
    using   Programs    =   std::vector<ProgramPtr>;
}

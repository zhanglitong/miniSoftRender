#pragma     once

#include    "FEShader.h"
#include    "../../inc/FELog.h"

namespace   FE
{
    class   FE_API  FEProgram :public FEObject
    {
    public:
        using   ProgramPtr  =   SharedPtr<FEProgram>;
    };
    using   ProgramPtr  =   SharedPtr<FEProgram>;
    using   Programs    =   std::vector<ProgramPtr>;
}


#include    "IProgram.h"

using   ProgramPtr  =   IProgram::ProgramPtr;

ShaderPtr   IProgram::createShader(const char* ,LogPtr )
{
    return  nullptr;
}
ProgramPtr  IProgram::link(const ShaderPtrs& ,LogPtr )
{
    return  nullptr;
}

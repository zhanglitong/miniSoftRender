#pragma     once

#include    "FEShader.h"

namespace   FE
{

    class   FEShaderHelper
    {
    public:
       static   Shader  buildFromMemory(FEContext& ctx,FEDevice& dev,const char* source,FEShaderType type,const char* entryPoint = "main");

       static   Shader  buildFromFile(FEContext& ctx,FEDevice& dev,const char* fileName,FEShaderType type,const char* entryPoint = "main");
    };

}



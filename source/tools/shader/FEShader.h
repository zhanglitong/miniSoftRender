#pragma     once
#include    <iostream>
#include    <stdio.h>
#include    <vector>
#include    <memory>

#include    "../../inc/FEDefine.h"
#include    "../../inc/FELog.h"
#include    "../../inc/FEObject.h"

namespace   FE
{
    struct  FEShaderCreateInfo
    {
        const char* source;
    };
    class   FE_API FEShader :public FEObject
    {
    public:
        bool    compile(const FEShaderCreateInfo& info);
    };

    using   ShaderPtr   =   SharedPtr<FEShader>;
    using   Shaders     =   std::vector<ShaderPtr>;
}



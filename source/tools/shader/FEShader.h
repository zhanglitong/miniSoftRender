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

    class   FE_API FEShader :public FEObject
    {};

    using   ShaderPtr   =   SharedPtr<FEShader>;
    using   Shaders     =   std::vector<ShaderPtr>;
}



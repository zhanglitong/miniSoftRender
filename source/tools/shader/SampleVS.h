#pragma     once
#include    "FEShaderRT.h"
#include    "../../inc/FEMath.hpp"

namespace   FE
{
    struct  Shader
    {
        struct Resource : public VSResource
        {};
    
        inline  Resource*   getResource()
        {
            return  _resource;
        }
        inline  void        setResource(VSResource* res)
        {
            _resource   =   static_cast<Resource*>(res);
        }
        inline  void        main()
        {
        }
    protected:
        Resource*   _resource   =   nullptr;
    };
}  


using   SampleVS    =   FE::TVertexShader<FE::Shader, FE::Shader::Resource>;
DEFINE_SHADER(SampleVS,vs.default);
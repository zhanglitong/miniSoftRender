#pragma     once
#include    "FEShaderRT.h"
#include    "../inc/FEMath.hpp"

namespace   FE
{
    struct  ResourceVS : public VSResource
    {
        struct  UBO
        {
            mat4    m;
            mat4    v;
            mat4    p;
            mat4    mvp;
        };
        IResource<UBO>      _ubo;
        IInput<float3>      _position;
        IOutput<float3>     _outPos;

        ResourceVS()
        {
        }

        void    init(FEShaderRT& rt)
        {
            VSResource::init(rt);
            rt.regResource(_ubo,    0);
            rt.regInput(_position,  0);
            rt.regOutput(_outPos,   0);            
        }
    };
    template<class TResource>
    struct  VSShader
    {
#define ubo         res._ubo.get()
#define position    res._position.get()

        inline  void    main(TResource& res)
        {
            float4  xx ;
            mat4    mm;
            float4  gg  =   mm * xx; 
            float3  pos =   position;
            auto    pp  =   ubo.mvp * float4(pos.x,pos.y,pos.z,1.0f);

            (res.gl_Position.get())  =   pp;
        }
    };

    struct  ResourcePS : public FSResource
    {
        void    init(FEShaderRT& rt)
        {
            FSResource::init(rt);           
        }
    };

    template<class TResource>
    struct  PSShader
    {
        inline  void    main(TResource& resource)
        { 
        }
    };
}

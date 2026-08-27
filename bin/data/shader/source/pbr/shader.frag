
#version    450
#include    "FEShaderDefine.h"

layout  (location = 0)          in vec3     inPos;
layout  (location = 1)          in vec3     inNor;
layout  (location = 2)          in vec4     inNodeColor;
layout  (location = 3)  flat    in uint     inFlagBits;


layout (location = 0) out vec4 fragColor;


layout (binding = SB_Material) uniform PBRBlock
{
    PBRData        _pbr;
};

layout(std140, binding = SB_Light) readonly buffer LightsBlock
{
    LightData    _lights[];
};

void main() 
{
    fragColor   =   vec4(0,0,0,1);
    
    vec3    diff    =   inFlagBits == 0 ? _pbr._diffuse.rgb : inNodeColor.rgb * 0.1 + _pbr._diffuse.rgb * 0.9;
    int numLights = _lights.length();
    for(int i = 0; i < numLights; ++i)
    {
        vec3    lDir    =   vec3(_lights[i].x,_lights[i].y,_lights[i].z);
        vec3    lColor  =   vec3(_lights[i].r,_lights[i].g,_lights[i].b);
        
        float   NdotL   =   max(0.1, dot(normalize(inNor), lDir));
        vec3    diffuse =   diff * (lColor * NdotL) + _pbr._emissive.rgb ;

        fragColor.xyz   +=  diffuse;
    }
}
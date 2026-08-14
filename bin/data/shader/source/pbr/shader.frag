#version 450
#include "FEShaderDefine.h"

layout  (location = 0) in vec3 inPos;
layout  (location = 1) in vec3 inNor;

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
    
    int numLights = 4;  // Fixed number of lights
    for(int i = 0; i < numLights; ++i)
    {
        vec3    lPos    =   vec3(_lights[i].x,_lights[i].y,_lights[i].z);
        vec3    lColor  =   vec3(_lights[i].r,_lights[i].g,_lights[i].b);
        vec3    L       =   lPos - inPos;

        float   NdotL   =   max(0.0, dot(normalize(inNor), normalize(L)));
        vec3    diffuse =   lColor * NdotL * _pbr._emissive.rgb ;

        fragColor.xyz   +=  diffuse;
    }
}
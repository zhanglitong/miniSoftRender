
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

layout (binding = SB_EngineState) uniform EngineStateBlock
{
    EngineState  _engineState;
};


void main()
{
    fragColor       =   vec4(0,0,0,1);
    vec3    diff    =   hasNodeColor(inFlagBits) ? inNodeColor.rgb : _pbr._diffuse.rgb;
    int numLights = _lights.length();
    for(int i = 0; i < numLights; ++i)
    {
        vec3    lDir    =   vec3(_lights[i].x,_lights[i].y,_lights[i].z);
        vec3    lColor  =   vec3(_lights[i].r,_lights[i].g,_lights[i].b);

        float   NdotL   =   max(0.1, dot(normalize(inNor), lDir));
        vec3    diffuse =   diff * (NdotL) + _pbr._emissive.rgb ;

        fragColor.xyz   +=  diffuse;
    }
    /// 选中高亮:与高亮色混合
    if (isSelected(inFlagBits))
    {
        vec4    hl  =   unpackColor(_engineState.selectColor);
        fragColor.rgb   =   mix(fragColor.rgb, hl.rgb, 0.5);
    }
}
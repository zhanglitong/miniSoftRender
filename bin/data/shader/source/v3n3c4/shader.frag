#version 450

#include "FEShaderDefine.h"

layout (location = 0) in        vec4    inColor;
layout (location = 1) in        vec3    inNor;
layout (location = 2) flat  in  uint    inFlagBits;
layout (location = 0) out       vec4    outFragColor;

layout (binding = SB_Global) uniform EngineGlobalBlock
{
    UBOSky       _sky;
    EngineState  _engineState;
};

void main()
{
    vec3    n       =   normalize(inNor);
    /// 固定方向光 + 环境光:让挤出的厚度与侧壁法线呈现明暗层次
    vec3    lightDir=   normalize(vec3(-0.35, 0.75, 0.55));
    float   diff    =   max(dot(n, lightDir), 0.0);
    vec3    color   =   inColor.rgb * (0.35 + 0.65 * diff);

    /// 选中高亮
    if (isSelected(inFlagBits))
    {
        vec4    hl  =   unpackColor(_engineState.selectColor);
        color       =   mix(color, hl.rgb, 0.5);
    }
    color  *=  skyEnvGainDisplay(_sky);

    outFragColor    =   vec4(color, inColor.a);
}

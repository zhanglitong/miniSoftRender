#version 450

#include "FEShaderDefine.h"

layout (location = 0) in        vec4    inColor;
layout (location = 1) flat  in  uint    inFlagBits;
layout (location = 0) out       vec4    outFragColor;

layout (binding = SB_EngineState) uniform EngineStateBlock
{
    EngineState  _engineState;
};

void main()
{
    outFragColor    =   inColor;
    /// 选中高亮:与高亮色混合
    if (isSelected(inFlagBits))
    {
        vec4    hl          =   unpackColor(_engineState.selectColor);
        outFragColor.rgb    =   mix(outFragColor.rgb, hl.rgb, 0.5);
    }
}
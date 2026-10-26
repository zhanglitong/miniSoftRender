#version 450
#include "FEShaderDefine.h"

layout (location = 0) in vec3 inColor;
layout (location = 2) in vec4 inNodeColor;
layout (location = 3) flat in uint inFlagBits;

layout (location = 0) out vec4 outFragColor;

/// 引擎全局 UBO:声明完整前缀(_sky 在偏移 0),只使用 EngineState
layout (binding = SB_Global) uniform EngineStateBlock
{
    UBOSky       _sky;
    EngineState  _engineState;
};

void main() 
{
    outFragColor = vec4(inColor, 1.0);
    /// 选中高亮:与高亮色混合
    if (isSelected(inFlagBits))
    {
        vec4    hl  =   unpackColor(_engineState.selectColor);
        outFragColor.rgb = mix(outFragColor.rgb, hl.rgb, 0.5);
    }
}

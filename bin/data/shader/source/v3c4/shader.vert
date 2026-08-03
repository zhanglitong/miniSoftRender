#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout  (location = 1) in vec4  inColor;

layout  (location = 2) in vec4  instanceColor;
layout  (location = 3) in uint  instanceFlagBits;

layout  (location = 4) in mat4 matLocal;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout (location = 0)       out vec4    outColor;
layout (location = 1) flat  out uint    flagBits;

vec4   instColor()
{
    return (instanceFlagBits & RF_COLOR) == 0 ? vec4(1,1,1,1) : instanceColor;
}

void main() 
{
    
    outColor            =   inColor  * instColor() ;
    flagBits            =   instanceFlagBits;
    gl_Position         =   _camera._p * _camera._v  * matLocal * vec4(inPos.xyz, 1.0);
}

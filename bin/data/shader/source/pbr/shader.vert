#version 450

#include "FEShaderDefine.h"

layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec3    inNor;
layout  (location = 2)  in  vec4    matLocalC0;
layout  (location = 3)  in  vec4    matLocalC1;
layout  (location = 4)  in  vec4    matLocalC2;
layout  (location = 5)  in  vec4    matLocalC3;

layout  (location = 6)  in  vec4    nodeColor;
layout  (location = 7)  in  uint    flagBits;

layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData    _camera;
};

layout (location =  0)          out vec3    outPos;
layout (location =  1)          out vec3    outNor;
layout (location =  2)          out vec4    outColor;
layout (location =  3)  flat    out uint    outFlagBits;

void main() 
{
    mat4    matLocal=   mat4(matLocalC0, matLocalC1, matLocalC2, matLocalC3);
    gl_PointSize    =   1;
    vec4    tmp     =   (matLocal * vec4(inPos.xyz, 1.0));
    // Use normal matrix as transpose of inverse (assuming uniform scale)
    outPos          =   tmp.xyz;
    outFlagBits     =   flagBits;
    outColor        =   nodeColor;
    outNor          =   normalize(matLocal * vec4(inNor, 0.0)).xyz;
    gl_Position     =   _camera._p * _camera._v  * tmp;
}

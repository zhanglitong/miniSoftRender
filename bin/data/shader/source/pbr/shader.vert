#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3 inPos;
layout  (location = 1) in vec3 inNor;
layout  (location = 2) in mat4 matLocal;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData    _camera;
};

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNor;
void main() 
{
    gl_PointSize    =   1;
    vec4    tmp     =   (matLocal * vec4(inPos.xyz, 1.0));
    mat3    matNor  =   transpose(inverse(mat3(matLocal)));
    outPos          =   tmp.xyz;
    outNor          =   normalize(matNor * inNor);
    gl_Position     =   _camera._p * _camera._v  * tmp;
}

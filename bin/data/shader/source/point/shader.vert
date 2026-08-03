#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout  (location = 1) in vec4  inColor;
layout  (location = 2) in mat4  matLocal;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};

layout (location = 0) out vec4  outColor;

void main() 
{
    outColor            =   inColor;
    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    gl_PointSize        =   pointMin;
    gl_Position         =   _camera._p * _camera._v  * matLocal * vec4(inPos.xyz, 1.0);
}

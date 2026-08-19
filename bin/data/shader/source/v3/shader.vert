#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout  (location = 2) in vec4  matLocalC0;
layout  (location = 3) in vec4  matLocalC1;
layout  (location = 4) in vec4  matLocalC2;
layout  (location = 5) in vec4  matLocalC3;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};
layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};


void main() 
{
    mat4    matLocal    =   mat4(matLocalC0, matLocalC1, matLocalC2, matLocalC3);
    
    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    gl_PointSize        =   pointMin;
    gl_Position         =   _camera._p * _camera._v  * matLocal * vec4(inPos.xyz, 1.0);
}

#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};

void main() 
{
    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    gl_PointSize        =   pointMin;
    gl_Position         =   _point._mvp * vec4(inPos.xyz, 1.0);
}

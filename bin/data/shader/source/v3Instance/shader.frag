#version    450
#include    "FEShaderDefine.h"

layout (location = 0) out   vec4    fragColor;

layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};

void main() 
{
    vec4    color       =   unpackUnorm4x8(_point._color);

    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);
    uint    isPoint     =   _point._point & 0xFFU;
    if (isPoint != 0)
    {
        vec2    coord       =   gl_PointCoord - vec2(0.5, 0.5);
        float   dist        =   length(coord);
        if(dist > 0.5)
            discard;
    }
    fragColor           =   color;
}
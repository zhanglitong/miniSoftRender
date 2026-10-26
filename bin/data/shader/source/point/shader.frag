#version    450
#include    "FEShaderDefine.h"

layout (location = 0) in    vec4    inColor;
layout (location = 0) out   vec4    fragColor;

layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};
/// 天空环境(所有非 UI 材质共用):昼夜门控
layout (binding = SB_Global) uniform SkyBlock
{
    UBOSky  _sky;
};

void main() 
{
    vec4    color       =   unpackUnorm4x8(_point._color);

    float   override    =   float((_point._point >> 24) & 0xFFu);
    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    vec2    coord       =   gl_PointCoord - vec2(0.5, 0.5);
    float   dist        =   length(coord);
    if(dist > 0.5)
        discard;
    fragColor   =   inColor * (1.0 - override) + color * color;
    /// LDR 直出:用显示空间门控,与 PBR 夜间观感保持一致
    fragColor.rgb   *=  skyEnvGainDisplay(_sky);
}
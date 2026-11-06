#version 450

#include "FEShaderDefine.h"
#include "FESkyModel.glsl"

/// 天空环境烘焙片元着色器:
///   顶点着色器与 sky/shader.frag 共用(shader.vert),inDir 即从相机出发的世界方向。
///   与天空盒着色器的区别:不做 ACES 色调映射、不做 sRGB 编码,直接输出线性 HDR
///   辐亮度(不含日面),并按曝光(_env.z)与天空开关(_env.w)缩放。
///   采样该 cubemap 得到的数值与原 PBR "解析天空 IBL"分支
///   (skyDiffuseRadiance / skySpecularRadiance * skyExposure * skyOn)同量级,
///   因此烘焙后 PBR 只做纹理采样,不再逐像素求值大气+云。
layout  (location = 0)  in  vec3    inDir;
layout  (location = 0)  out vec4    outColor;

layout  (binding = SB_Global) uniform SkyBlock
{
    UBOSky  _sky;
};

void main()
{
    /// 不含日面:太阳直射已由方向光的 GGX 高光表达,烘进环境贴图会形成硬边白斑
    vec3    radiance    =   skyRadianceNoSun(inDir, _sky);
    outColor    =   vec4(radiance * _sky._env.z * _sky._env.w, 1.0);
}

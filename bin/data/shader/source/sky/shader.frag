#version 450

#include "FEShaderDefine.h"
#include "FESkyModel.glsl"

/// 天空盒(大气散射)片元着色器:
///   大气散射模型已抽到 common/FESkyModel.glsl,与 PBR 的解析环境光照共用。
///   本着色器对视线方向求值后做 ACES 色调映射 + sRGB 编码写入交换链;
///   PBR 侧则直接使用 skyRadiance() 的线性 HDR 结果作为 IBL 输入。
///   参数来自 SkyBlock(绑定槽 SB_Sky),由 FESkyComponent 写入 UBOSky。

layout  (location = 0)  in  vec3    inDir;
layout  (location = 0)  out vec4    outColor;

layout  (binding = SB_Sky) uniform SkyBlock
{
    UBOSky  _sky;
};

/// ---- 色调映射(严格对齐 three.js ACESFilmicToneMapping)----
/// 引擎输出到 UNORM 交换链,故在着色器内完成 HDR->LDR 与 sRGB 编码,
/// 数值与 three.js 的 tonemapping_fragment + colorspace_fragment 一致
vec3    acesInputMat(vec3 c)
{
    /// three.js 的 mat3 以列向量构造,这里用点积等价展开
    return  vec3(dot(c, vec3(0.59719, 0.35458, 0.04823)),
                 dot(c, vec3(0.07600, 0.90834, 0.01566)),
                 dot(c, vec3(0.02840, 0.13383, 0.83777)));
}
vec3    acesOutputMat(vec3 c)
{
    return  vec3(dot(c, vec3( 1.60475, -0.53108, -0.07367)),
                 dot(c, vec3(-0.10208,  1.10813, -0.00605)),
                 dot(c, vec3(-0.00327, -0.07276,  1.07602)));
}
vec3    rrtAndOdtFit(vec3 v)
{
    vec3    a   =   v * (v + 0.0245786) - 0.000090537;
    vec3    b   =   v * (0.983729 * v + 0.4329510) + 0.238081;
    return  a / b;
}
vec3    acesFilmicToneMapping(vec3 color,float exposure)
{
    color  *=  exposure / 0.6;
    color   =   acesInputMat(color);
    color   =   rrtAndOdtFit(color);
    color   =   acesOutputMat(color);
    return  clamp(color, 0.0, 1.0);
}
/// linear -> sRGB 传递函数(three.js sRGBTransferOETF)
vec3    linearToSRGB(vec3 c)
{
    return  mix(1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, c * 12.92,
                vec3(lessThanEqual(c, vec3(0.0031308))));
}

void main()
{
    vec3    texColor    =   skyRadiance(inDir, _sky);
    /// 曝光(UBOSky._sunDirection.w = three.js 的 toneMappingExposure,示例值 0.05)
    vec3    color       =   acesFilmicToneMapping(texColor, _sky._sunDirection.w);
    /// linear -> sRGB 编码后写入 UNORM 交换链
    outColor    =   vec4(linearToSRGB(color), 1.0);
}

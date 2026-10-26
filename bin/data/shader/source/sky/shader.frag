#version 450

#include "FEShaderDefine.h"

/// 天空盒(大气散射)片元着色器:
///   移植自 three.js examples webgl_shaders_sky(SkyShader / Preetham 大气散射模型),
///   实时计算天空颜色,不依赖任何贴图。
///   参数来自 SkyBlock(绑定槽 SB_Sky),由 FESkyComponent 写入 UBOSky。

layout  (location = 0)  in  vec3    inDir;
layout  (location = 0)  out vec4    outColor;

layout  (binding = SB_Sky) uniform SkyBlock
{
    UBOSky  _sky;
};

/// ---- 大气散射常量(与 three.js Sky 一致)----
const vec3  cameraPos   =   vec3(0.0);
const float PI          =   3.141592653589793238462643383279502884197169;
/// 空气折射率
const float n           =   1.0003;
/// 单位体积分子数(288.15K, 1013mb)
const float N           =   2.545e25;
/// 选用的三个主波长(Preetham)
const vec3  lambda      =   vec3(680e-9, 550e-9, 450e-9);
/// 地球-太阳距离=1 时的预计算瑞利散射总量(lambda=680/550/450nm,与 three.js Sky 一致)
const vec3  totalRayleigh = vec3(5.804542996261093e-6, 1.3562911419845635e-5, 3.0265902468824876e-5);
/// 天顶方向光学长度(分子/米氏)
const float rayleighZenithLength =   8.4e3;
const float mieZenithLength      =   1.25e3;
/// 太阳角直径余弦(66 角秒)
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;
/// 太阳强度相关常量
const float cutoffAngle =   1.6110731556870734;
const float steepness   =   1.5;
const float EE          =   1000.0;
/// 米氏散射系数常量
const vec3  MieConst    =   vec3(1.8399918514433978e14, 2.7798023919660528e14, 4.0790479543861094e14);

/// 瑞利相位函数
float   rayleighPhase(float cosTheta)
{
    return  (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}
/// Henyey-Greenstein 相位函数(米氏)
float   hgPhase(float cosTheta,float g)
{
    float   g2  =   g * g;
    float   inv =   1.0 / pow(max(0.0, 1.0 - 2.0 * g * cosTheta + g2), 1.5);
    return  (1.0 / (4.0 * PI)) * ((1.0 - g2) * inv);
}
/// 太阳强度(随天顶角余弦)
float   sunIntensity(float zenithAngleCos)
{
    zenithAngleCos  =   clamp(zenithAngleCos, -1.0, 1.0);
    return  EE * max(0.0, 1.0 - exp(-((cutoffAngle - acos(zenithAngleCos)) / steepness)));
}

/// ---- 云层噪声(移植 three.js Sky.js,源码逐行等价)----
/// 栅格角点上的梯度;使用无 sin 的哈希,保证各 GPU 输出一致
vec2    gradient(vec2 i)
{
    vec3    p   =   fract(i.xyx * vec3(0.1031, 0.1030, 0.0973));
    p          +=  dot(p, p.yzx + 33.33);
    /// 注意:必须先乘 p.zy 再取 fract(与 three.js 一致),
    /// 否则哈希退化为低频,云会糊成一大片
    return  fract((p.xx + p.yz) * p.zy) * 2.0 - 1.0;
}
/// 2D 梯度噪声:以值噪声的成本得到接近 Perlin 的等向团块
float   noise2(vec2 p)
{
    vec2    i   =   floor(p);
    vec2    f   =   fract(p);
    /// quintic fade
    vec2    u   =   f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float   a   =   dot(gradient(i), f);
    float   b   =   dot(gradient(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float   c   =   dot(gradient(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float   d   =   dot(gradient(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));
    /// 约 [-1,1]
    return  mix(mix(a, b, u.x), mix(c, d, u.x), u.y) * 1.6;
}
/// 4 阶 FBM;逐阶叠加 drift,使云翻涌而不是像印章一样整体平移
float   fbm(vec2 p,float drift)
{
    float   result      =   0.0;
    float   amplitude   =   1.0;
    for (int i = 0; i < 4; ++i)
    {
        result      +=  amplitude * noise2(p);
        amplitude   *=  0.5;
        p            =  p * 2.0 + drift;
    }
    return  result;
}

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
    float   turbidity   =   _sky._params.x;
    float   rayleigh    =   _sky._params.y;
    float   mieCoef     =   _sky._params.z;
    float   mieG        =   _sky._params.w;
    float   luminance   =   _sky._sunDirection.w;
    vec3    up          =   normalize(_sky._up.xyz);
    vec3    sunDir      =   normalize(_sky._sunDirection.xyz);

    vec3    direction   =   normalize(inDir - cameraPos);

    /// 视线天顶角与光学长度
    float   zenithAngle =   acos(max(0.0, dot(up, direction)));
    float   inv         =   1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - (zenithAngle * 180.0 / PI), -1.253));
    float   sR          =   rayleighZenithLength * inv;
    float   sM          =   mieZenithLength * inv;

    /// 太阳强度与日落渐隐(three.js 的 sunPosition.y 即 up 轴分量,本引擎 up 可任意)
    float   sunE        =   sunIntensity(dot(sunDir, up));
    float   sunfade     =   1.0 - clamp(1.0 - exp(dot(sunDir, up) / 450000.0), 0.0, 1.0);

    /// 散射系数(米氏总量随浊度变化)
    vec3    totalMie    =   0.434 * ((0.2 * turbidity) * 10e-18) * MieConst;
    vec3    betaR       =   totalRayleigh * (rayleigh - (1.0 - sunfade));
    vec3    betaM       =   totalMie * mieCoef;

    /// 消光(吸收 + 外散射)
    vec3    Fex         =   exp(-(betaR * sR + betaM * sM));

    /// 内散射
    float   cosTheta    =   dot(direction, sunDir);
    float   rPhase      =   rayleighPhase(cosTheta * 0.5 + 0.5);
    vec3    betaRTheta  =   betaR * rPhase;
    float   mPhase      =   hgPhase(cosTheta, mieG);
    vec3    betaMTheta  =   betaM * mPhase;

    vec3    Lin         =   pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * (1.0 - Fex), vec3(1.5));
    Lin *=  mix(vec3(1.0),
                pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * Fex, vec3(0.5)),
                clamp(pow(1.0 - dot(up, sunDir), 5.0), 0.0, 1.0));

    /// 夜空底色(无太阳直射时的环境散射)
    vec3    L0          =   vec3(0.1) * Fex;
    /// 太阳圆盘:与 three.js Sky 一致,cosTheta 超过日面角直径余弦即为日面
    float   sundisc     =   clamp((cosTheta - sunAngularDiameterCos) * 50000.0, 0.0, 1.0);
    vec3    sundiscColor=   (760.0 * sundisc) * min(sunE * Fex, vec3(80.0));

    vec3    texColor    =   (Lin + L0) * 0.04 + sundiscColor + vec3(0.0, 0.0003, 0.00075);

    /// ---- 云层(移植 three.js SkyMesh,合成进天空自身的 HDR 辐亮度 texColor)----
    /// 参数:_cloud=(coverage, density, elevation, _)  _anim=(时间秒, cloudScale, cloudSpeed, _)
    /// 注意 three.js 是 Y 轴朝上,本引擎 Z 轴朝上,故把 direction.xz/direction.y
    /// 的平面投影改写为以 up 为法线构造切向基,几何含义完全等价
    float   cloudCov    =   _sky._cloud.x;
    float   cloudDens   =   _sky._cloud.y;
    float   cloudElev   =   _sky._cloud.z;
    float   cloudTime   =   _sky._anim.x;
    float   cloudScale  =   _sky._anim.y;
    float   cloudSpeed  =   _sky._anim.z;
    float   dirUp       =   dot(direction, up);
    if (dirUp > 0.0 && cloudCov > 0.0)
    {
        vec3    helper      =   abs(up.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3    tanX        =   normalize(cross(helper, up));
        vec3    tanY        =   cross(up, tanX);
        /// 投影到云平面(elevation 越大云看起来越低/越近)
        float   elev        =   mix(1.0, 0.1, cloudElev);
        vec2    cloudUV     =   vec2(dot(direction, tanX), dot(direction, tanY)) / (dirUp * elev);
        cloudUV            *=  cloudScale;
        cloudUV            +=  cloudTime * cloudSpeed;

        /// 云密度场(严格移植 three.js Sky.js,逐行等价)
        float   evolve      =   cloudTime * cloudSpeed * 300.0;
        float   cloudNoise  =   clamp(fbm(cloudUV * 1000.0, evolve) * 0.7 + 0.5, 0.0, 1.0);
        /// 大尺度覆盖变化
        float   region      =   noise2(cloudUV * 300.0) * 0.37 + 0.5;
        float   cov         =   clamp(cloudCov + (region - 0.5) * 0.6, 0.0, 1.0);
        /// 阈值
        float   threshold   =   1.0 - cov;
        /// smoothstep 过渡带 0.3(与参考一致)
        float   cloudMask   =   smoothstep(threshold, threshold + 0.3, cloudNoise);
        /// 近地平线淡出
        float   horizonFade =   smoothstep(0.0, 0.03 + 0.06 * cloudElev, dirUp);
        cloudMask          *=  horizonFade;

        /// 云受光
        float   dayFactor   =   smoothstep(-0.08, 0.3, dot(sunDir, up));
        vec3    sunColor    =   sunE * Fex * 0.22 * 0.04;
        vec3    skyAmbient  =   Lin * 0.04 + vec3(0.0, 0.0003, 0.00075);
        /// Beer-powder 自阴影
        float   depthC      =   max(0.0, cloudNoise - threshold);
        float   beer        =   exp(depthC * -4.0);
        float   powder      =   1.0 - beer * beer;
        float   shade       =   mix(0.45, 1.0, clamp(beer * powder * 2.6, 0.0, 1.0));
        /// Henyey-Greenstein 银边
        float   silver      =   clamp(0.51 / pow(1.49 - cosTheta * 1.4, 1.5), 0.0, 3.0);
        float   edge        =   cloudMask * (1.0 - cloudMask) * 4.0;
        vec3    cloudColor  =   skyAmbient + sunColor * shade;
        cloudColor         +=  sunColor * silver * edge * 0.6;
        cloudColor         *=  max(dayFactor, 0.03);
        /// Beer 定律 alpha
        float   cloudAlpha  =   (1.0 - exp(depthC * cloudDens * -12.0)) * horizonFade;
        /// 遮挡云后的太阳圆盘光晕(three.js:只减 L0 的曝光项)
        texColor           -=  L0 * 0.04 * cloudAlpha;
        vec3    cloudAerial =   mix(texColor, cloudColor, Fex);
        texColor            =   mix(texColor, cloudAerial, cloudAlpha);
    }

    /// 曝光(UBOSky._sunDirection.w = three.js 的 toneMappingExposure,示例值 0.05)
    vec3    color       =   acesFilmicToneMapping(texColor, luminance);
    /// linear -> sRGB 编码后写入 UNORM 交换链
    outColor    =   vec4(linearToSRGB(color), 1.0);
}

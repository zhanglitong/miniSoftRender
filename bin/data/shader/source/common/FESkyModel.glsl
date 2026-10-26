#ifndef FE_SKY_MODEL_GLSL
#define FE_SKY_MODEL_GLSL 1

#include "FEShaderDefine.h"

/// <summary>
/// 程序化天空的大气散射模型(移植 three.js webgl_shaders_sky 的 Preetham 模型)。
/// 天空盒片元着色器与 PBR 的解析环境光照共用本文件:
///   - sky/shader.frag 对视线方向求值后做 ACES + sRGB 输出到交换链;
///   - pbr/shader.frag 对环境反射/法线方向求值,作为无环境纹理时的 IBL 输入。
/// skyRadiance() 返回线性 HDR 辐亮度(未做色调映射、曝光与 sRGB 编码),
/// 由调用方按各自的显示/光照链路处理。
/// </summary>

/// ---- 大气散射常量(与 three.js Sky 一致)----
const float SKY_PI      =   3.141592653589793238462643383279502884197169;
/// 空气折射率
const float SKY_N       =   1.0003;
/// 单位体积分子数(288.15K, 1013mb)
const float SKY_N_MOL   =   2.545e25;
/// 选用的三个主波长(Preetham)
const vec3  SKY_LAMBDA  =   vec3(680e-9, 550e-9, 450e-9);
/// 地球-太阳距离=1 时的预计算瑞利散射总量(lambda=680/550/450nm,与 three.js Sky 一致)
const vec3  SKY_TOTAL_RAYLEIGH = vec3(5.804542996261093e-6, 1.3562911419845635e-5, 3.0265902468824876e-5);
/// 天顶方向光学长度(分子/米氏)
const float SKY_RAYLEIGH_ZENITH_LENGTH = 8.4e3;
const float SKY_MIE_ZENITH_LENGTH      = 1.25e3;
/// 太阳角直径余弦(66 角秒)
const float SKY_SUN_ANGULAR_DIAMETER_COS = 0.999956676946448443553574619906976478926848692873900859324;
/// 太阳强度相关常量
const float SKY_CUTOFF_ANGLE = 1.6110731556870734;
const float SKY_STEEPNESS    = 1.5;
const float SKY_EE           = 1000.0;
/// 米氏散射系数常量
const vec3  SKY_MIE_CONST    = vec3(1.8399918514433978e14, 2.7798023919660528e14, 4.0790479543861094e14);

/// 瑞利相位函数
float   skyRayleighPhase(float cosTheta)
{
    return  (3.0 / (16.0 * SKY_PI)) * (1.0 + cosTheta * cosTheta);
}
/// Henyey-Greenstein 相位函数(米氏)
float   skyHgPhase(float cosTheta,float g)
{
    float   g2  =   g * g;
    float   inv =   1.0 / pow(max(0.0, 1.0 - 2.0 * g * cosTheta + g2), 1.5);
    return  (1.0 / (4.0 * SKY_PI)) * ((1.0 - g2) * inv);
}
/// 太阳强度(随天顶角余弦)
float   skySunIntensity(float zenithAngleCos)
{
    zenithAngleCos  =   clamp(zenithAngleCos, -1.0, 1.0);
    return  SKY_EE * max(0.0, 1.0 - exp(-((SKY_CUTOFF_ANGLE - acos(zenithAngleCos)) / SKY_STEEPNESS)));
}

/// <summary>
/// 太阳可见度门控:返回 PBR 环境光的压暗系数。
/// 曲线与 FESkyComponent::syncSunLight 的方向光完全一致(-2°~+8° smoothstep),
/// 太阳落到地平线附近时收敛到 nightFloor(默认约 2% 夜光)。
/// UBOSky._anim.w < 0.5 表示关闭压暗(保持旧行为)。
/// </summary>
float   skyAmbientSunGate(UBOSky sky,float nightFloor)
{
    if  (sky._anim.w < 0.5)
        return  1.0;
    vec3    sunDir  =   sky._sunDirection.xyz;
    float   sunLen  =   length(sunDir);
    if  (sunLen < 1e-6)
        return  1.0;    /// 无有效太阳方向时不压暗,避免 normalize(0)
    sunDir          =   sunDir / sunLen;
    vec3    up      =   normalize(sky._up.xyz);
    const   float   lo  =   sin(radians(-2.0));
    const   float   hi  =   sin(radians(8.0));
    float   t   =   clamp((dot(sunDir,up) - lo) / (hi - lo),0.0,1.0);
    t   =   t * t * (3.0 - 2.0 * t);
    return  mix(nightFloor,1.0,t);
}

/// ---- 云层噪声(移植 three.js Sky.js,源码逐行等价)----
/// 栅格角点上的梯度;使用无 sin 的哈希,保证各 GPU 输出一致
vec2    skyGradient(vec2 i)
{
    vec3    p   =   fract(i.xyx * vec3(0.1031, 0.1030, 0.0973));
    p          +=  dot(p, p.yzx + 33.33);
    /// 注意:必须先乘 p.zy 再取 fract(与 three.js 一致),
    /// 否则哈希退化为低频,云会糊成一大片
    return  fract((p.xx + p.yz) * p.zy) * 2.0 - 1.0;
}
/// 2D 梯度噪声:以值噪声的成本得到接近 Perlin 的等向团块
float   skyNoise2(vec2 p)
{
    vec2    i   =   floor(p);
    vec2    f   =   fract(p);
    /// quintic fade
    vec2    u   =   f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float   a   =   dot(skyGradient(i), f);
    float   b   =   dot(skyGradient(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float   c   =   dot(skyGradient(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float   d   =   dot(skyGradient(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));
    /// 约 [-1,1]
    return  mix(mix(a, b, u.x), mix(c, d, u.x), u.y) * 1.6;
}
/// 4 阶 FBM;逐阶叠加 drift,使云翻涌而不是像印章一样整体平移
float   skyFbm(vec2 p,float drift)
{
    float   result      =   0.0;
    float   amplitude   =   1.0;
    for (int i = 0; i < 4; ++i)
    {
        result      +=  amplitude * skyNoise2(p);
        amplitude   *=  0.5;
        p            =  p * 2.0 + drift;
    }
    return  result;
}

/// <summary>
/// 按世界方向求天空的线性 HDR 辐亮度(Preetham 大气散射 + 太阳圆盘 + 云)。
/// 参数全部来自 SkyUBO,与天空盒渲染路径完全一致;
/// 返回值未做曝光 / ACES / sRGB 处理。
/// </summary>
vec3    skyRadiance(vec3 direction, UBOSky sky)
{
    float   turbidity   =   sky._params.x;
    float   rayleigh    =   sky._params.y;
    float   mieCoef     =   sky._params.z;
    float   mieG        =   sky._params.w;
    vec3    up          =   normalize(sky._up.xyz);
    vec3    sunDir      =   normalize(sky._sunDirection.xyz);

    direction   =   normalize(direction);

    /// 视线天顶角与光学长度
    float   zenithAngle =   acos(max(0.0, dot(up, direction)));
    float   inv         =   1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - (zenithAngle * 180.0 / SKY_PI), -1.253));
    float   sR          =   SKY_RAYLEIGH_ZENITH_LENGTH * inv;
    float   sM          =   SKY_MIE_ZENITH_LENGTH * inv;

    /// 太阳强度与日落渐隐(three.js 的 sunPosition.y 即 up 轴分量,本引擎 up 可任意)
    float   sunE        =   skySunIntensity(dot(sunDir, up));
    float   sunfade     =   1.0 - clamp(1.0 - exp(dot(sunDir, up) / 450000.0), 0.0, 1.0);

    /// 散射系数(米氏总量随浊度变化)
    vec3    totalMie    =   0.434 * ((0.2 * turbidity) * 10e-18) * SKY_MIE_CONST;
    vec3    betaR       =   SKY_TOTAL_RAYLEIGH * (rayleigh - (1.0 - sunfade));
    vec3    betaM       =   totalMie * mieCoef;

    /// 消光(吸收 + 外散射)
    vec3    Fex         =   exp(-(betaR * sR + betaM * sM));

    /// 内散射
    float   cosTheta    =   dot(direction, sunDir);
    float   rPhase      =   skyRayleighPhase(cosTheta * 0.5 + 0.5);
    vec3    betaRTheta  =   betaR * rPhase;
    float   mPhase      =   skyHgPhase(cosTheta, mieG);
    vec3    betaMTheta  =   betaM * mPhase;

    vec3    Lin         =   pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * (1.0 - Fex), vec3(1.5));
    Lin *=  mix(vec3(1.0),
                pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * Fex, vec3(0.5)),
                clamp(pow(1.0 - dot(up, sunDir), 5.0), 0.0, 1.0));

    /// 夜空底色(无太阳直射时的环境散射)
    vec3    L0          =   vec3(0.1) * Fex;
    /// 太阳圆盘:与 three.js Sky 一致,cosTheta 超过日面角直径余弦即为日面
    float   sundisc     =   clamp((cosTheta - SKY_SUN_ANGULAR_DIAMETER_COS) * 50000.0, 0.0, 1.0);
    vec3    sundiscColor=   (760.0 * sundisc) * min(sunE * Fex, vec3(80.0));

    vec3    texColor    =   (Lin + L0) * 0.04 + sundiscColor + vec3(0.0, 0.0003, 0.00075);

    /// ---- 云层(移植 three.js SkyMesh,合成进天空自身的 HDR 辐亮度)—
    /// 参数:_cloud=(coverage, density, elevation, _)  _anim=(时间秒, cloudScale, cloudSpeed, _)
    /// 注意 three.js 是 Y 轴朝上,本引擎 Z 轴朝上,故把 direction.xz/direction.y
    /// 的平面投影改写为以 up 为法线构造切向基,几何含义完全等价
    float   cloudCov    =   sky._cloud.x;
    float   cloudDens   =   sky._cloud.y;
    float   cloudElev   =   sky._cloud.z;
    float   cloudTime   =   sky._anim.x;
    float   cloudScale  =   sky._anim.y;
    float   cloudSpeed  =   sky._anim.z;
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
        float   cloudNoise  =   clamp(skyFbm(cloudUV * 1000.0, evolve) * 0.7 + 0.5, 0.0, 1.0);
        /// 大尺度覆盖变化
        float   region      =   skyNoise2(cloudUV * 300.0) * 0.37 + 0.5;
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

    return  texColor;
}

#endif

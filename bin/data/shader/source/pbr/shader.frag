#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "FEShaderDefine.h"
/// 程序化天空辐亮度(与天空盒着色器共用),用于无环境纹理时的解析 IBL
#include "FESkyModel.glsl"

const float M_PI = 3.141592653589793;

float clampedDot(vec3 x, vec3 y)
{
    return clamp(dot(x, y), 0.0, 1.0);
}

layout(set = 0, binding = 0) uniform CameraBlock {
    CameraData _camera;
};

layout(set = 0, binding = 1) buffer LightsBlock {
    LightData _lights[];
};

/// 引擎全局 UBO(binding 3 = SB_Global,与 sky 管线共用同一份设备 UBO):
/// EngineState=选中高亮;UBOSky=天空参数/昼夜门控。
/// 未挂接环境纹理时,直接把天空当作无限远环境光源做解析 IBL
layout(set = 0, binding = SB_Global) uniform EngineGlobalBlock {
    UBOSky      _sky;
    EngineState _engineState;
};

layout(set = 0, binding = 4) uniform PBRBlock {
    PBRData _pbr;
};

/// PBR 贴图:绑定到 6~10,与 FEMaterialPBR 中 bind(0, N, ...) 一致
layout(set = 0, binding = 6)  uniform sampler2D u_BaseColorTex;
layout(set = 0, binding = 7)  uniform sampler2D u_MetallicRoughnessTex;
layout(set = 0, binding = 8)  uniform sampler2D u_NormalTex;
layout(set = 0, binding = 9)  uniform sampler2D u_OcclusionTex;
layout(set = 0, binding = 10) uniform sampler2D u_EmissiveTex;

/// PBR 贴图参数(标志位/法线缩放/AO 强度/自发光强度),绑定到 11。
/// 说明:这些参数不能放在 PBRData 的 _reserver* 中——PBRData 在 C++ 下是紧凑
/// 布局(glm::vec4 对齐为 4,整个结构 128 字节),而 GLSL 按 std140 布局会把
/// 嵌套结构体对齐到 16 字节,_reserver* 偏移落在 144 之外,读取越界恒为 0。
/// 这里使用纯标量成员,std140 偏移(0/4/8/12)与 C++ 一致,保证数据正确。
/// KHR 材质扩展参数(clearcoat/transmission/iridescence/AO UV 集)同理追加在尾部,
/// 与 FEMaterialPBR::PBRTexParams 逐字节对齐。
layout(set = 0, binding = 11) uniform PBRTexBlock {
    uint    _pbrTexFlags;
    float   _pbrNormalScale;
    float   _pbrOcclusionStrength;
    float   _pbrEmissiveStrength;
    float   _pbrTransmissionFactor;
    float   _pbrClearcoatStrength;
    float   _pbrClearcoatRoughness;
    float   _pbrIriFactor;
    float   _pbrIriIor;
    float   _pbrIriTmin;
    float   _pbrIriTmax;
    float   _pbrAoTexCoord;
    /// KHR_texture_transform:6 类贴图各一套 2x2 矩阵 + 平移。
    /// 每槽占 2 个 vec4:[m00,m01,m10,m11] + [ox,oy,-,-](槽序 0~5 =
    /// baseColor/metallicRoughness/normal/occlusion/emissive/iriThickness),
    /// 与 FEMaterialPBR::PBRTexParams::_uvTransform 逐字节一致
    vec4    _pbrUvTr[12];
    /// KHR_materials_volume(尾部追加,与 PBRTexParams::_volume 逐字节一致):
    /// [0]=(thickness, attenuationColor.rgb),[1]=(attenuationDistance,-,-,-),
    /// distance<=0 视为无穷大(不吸收)
    vec4    _pbrVolume[2];
};

/// 环境贴图(cubemap,IBL 镜面预滤波用),绑定到 12
layout(set = 0, binding = 12) uniform samplerCube u_EnvironmentMap;
/// 漫反射辐照度 cubemap(CPU 球谐余弦卷积预积分,单 mip),绑定到 13
layout(set = 0, binding = 13) uniform samplerCube u_IrradianceMap;
/// IBL split-sum BRDF 积分 LUT(R=scale,G=offset;x=NdotV,y=roughness),绑定到 15
/// (14 保留:WGSL 侧为统一 sampler 槽,保持两端 binding 编号一致)
layout(set = 0, binding = 15) uniform sampler2D u_BRDFLUT;
/// 虹彩薄膜厚度贴图(KHR_materials_iridescence iridescenceThicknessTexture,G 通道),绑定到 16
layout(set = 0, binding = 16) uniform sampler2D u_IriThicknessTex;
/// transmission 场景折射纹理(不透明场景预渲染结果,屏幕空间采样),绑定到 17
layout(set = 0, binding = 17) uniform sampler2D u_SceneTex;

/// 贴图存在标志位
#define PBR_TEX_BASECOLOR       0x00000001u
#define PBR_TEX_METALLICROUGH   0x00000002u
#define PBR_TEX_NORMAL          0x00000004u
#define PBR_TEX_OCCLUSION       0x00000008u
#define PBR_TEX_EMISSIVE        0x00000010u
#define PBR_TEX_ENVIRONMENT     0x00000020u
#define PBR_TEX_IRITHICKNESS    0x00000040u

/// KHR_texture_transform:按贴图槽位(0~5,同 C++ PBRTexSlot)变换 UV
vec2 applyUvTr(int slot, vec2 uv)
{
    vec4 m = _pbrUvTr[slot * 2];
    vec4 t = _pbrUvTr[slot * 2 + 1];
    return vec2(m.x * uv.x + m.y * uv.y + t.x,
                m.z * uv.x + m.w * uv.y + t.y);
}

/// 光类型:直接复用 FEShaderDefine.h 中的 LightTypeDir/Point/Spot
/// (与 FELight::LightType 一致,注意 LightTypeDir==0,不能自行假定为 1)
#define LT_DIR                  LightTypeDir
#define LT_POINT                LightTypePoint
#define LT_SPOT                 LightTypeSpot

layout(location = 0) in  vec3 inPos;
layout(location = 1) in  vec3 inNor;
layout(location = 2) in  vec2 inUV;
layout(location = 3) in  vec4 inColor;
layout(location = 4) flat in uint inFlagBits;
/// 第二套 UV(glTF TEXCOORD_1,目前用于 AO 专用 UV 集;
/// 网格未提供时引擎侧不绑定,Vulkan 读 0/WebGPU 由零缓冲兜底)
layout(location = 5) in  vec2 inUV1;

layout(location = 0) out vec4 fragColor;

/// GGX/Trowbridge-Reitz 法线分布
float distributionGGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (M_PI * d * d + 1e-7);
}

/// Schlick-Smith 几何遮蔽(GGX)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 1e-7);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

/// Schlick 菲涅尔近似
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/// 带粗糙度补偿的菲涅尔(IBL 用,粗糙表面菲涅尔峰值降低)
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/// ===== KHR_materials_iridescence 规范实现(Belcour 薄膜干涉,
///       对齐 glTF-Sample-Viewer iridescence.glsl)=====
float sq(float x) { return x * x; }
vec3  sq(vec3 x)  { return x * x; }

/// Schlick 菲涅尔(标量/向量两版,薄膜干涉内部使用)
float iriSchlick(float f0, float cosTheta) { return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); }
vec3  iriSchlick(vec3 f0, float cosTheta)  { return f0 + (vec3(1.0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); }

/// XYZ → sRGB(Rec709) 变换矩阵(列主序,与 iridescence.glsl 一致)
const mat3 IRI_XYZ_TO_REC709 = mat3(
     3.2404542, -0.9692660,  0.0556434,
    -1.5371385,  1.8760108, -0.2040259,
    -0.4985314,  0.0415560,  1.0572252
);

vec3 iriFresnel0ToIor(vec3 f0)
{
    vec3 sqrtF0 = sqrt(f0);
    return (vec3(1.0) + sqrtF0) / (vec3(1.0) - sqrtF0);
}
vec3 iriIorToFresnel0(vec3 transmittedIor, float incidentIor)
{
    return sq((transmittedIor - vec3(incidentIor)) / (transmittedIor + vec3(incidentIor)));
}
float iriIorToFresnel0(float transmittedIor, float incidentIor)
{
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

/// XYZ 灵敏度曲线的傅里叶求值(OPD 单位 nm)
vec3 evalSensitivity(float OPD, vec3 shift)
{
    float phase = 2.0 * M_PI * OPD * 1.0e-9;
    vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);

    vec3 xyz = val * sqrt(2.0 * M_PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
    xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
    xyz /= 1.0685e-7;

    return IRI_XYZ_TO_REC709 * xyz;
}

/// 薄膜干涉虹彩菲涅尔:返回视角相关的 RGB 反射率(替代常规 F)。
/// outsideIOR=外部(空气)IOR,eta2=薄膜 IOR,cosTheta1=NdotV,
/// thinFilmThickness 单位 nm,baseF0=基材 F0(介电 0.04/金属 baseColor)
vec3 evalIridescence(float outsideIOR, float eta2, float cosTheta1, float thinFilmThickness, vec3 baseF0)
{
    /// 厚度→0 时虹彩 IOR 退化到外界 IOR(无干涉)
    float iridescenceIor = mix(outsideIOR, eta2, smoothstep(0.0, 0.03, thinFilmThickness));
    /// Snell 定律求基层折射角
    float sinTheta2Sq = sq(outsideIOR / iridescenceIor) * (1.0 - sq(cosTheta1));
    float cosTheta2Sq = 1.0 - sinTheta2Sq;
    if (cosTheta2Sq < 0.0) return vec3(1.0);    /// 全反射
    float cosTheta2 = sqrt(cosTheta2Sq);

    /// 第一界面(空气→薄膜)
    float R0 = iriIorToFresnel0(iridescenceIor, outsideIOR);
    float R12 = iriSchlick(R0, cosTheta1);
    float T121 = 1.0 - R12;
    float phi12 = 0.0;
    if (iridescenceIor < outsideIOR) phi12 = M_PI;
    float phi21 = M_PI - phi12;

    /// 第二界面(薄膜→基材)
    vec3 baseIOR = iriFresnel0ToIor(clamp(baseF0, 0.0, 0.9999));  /// 防止 1.0 奇点
    vec3 R1 = iriIorToFresnel0(baseIOR, iridescenceIor);
    vec3 R23 = iriSchlick(R1, cosTheta2);
    vec3 phi23 = vec3(0.0);
    if (baseIOR[0] < iridescenceIor) phi23[0] = M_PI;
    if (baseIOR[1] < iridescenceIor) phi23[1] = M_PI;
    if (baseIOR[2] < iridescenceIor) phi23[2] = M_PI;

    /// 光程差与相位
    float OPD = 2.0 * iridescenceIor * thinFilmThickness * cosTheta2;
    vec3 phi = vec3(phi21) + phi23;

    /// 多光束干涉复合项
    vec3 R123 = clamp(R12 * R23, 1e-5, 0.9999);
    vec3 r123 = sqrt(R123);
    vec3 Rs = sq(T121) * R23 / (vec3(1.0) - R123);

    /// m=0 直流项
    vec3 I = R12 + Rs;

    /// m=1,2 干涉项
    vec3 Cm = Rs - T121;
    for (int m = 1; m <= 2; ++m)
    {
        Cm *= r123;
        vec3 Sm = 2.0 * evalSensitivity(float(m) * OPD, float(m) * phi);
        I += Cm * Sm;
    }

    /// 色域外负值钳制
    return max(I, vec3(0.0));
}

/// 由位置与 UV 的屏幕空间导数构造 TBN(无需顶点切线属性)
mat3 buildTBN(vec3 position, vec3 normal, vec2 uv)
{
    vec3 pos_dx = dFdx(position);
    vec3 pos_dy = dFdy(position);
    vec2 uv_dx  = dFdx(uv);
    vec2 uv_dy  = dFdy(uv);

    vec3 t = normalize(uv_dy.y * pos_dx - uv_dx.y * pos_dy);
    vec3 n = normalize(normal);
    /// 正交化
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

/// <summary>
/// 解析天空镜面(无环境纹理时的 IBL 镜面近似):
/// 以反射方向 R 为中心,在切空间内按 roughness^2 的锥角做固定 4 方向采样,
/// 近似 GGX 预滤波 cubemap 的 mip 模糊效果(无法对程序化函数做 textureLod)
/// </summary>
vec3 skySpecularRadiance(vec3 R,float roughness)
{
    vec3    up      =   normalize(_sky._up.xyz);
    float   spread  =   roughness * roughness;
    if  (spread < 1.0e-3)
        return  skyRadiance(R, _sky);
    vec3    helper  =   abs(dot(R, up)) < 0.99 ? up : vec3(1.0, 0.0, 0.0);
    vec3    tanX    =   normalize(cross(helper, R));
    vec3    tanY    =   cross(R, tanX);
    const int   N   =   4;
    vec3    acc     =   vec3(0.0);
    for (int i = 0; i < N; ++i)
    {
        float   a   =   (float(i) + 0.5) * (2.0 * M_PI / float(N));
        vec3    d   =   normalize(R + (tanX * cos(a) + tanY * sin(a)) * (spread * 0.9));
        acc        +=   skyRadiance(d, _sky);
    }
    return  acc / float(N);
}

/// <summary>
/// 解析天空漫反射(无环境纹理时的 irradiance 近似):
/// 半球余弦卷积无法解析求值,用法线方向与天顶方向的加权混合方向单次采样,
/// 得到天空在此半球上的平均辐亮度(均匀天空下等价于 E/π)
/// </summary>
vec3 skyDiffuseRadiance(vec3 n)
{
    vec3    up  =   normalize(_sky._up.xyz);
    vec3    d   =   normalize(n + up * 0.5);
    return  skyRadiance(d, _sky);
}

void main()
{
    uint texFlags = _pbrTexFlags;
    float normalScale       = _pbrNormalScale;
    float occlusionStrength = _pbrOcclusionStrength;
    float emissiveStrength  = _pbrEmissiveStrength;

    /// 1. 基础颜色
    vec4 baseColor = _pbr._diffuse;
    if ((texFlags & PBR_TEX_BASECOLOR) != 0u)
    {
        /// baseColor 贴图按 glTF 规范为 sRGB,解码到线性空间再参与光照
        vec4 bc = texture(u_BaseColorTex, applyUvTr(0, inUV));
        baseColor *= vec4(pow(bc.rgb, vec3(2.2)), bc.a);
    }
    /// 节点/实例颜色:仅当节点设置了颜色属性时才参与相乘(与引擎 hasNodeColor 约定一致)
    baseColor.rgb *= hasNodeColor(inFlagBits) ? inColor.rgb : vec3(1.0);

    /// 2. 金属度 / 粗糙度
    float metallic  = _pbr._metallic;
    float roughness = _pbr._roughness;
    if ((texFlags & PBR_TEX_METALLICROUGH) != 0u)
    {
        vec4 mrSample = texture(u_MetallicRoughnessTex, applyUvTr(1, inUV));
        metallic  *= mrSample.b;
        roughness *= mrSample.g;
    }
    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic,  0.0,  1.0);
    float alphaRoughness = roughness * roughness;

    /// 3. 法线
    vec3 n = normalize(inNor);
    if ((texFlags & PBR_TEX_NORMAL) != 0u)
    {
        /// 法线贴图的 UV 变换须同时用于 TBN 构造,保证切线方向与纹理空间一致
        vec2 uvN = applyUvTr(2, inUV);
        mat3 TBN = buildTBN(inPos, n, uvN);
        vec3 normalSample = texture(u_NormalTex, uvN).rgb * 2.0 - 1.0;
        normalSample.xy *= normalScale;
        n = normalize(TBN * normalSample);
    }

    /// 4. 视线方向(从世界位置指向相机)
    vec3 camPos = -transpose(mat3(_camera._v)) * vec3(_camera._v[3]);
    vec3 v = normalize(camPos - inPos);
    float NdotV = max(dot(n, v), 1e-4);

    /// 5. 反射率 F0:介电 0.04,金属用 baseColor
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);

    /// 5b. KHR 材质扩展参数(与 FEMaterialPBR::PBRTexParams 尾部字段对齐)
    float transmission      = clamp(_pbrTransmissionFactor,   0.0, 1.0);
    float ccStrength        = clamp(_pbrClearcoatStrength,    0.0, 1.0);
    float ccRoughness       = clamp(_pbrClearcoatRoughness,   0.04, 1.0);
    /// clearcoat 分层权重 w = ccStrength * F_cc(NdotV):清漆界面介电菲涅尔取
    /// NdotV 版,IBL 与 punctual 共用(与 glTF-Sample-Viewer pbr.frag:173 一致)
    float ccWeight          = ccStrength * (0.04 + 0.96 * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0));
    float iriFactor         = clamp(_pbrIriFactor,            0.0, 1.0);
    /// 薄膜厚度(nm):有厚度贴图时取 G 通道在 [tmin,tmax] 插值,否则取中值
    float iriThickness      = mix(_pbrIriTmin, _pbrIriTmax, 0.5);
    if ((texFlags & PBR_TEX_IRITHICKNESS) != 0u)
        iriThickness = mix(_pbrIriTmin, _pbrIriTmax, texture(u_IriThicknessTex, applyUvTr(5, inUV)).g);
    /// 虹彩菲涅尔(规范薄膜干涉,视角相关):介电 baseF0=0.04 与金属 baseF0=baseColor
    /// 两版按 metallic 混合(参考 pbr.frag:151-152),后续替换常规 F 而非混入 F0
    vec3 iriFresnel = vec3(0.0);
    if (iriFactor > 0.0)
        iriFresnel = mix(evalIridescence(1.0, _pbrIriIor, NdotV, iriThickness, vec3(0.04)),
                         evalIridescence(1.0, _pbrIriIor, NdotV, iriThickness, baseColor.rgb), metallic);

    /// 6. 逐光源直接光照(Cook-Torrance / Lambert)
    vec3 Lo = vec3(0.0);
    int lightCount = _lights.length();
    for (int i = 0; i < lightCount; ++i)
    {
        LightData L = _lights[i];
        vec3 lightColor = vec3(L.r, L.g, L.b);

        vec3 l;
        float attenuation = 1.0;
        if (L.lightType == LT_DIR)
        {
            /// 方向光:_dir 存于 xyz,约定为指向光源的方向
            l = normalize(vec3(L.x, L.y, L.z));
        }
        else if (L.lightType == LT_POINT)
        {
            vec3 toLight = vec3(L.x, L.y, L.z) - inPos;
            float dist = length(toLight);
            l = toLight / max(dist, 1e-4);
            /// 平方衰减,radius 控制影响范围
            float range = max(L.radius, 1e-4);
            attenuation = clamp(1.0 - (dist / range), 0.0, 1.0);
            attenuation = attenuation * attenuation / (dist * dist + 1.0);
        }
        else
        {
            continue;
        }

        vec3 h = normalize(v + l);
        float NdotL = max(dot(n, l), 0.0);
        float NdotH = max(dot(n, h), 0.0);
        float VdotH = max(dot(v, h), 0.0);

        float D = distributionGGX(NdotH, roughness);
        float G = geometrySmith(NdotV, NdotL, roughness);
        vec3  F = fresnelSchlick(VdotH, F0);
        /// 虹彩:以规范薄膜干涉菲涅尔替换常规 F(参考 pbr.frag:346-347,
        /// 菲涅尔权重整体被替换,漫反射补项 (1-F) 随之联动)
        if (iriFactor > 0.0)
            F = mix(F, iriFresnel, iriFactor);

        vec3 numerator   = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 1e-7;
        vec3  specular    = numerator / denominator;

        /// 金属没有漫反射;介电漫反射为 (1-F)*baseColor/pi
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        /// transmission:透射光穿过材质,漫反射按透射因子衰减
        kD *= (vec3(1.0) - transmission);
        vec3 diffuse = kD * baseColor.rgb / M_PI;

        vec3 contrib = (diffuse + specular) * lightColor * attenuation * NdotL;

        /// clearcoat:独立清漆层镜面(GGX,粗糙度独立,F0=0.04),
        /// 与基础层按 w 混合分层(能量补偿:基础层乘 1-w、清漆层乘 w,
        /// 参考 glTF-Sample-Viewer pbr.frag:363 的 mix(l_color, l_clearcoat_brdf, w))
        if (ccStrength > 0.0)
        {
            float ccD = distributionGGX(NdotH, ccRoughness);
            float ccG = geometrySmith(NdotV, NdotL, ccRoughness);
            vec3  ccF = fresnelSchlick(VdotH, vec3(0.04));
            float ccDenom = 4.0 * NdotV * NdotL + 1e-7;
            vec3  ccBRDF = (ccD * ccG * ccF / ccDenom) * lightColor * attenuation * NdotL;
            contrib = mix(contrib, ccBRDF, ccWeight);
        }
        Lo += contrib;
    }

    /// 7. IBL 环境光:
    ///    漫反射 = 球谐预卷积 irradiance cubemap(按 n 采样)
    ///    镜面   = split-sum:预滤波 cubemap × BRDF LUT(Karis 2013,与
    ///             glTF-Sample-Viewer 一致;LUT 修正掠射角能量损失与 F0 无关项)
    ///    未挂接环境纹理时(方案 C):直接把程序化天空当作无限远环境光源,
    ///    用 skyRadiance() 对反射/法线方向解析求值,并按曝光参数缩放到
    ///    与方向光一致的量级;此时没有 BRDF LUT,brdf 取 (1,0) 等价 F*1+0
    vec3 color = Lo;
    bool hasEnvTex = (texFlags & PBR_TEX_ENVIRONMENT) != 0u;
    float skyExposure = _sky._env.z;
    /// 太阳不可见后压暗环境光(解析天空与 HDR 环境贴图同规则),
    /// 门控由 CPU 在 SkyUBO 统一算好(与方向光同一曲线,夜间保留约 2% 微光)。
    /// 自发光(第 9 步)同样按此衰减,避免夜间自发光贴图仍全亮刺眼
    float sunGate = skyEnvGain(_sky);
    {
        float maxLod = hasEnvTex ? floor(log2(float(textureSize(u_EnvironmentMap, 0).x))) : 0.0;
        vec3 R = reflect(-v, n);
        vec3 irradiance;
        vec3 prefiltered;
        vec2 brdf;
        if (hasEnvTex)
        {
            irradiance  = texture(u_IrradianceMap, n).rgb;
            prefiltered = textureLod(u_EnvironmentMap, R, roughness * maxLod).rgb;
            brdf        = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;
        }
        else
        {
            irradiance  = skyDiffuseRadiance(n) * skyExposure;
            prefiltered = skySpecularRadiance(R, roughness) * skyExposure;
            brdf        = vec2(1.0, 0.0);
        }
        /// 太阳不可见后压暗环境光(解析天空与 HDR 环境贴图同规则),
        /// 曲线与方向光一致,夜间保留约 2% 微光避免全黑
        irradiance  *=  sunGate;
        prefiltered *=  sunGate;
        vec3 Fenv  = fresnelSchlickRoughness(NdotV, F0, roughness);
        /// 虹彩:IBL 菲涅尔权重同步替换(参考 pbr.frag:219-220)。
        /// 镜面权重:常规 F*brdf.x+brdf.y ↔ 裸 iriFresnel(参考实现不叠 LUT);
        /// 漫反射补项:1-Fenv ↔ 1-iriFresnel(菲涅尔被整体替换,能量守恒)
        vec3 envSpecW = Fenv * brdf.x + brdf.y;
        vec3 envF     = Fenv;
        if (iriFactor > 0.0)
        {
            envSpecW = mix(envSpecW, iriFresnel, iriFactor);
            envF     = mix(envF,     iriFresnel, iriFactor);
        }
        vec3 kDenv = (vec3(1.0) - envF) * (1.0 - metallic);
        /// transmission:IBL 漫反射同样按透射因子衰减
        kDenv *= (vec3(1.0) - transmission);
        color += kDenv * irradiance * baseColor.rgb + prefiltered * envSpecW;

        /// clearcoat IBL:清漆层镜面 = 按清漆粗糙度预滤波的环境色,
        /// 与基础层按 w = ccStrength*F_cc(NdotV) 混合(参考 pbr.frag:236)。
        /// 不再叠乘 LUT:外层混合权重已含菲涅尔,再乘 LUT 会重复计入菲涅尔
        if (ccStrength > 0.0)
        {
            vec3 ccPrefiltered = hasEnvTex
                ? textureLod(u_EnvironmentMap, R, ccRoughness * maxLod).rgb
                : skySpecularRadiance(R, ccRoughness) * skyExposure;
            ccPrefiltered  *=  sunGate;
            color = mix(color, ccPrefiltered, ccWeight);
        }
    }

    /// 7b. transmission 场景折射:透射光按因子补足被衰减的漫反射
    ///     (参考 glTF-Sample-Viewer getIBLVolumeRefraction)。
    ///     KHR_materials_volume:thickness>0 时折射光线按 IOR 折射并偏移厚度,
    ///     出口点投影采样屏幕空间 UV;再按 Beer-Lambert 吸收衰减。
    ///     thickness=0 时出口点即片元自身,退化为薄壁近似。
    ///     sceneTex 存的是已 ACES+gamma 的 LDR,这里近似还原线性后随本体再次
    ///     色调映射(二次 ACES 使玻璃后景物略暗,视觉上接近玻璃吸收,可接受)。
    if (transmission > 0.0)
    {
        /// KHR_materials_ior 未接入,固定玻璃典型值 1.5
        /// (参考实现 applyIorToRoughness(1.5)=identity,故 lod 仍按 roughness 取)
        const float VOL_IOR = 1.5;
        float volThickness  = _pbrVolume[0].x;
        vec3  volAttColor   = _pbrVolume[0].yzw;
        float volAttDist    = _pbrVolume[1].x;
        /// 折射向量(IOR 1.0→1.5),出口点 = 片元位置 + 折射向量*厚度
        /// (参考:transmissionRay = refract(-v, n, 1.0/ior) * thickness;
        ///  TIR 时 refract 返回零向量,出口点退化为片元自身)
        vec3  refractVec    = refract(-v, n, 1.0 / VOL_IOR);
        vec3  exitPos       = inPos + refractVec * volThickness;
        vec4  ndc           = _camera._vp * vec4(exitPos, 1.0);
        /// NDC→sceneTex UV:X 自洽不翻;Y 必须取反——引擎投影为 OpenGL 风格
        /// (NDC+Y 朝上),Vulkan 经 INVERT_VIEWPORT 负高视口、WebGPU 经自身
        /// NDC-Y-up 视口变换,均使 NDC+Y 落到帧缓冲顶行(y_f=0),即
        /// y_nd = 1-2*y_f/h,故 ndc.y*0.5+0.5 = 1-y_f/h,取反后才是本像素行
        vec2 ruv      = ndc.xy / ndc.w * vec2(0.5, -0.5) + 0.5;
        /// 磨砂玻璃模糊折射:按 roughness 选 mip lod 采样(对标 glTF-Sample-Viewer
        /// framebufferLod = roughness * log2(screenW);满链时 log2(screenW)≈maxLod,
        /// sceneTex 仅 Vulkan 端建满 mip 链,占位 1x1 白图 maxLod=0 退化为原行为)
        float sceneMaxLod = float(textureQueryLevels(u_SceneTex) - 1);
        vec3 sceneRGB = textureLod(u_SceneTex, ruv, roughness * sceneMaxLod).rgb;
        /// Beer-Lambert 体积吸收(参考 applyVolumeAttenuation):
        /// attenuation = pow(attenuationColor, vec3(thickness/attenuationDistance));
        /// distance<=0 视为无穷大(不吸收);thickness=0 时不进入分支
        /// (避免 pow(0,0) 未定义),且 pow(c,0)=1 本就等价不吸收
        vec3 volAtt = vec3(1.0);
        if (volAttDist > 0.0 && volThickness > 0.0)
            volAtt = pow(clamp(volAttColor, 0.0, 1.0), vec3(volThickness / volAttDist));
        /// 官方:f_specular_transmission 取代 f_diffuse 后再经 (1-F_ibl) 混合,
        /// 透射项最终权重 = transmission*(1-F)(掠射角透射减弱、反射增强)。
        /// sceneTex 存的是已 ACES+gamma 的 LDR,近似还原线性后随本体再次色调映射。
        vec3 Ft       = fresnelSchlickRoughness(NdotV, vec3(0.04), roughness);
        color += pow(sceneRGB, vec3(2.2)) * volAtt * baseColor.rgb * transmission * (vec3(1.0) - Ft);
    }

    /// 8. 环境遮蔽(AO 的 UV 集由材质参数选择:glTF occlusionTexture.texCoord)
    if ((texFlags & PBR_TEX_OCCLUSION) != 0u)
    {
        vec2 aoUV = (_pbrAoTexCoord > 0.5) ? inUV1 : inUV;
        float ao = texture(u_OcclusionTex, applyUvTr(3, aoUV)).r;
        color *= mix(vec3(1.0), vec3(ao), occlusionStrength);
    }

    /// 9. 自发光(emissive 贴图同为 sRGB,需解码);
    ///    自发光同样按 1-w 被清漆层衰减(参考 pbr.frag:386)
    ///    夜间(太阳不可见)自发光按 sunGate 一并压暗,避免贴图中心亮斑刺眼
    vec3 emissive = _pbr._emissive.rgb;
    if ((texFlags & PBR_TEX_EMISSIVE) != 0u)
        emissive *= pow(texture(u_EmissiveTex, applyUvTr(4, inUV)).rgb, vec3(2.2));
    color += emissive * emissiveStrength * (1.0 - ccWeight) * sunGate;

    /// 10. 选中高亮
    if (isSelected(inFlagBits))
    {
        vec3 sel = unpackColor(_engineState.selectColor).rgb;
        color = mix(color, sel, 0.5);
    }

    /// 11. 色调映射(ACES filmic 近似,与 glTF-Sample-Viewer 同款)+ gamma 校正
    ///     (PBR 在线性空间计算,ACES 恢复高光滚降与饱和度后再做 gamma)
    color = clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
    color = pow(color, vec3(1.0 / 2.2));

    /// 12. 透明度输出(pbrBlend 管线为"预乘 alpha 混合":final = src.rgb + dst*(1-src.a))
    ///     (a) transmission 玻璃:场景折射双趟已把不透明场景经 7b 段计入 color,
    ///         故 alpha=1 不透明写出,避免 dst 背景与 sceneTex 采样二次叠加;
    ///         掠射角反射增强由 BRDF 镜面项与 7b 的 (1-F) 权重自然呈现。
    ///     (b) 普通 alpha 材质:输出预乘色 color*alpha,在预乘混合下与标准
    ///         src*alpha+dst*(1-alpha) 等价;不透明材质 alpha=1,无变化。
    if (transmission > 0.0)
    {
        /// 场景折射 pass 已把透射背景计入 color(7b 段),玻璃按不透明写出
        /// (预乘混合 alpha=1 -> final = src.rgb),避免 dst 背景二次叠加
        fragColor = vec4(color, 1.0);
    }
    else
    {
        fragColor = vec4(color * baseColor.a, baseColor.a);
    }
}

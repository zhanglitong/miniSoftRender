#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "FEShaderDefine.h"

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

layout(set = 0, binding = 4) uniform PBRBlock {
    PBRData _pbr;
};

layout(set = 0, binding = 5) uniform EngineStateBlock {
    EngineState _engineState;
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
layout(set = 0, binding = 11) uniform PBRTexBlock {
    uint    _pbrTexFlags;
    float   _pbrNormalScale;
    float   _pbrOcclusionStrength;
    float   _pbrEmissiveStrength;
};

/// 环境贴图(cubemap,IBL 用),绑定到 12
layout(set = 0, binding = 12) uniform samplerCube u_EnvironmentMap;

/// 贴图存在标志位
#define PBR_TEX_BASECOLOR       0x00000001u
#define PBR_TEX_METALLICROUGH   0x00000002u
#define PBR_TEX_NORMAL          0x00000004u
#define PBR_TEX_OCCLUSION       0x00000008u
#define PBR_TEX_EMISSIVE        0x00000010u
#define PBR_TEX_ENVIRONMENT     0x00000020u

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
        vec4 bc = texture(u_BaseColorTex, inUV);
        baseColor *= vec4(pow(bc.rgb, vec3(2.2)), bc.a);
    }
    /// 节点/实例颜色:仅当节点设置了颜色属性时才参与相乘(与引擎 hasNodeColor 约定一致)
    baseColor.rgb *= hasNodeColor(inFlagBits) ? inColor.rgb : vec3(1.0);

    /// 2. 金属度 / 粗糙度
    float metallic  = _pbr._metallic;
    float roughness = _pbr._roughness;
    if ((texFlags & PBR_TEX_METALLICROUGH) != 0u)
    {
        vec4 mrSample = texture(u_MetallicRoughnessTex, inUV);
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
        mat3 TBN = buildTBN(inPos, n, inUV);
        vec3 normalSample = texture(u_NormalTex, inUV).rgb * 2.0 - 1.0;
        normalSample.xy *= normalScale;
        n = normalize(TBN * normalSample);
    }

    /// 4. 视线方向(从世界位置指向相机)
    vec3 camPos = -transpose(mat3(_camera._v)) * vec3(_camera._v[3]);
    vec3 v = normalize(camPos - inPos);
    float NdotV = max(dot(n, v), 1e-4);

    /// 5. 反射率 F0:介电 0.04,金属用 baseColor
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);

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

        vec3 numerator   = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 1e-7;
        vec3  specular    = numerator / denominator;

        /// 金属没有漫反射;介电漫反射为 (1-F)*baseColor/pi
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 diffuse = kD * baseColor.rgb / M_PI;

        Lo += (diffuse + specular) * lightColor * attenuation * NdotL;
    }

    /// 7. IBL 环境光(cubemap:最低 mip 近似 irradiance 漫反射,
    ///    按 roughness 选择 mip 采样反射向量作预滤波镜面)
    vec3 color = Lo;
    if ((texFlags & PBR_TEX_ENVIRONMENT) != 0u)
    {
        float maxLod = floor(log2(float(textureSize(u_EnvironmentMap, 0).x)));
        vec3 R = reflect(-v, n);
        vec3 irradiance  = textureLod(u_EnvironmentMap, n, maxLod).rgb;
        vec3 prefiltered = textureLod(u_EnvironmentMap, R, roughness * maxLod).rgb;
        vec3 Fenv  = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kDenv = (vec3(1.0) - Fenv) * (1.0 - metallic);
        color += kDenv * irradiance * baseColor.rgb + prefiltered * Fenv;
    }

    /// 8. 环境遮蔽
    if ((texFlags & PBR_TEX_OCCLUSION) != 0u)
    {
        float ao = texture(u_OcclusionTex, inUV).r;
        color *= mix(vec3(1.0), vec3(ao), occlusionStrength);
    }

    /// 9. 自发光(emissive 贴图同为 sRGB,需解码)
    vec3 emissive = _pbr._emissive.rgb;
    if ((texFlags & PBR_TEX_EMISSIVE) != 0u)
        emissive *= pow(texture(u_EmissiveTex, inUV).rgb, vec3(2.2));
    color += emissive * emissiveStrength;

    /// 10. 选中高亮
    if (isSelected(inFlagBits))
    {
        vec3 sel = unpackColor(_engineState.selectColor).rgb;
        color = mix(color, sel, 0.5);
    }

    /// 11. 色调映射(Reinhard)+ gamma 校正(PBR 在线性空间计算,输出需 gamma)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, baseColor.a);
}

#ifndef FE_SHADER_DEFINE_HPP
#define FE_SHADER_DEFINE_HPP 1

#ifdef __cplusplus
#include    "../FEFlags.hpp"
#include    "../FEStringHelper.hpp"
#include    "../FEMath.hpp"
using   namespace   FE;

#else
#define     float2  vec2
#define     float3  vec3
#define     float4  vec4

#define     int2    ivec2
#define     int3    ivec3
#define     int4    ivec4

#define     uint2   uvec2
#define     uint3   uvec3
#define     uint4   uvec4
#define     quatf   vec4
#define     matrix4 mat4

#endif

#ifdef __cplusplus
    enum    SystemBinding
    {
        SB_Camera       =   0,
        SB_Light        =   1,
        SB_Clip         =   2,
        SB_Sky          =   3,
        SB_Material     =   4,
    };
#else
    #define  SB_Camera      0
    #define  SB_Light       1
    #define  SB_Clip        2
    #define  SB_Sky         3
    #define  SB_Material    4
#endif

/// <summary>
/// 骞宠鍏?
/// </summary>
const   int     LightTypeDir    =   0;
/// <summary>
/// 鐐瑰厜婧?
/// </summary>
const   int     LightTypePoint  =   1;
/// <summary>
/// 鑱氬厜鐏?
/// </summary>
const   int     LightTypeSpot   =   2;

#ifdef __cplusplus
    enum    RenderFlag
    {
        RF_RECEIVE_SHADOW   =   1,
        RF_CAST_SHADOW      =   (RF_RECEIVE_SHADOW) <<1,
        RF_RECEIVE_LIGHT    =   (RF_CAST_SHADOW)    <<1,
        RF_COLOR            =   (RF_RECEIVE_LIGHT)  <<1,
    };
#else
    const   uint    RF_RECEIVE_SHADOW   =   1;
    const   uint    RF_CAST_SHADOW      =   2;
    const   uint    RF_RECEIVE_LIGHT    =   4;
    const   uint    RF_COLOR            =   8;
#endif


struct  LightData
{
    int     lightType;
    float   r;
    float   g;
    float   b;
    float   x;
    float   y;
    float   z;
    float   radius;
};

/// <summary>
/// KHR_materials_iridescence 扩展的作用是在 glTF 材质中模拟真实的虹彩（薄膜干涉）效应，也就是我们常说的彩虹色。
/// 这种效果出现在肥皂泡、油膜、某些昆虫翅膀或特殊涂层表面，能显著增强模型的真实感和视觉吸引力。
/// 自然现象：肥皂泡、油膜、甲虫或蝴蝶翅膀。
/// 人造物体：特殊车漆、陶瓷釉面、手机背壳等装饰性涂层。
/// 可选纹理，使用其 G 通道在最小与最大厚度之间进行插值，实现表面厚度的不均匀变化，从而产生更复杂的虹彩图案
/// uniform sampler2D u_IridescenceTexture;             强度贴图 (R通道)
/// uniform sampler2D u_IridescenceThicknessTexture;    厚度贴图 (G通道)
/// </summary>
struct  KHRMatIridescence
{
    /// <summary>
    /// 控制虹彩效果的强度，范围 0.0 (无效果) 到 1.0 (完全效果)。可使用 R 通道的纹理贴图控制。
    /// </summary>
    float   _factor;
    /// <summary>
    /// 薄膜材质的折射率 (IOR)，典型范围 1.0 - 2.0，影响色彩变化的幅度。
    /// </summary>
    float   _ior;
    /// <summary>
    /// 薄膜的最小厚度（单位：纳米 nm），用于定义厚度范围的下限。
    /// </summary>
    float   _thicknessMin;
    /// <summary>
    /// 薄膜的最大厚度（单位：纳米 nm），用于定义厚度范围的上限。若未提供厚度纹理，则使用此固定值。
    /// </summary>
    float   _thicknessMax;
#ifdef __cplusplus
    KHRMatIridescence()
    {
        _factor             =   1.0f;
        _ior                =   1.5f;
        _thicknessMin       =   100.0f;
        _thicknessMax       =   400.0f;
    }
#endif
};

/// <summary>
/// 清漆层
/// 清漆强度	strength = factor × texture.r
/// 清漆粗糙度	roughness = factor × texture.g
/// 叠加方式	final = base + clearcoat
/// 最终颜色 = 基础材质颜色 + 清漆层高光
/// </summary>
struct  KHRMatClearcoat
{
    float   _strength;
    float   _roughness;
#ifdef __cplusplus
    KHRMatClearcoat()
    {
        _strength   =   0;
        _roughness  =   0;
    }
#endif
};

/// <summary>
/// KHR_materials_transmission
/// 核心作用是让光线能够穿透物体表面，从而模拟出玻璃、水、塑料等半透明材质的效果
/// 它的关键作用在于，它实现的是基于物理的“透射”，而不仅仅是简单的“透明”。
/// 简单透明（Alpha 混合）只是让物体变淡，而透射则模拟了光线真正穿过物体的物理过程
/// 物理透射	模拟光线穿过材质时的真实行为，而非简单的透明度叠加
/// 保留高光	即使物体是透明的，表面依然能呈现真实、明亮的高光反射
/// 粗糙度影响	材质的粗糙度参数会影响透射的清晰度：
/// - 光滑玻璃 (粗糙度≈0) → 透射图像清晰
/// - 磨砂玻璃 (粗糙度>0) → 透射图像变得模糊
/// 薄壁模式	默认情况下，它将物体视为无限薄的表面。这对于窗户玻璃、灯泡外壳这类本身很薄的物体来说，效果非常理想。
/// </summary>
struct  KHRMatTransmission
{
    float   _factor;
#ifdef __cplusplus
    KHRMatTransmission()
    {
        _factor     =   0;
    }
#endif
};
/// <summary>
/// KHR_materials_transmission
/// </summary>
struct  KHRMatVolume
{
    /// <summary>
    ///  吸收颜色
    /// </summary>
    float   _colorR;
    float   _colorG;
    float   _colorB;
    /// <summary>
    /// 厚度
    /// </summary>
    float   _thickness;
    /// <summary>
    /// 吸收距离
    /// </summary>
    float   _distance;
#ifdef __cplusplus
    KHRMatVolume()
    {
        _colorR             =   0;
        _colorG             =   0;
        _colorB             =   0;
        _thickness          =   0;
        _distance           =   0;
    }
#endif
};
struct  PBRData
{
    float4              _emissive;
    float4              _diffuse;
    float4              _spacular;    
    float               _roughness;
    float               _metallic;
    KHRMatIridescence   _iri;
    KHRMatClearcoat     _clearcoat;
    KHRMatTransmission  _transmission;
    KHRMatVolume        _volume;
#ifdef __cplusplus
    PBRData()
    {
        _roughness  =   0.5f;
        _metallic   =   0.5f;
        _emissive   =   float4(1,1,1,1);
        _diffuse    =   float4(1,1,1,1);
        _spacular   =   float4(1,1,1,1);
    }
#endif
};
struct  CameraData
{
    mat4        _v;
    mat4        _p;
    mat4        _vp;
    mat4        _offsetVp;
    float4      _position;
    int4        _offset;
    float4      _upDir;
    float4      _rightDir;
};

struct  ClipData
{
    float3  pos;
    float3  color;
};
struct  UBOSky
{
    float _dummy;
};

struct  PointData
{
    /// point range,min value~ max value
    uint        _point;
    /// 颜色
    uint        _color;
#ifdef __cplusplus
    PointData()
    {
        _color  =   0;
        _color  |=  (static_cast<uint32_t>((0xFF))  << 24);  // R 在高8位
        _color  |=  (static_cast<uint32_t>((0x00))  << 16);  // G
        _color  |=  (static_cast<uint32_t>((0x00))  << 8 );  // B
        _color  |=  (static_cast<uint32_t>((0xFF))  << 0 );  // A 在低8位
       
        _point  =   0;
        _point  |=  (static_cast<uint32_t>((1))     << 24);  // 是否覆盖顶点颜色
        _point  |=  (static_cast<uint32_t>((8))     << 16);  // 点的min 8
        _point  |=  (static_cast<uint32_t>((16))    << 8 );  // 点的max 16
        _point  |=  (static_cast<uint32_t>((0xFF))  << 0 );  // 非0,表示点
    }
#endif
};


#endif

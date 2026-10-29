

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

    /// <summary>
    /// 由平移/缩放/旋转(四元数)构造 local 矩阵(真实世界坐标)
    /// 9 个四元数乘积各只计算一次并打包成 vec3 运算,
    /// 避免对称位置(m01/m10 等)重复展开
    /// </summary>
    mat4    makeTransform(in vec3 t, in vec3 s, in vec4 q)
    {
        vec3    pp  =   2.0 * q.xyz * q.yzx;        /// 2xy, 2yz, 2zx
        vec3    pw  =   2.0 * q.w   * q.zyx;        /// 2wz, 2wy, 2wx
        vec3    dd  =   q.xyz * q.xyz;              /// xx, yy, zz
        vec3    sum =   pp + pw.xzy;                /// 2(xy+wz), 2(yz+wx), 2(xz+wy)
        vec3    dif =   pp - pw.xzy;                /// 2(xy-wz), 2(yz-wx), 2(xz-wy)
        vec3    dg  =   1.0 - 2.0 * (dd.yzx + dd.zxy);
        return mat4(
        vec4(vec3(dg.x,  sum.x, dif.z) * s.x, 0),
        vec4(vec3(dif.x, dg.y,  sum.y) * s.y, 0),
        vec4(vec3(sum.z, dif.y, dg.z)  * s.z, 0),
        vec4(t, 1));
    }

    /// <summary>
    /// 大坐标处理:用 (intPart - cameraOffset) 抵消相机整数偏移,
    /// 平移部分只保留小数量级数值,减少大坐标下的浮点误差(带偏移 local 矩阵)
    /// </summary>
    mat4    makeOffsetLocalMatrix(in vec3 t, in vec3 s, in vec4 q, ivec3 intPart, ivec3 cameraOffset)
    {
        return makeTransform(t + vec3(intPart - cameraOffset), s, q);
    }
#endif

#ifdef __cplusplus
    enum    SystemBinding
    {
        SB_Camera       =   0,
        SB_Light        =   1,
        SB_Clip         =   2,
        /// 引擎全局 UBO:EngineState + 天空环境(所有非 UI 材质共用)
        SB_Global       =   3,
        SB_Material     =   4,
    };
#else
    #define  SB_Camera      0
    #define  SB_Light       1
    #define  SB_Clip        2
    #define  SB_Global      3
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

#define CULL_GROUP_X    256

/// <summary>
/// RenderFlag 必须保证 C++ enum 和 GLSL const 的 bit 完全一致,
/// 每个 flag 独占 1 bit, 从 1<<0 依次排下去, 避免链式 <<1 导致的位碰撞
/// </summary>
#ifdef __cplusplus
    enum    RenderFlag
    {
        RF_VISIBLE          =   (1<<0),
        RF_RECV_SHADOW      =   (1<<1),
        RF_CAST_SHADOW      =   (1<<2),
        RF_RECV_LIGHTING    =   (1<<3),
        RF_EFFECT_AO        =   (1<<4),
        RF_EFFECT_BLOOM     =   (1<<5),
        RF_COLOR            =   (1<<6),
        RF_SELECTED         =   (1<<7),
    };
#else
    const   uint    RF_VISIBLE          =   (1<<0);
    const   uint    RF_RECV_SHADOW      =   (1<<1);
    const   uint    RF_CAST_SHADOW      =   (1<<2);
    const   uint    RF_RECV_LIGHTING    =   (1<<3);
    const   uint    RF_EFFECT_AO        =   (1<<4);
    const   uint    RF_EFFECT_BLOOM     =   (1<<5);
    const   uint    RF_COLOR            =   (1<<6);
    const   uint    RF_SELECTED         =   (1<<7);

    bool    hasNodeColor(uint flag)
    {
        return  (flag & RF_COLOR)  != 0;
    }
    /// <summary>
    /// 是否被选中(需要高亮)
    /// </summary>
    bool    isSelected(uint flag)
    {
        return  (flag & RF_SELECTED)  != 0;
    }
    /// <summary>
    /// 解包 RGBA8 颜色(高字节在 R),与 C++ 端 selectColor/_color 打包一致
    /// </summary>
    vec4    unpackColor(uint c)
    {
        return vec4(
        float((c >> 24) & 0xFFu) / 255.0,
        float((c >> 16) & 0xFFu) / 255.0,
        float((c >> 8 ) & 0xFFu) / 255.0,
        float((c >> 0 ) & 0xFFu) / 255.0);
    }
    /// <summary>
    /// 打包 RGBA8 颜色(R 存高字节),与 unpackColor 互逆;超出 [0,1] 的分量会被截断
    /// </summary>
    uint    packColor(vec4 c)
    {
        uvec4   v = uvec4(clamp(c, 0.0, 1.0) * 255.0 + 0.5);
        return (v.r << 24) | (v.g << 16) | (v.b << 8) | (v.a << 0);
    }
    /// <summary>
    /// 打包 HDR 颜色到 2 个 uint(4 通道 half),支持 >1.0 的 HDR 值,与 unpackColorHDR 互逆
    /// </summary>
    uint2   packColorHDR(vec4 c)
    {
        return uint2(packHalf2x16(c.rg), packHalf2x16(c.ba));
    }
    /// <summary>
    /// 解包 HDR 颜色(packColorHDR 的逆操作)
    /// </summary>
    vec4    unpackColorHDR(uint2 c)
    {
        return vec4(unpackHalf2x16(c.x), unpackHalf2x16(c.y));
    }
#endif

/// <summary>
/// 通用结构，用来存储绘制的状态信息
/// </summary>
struct  EngineState
{
    /// <summary>
    /// 选中模型后的绘制颜色(打包 RGBA8)
    /// </summary>
    uint    selectColor;
#ifdef __cplusplus
    EngineState()
    {
        /// 默认高亮橙色 R=0xFF,G=0x80,B=0x00,A=0xFF
        selectColor =   0;
        selectColor |=  (static_cast<uint32_t>((0xFF))  << 24);
        selectColor |=  (static_cast<uint32_t>((0x80))  << 16);
        selectColor |=  (static_cast<uint32_t>((0x00))  << 8 );
        selectColor |=  (static_cast<uint32_t>((0xFF))  << 0 );
    }
#endif
};
struct  CullParam
{
    float4      _planes[6];
    float       _rightX;
    float       _rightY;
    float       _rightZ;
    /// <summary>
    ///  视口的宽度
    /// </summary>
    float       _viewSizeW;
    /// <summary>
    /// 视口的高度
    /// </summary>
    float       _viewSizeH;
    /// <summary>
    /// 最小可见像素
    /// </summary>
    float       _minVisiblePixels;
    /// <summary>
    /// 在总数组上索引偏移量
    /// </summary>
    uint        _offset;
    /// <summary>
    /// 元素个数
    /// </summary>
    uint        _count;
    mat4        _mvp;
};

struct  CullResult
{
    /// <summary>
    /// 输出个数,在计算前,确保原始数据是0
    /// </summary>
    uint    clippedCount;
    /// <summary>
    /// 保留,对齐
    /// </summary>
    uint    reserver0;
    uint    reserver1;
    uint    reserver2;
};

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
    KHRMatTransmission  _transmission;
    KHRMatIridescence   _iri;
    KHRMatClearcoat     _clearcoat;
    KHRMatVolume        _volume;
    uint                _reserver0;
    uint                _reserver1;
    uint                _reserver2;
    uint                _reserver3;
    uint                _reserver4;
    uint                _reserver5;
#ifdef __cplusplus
    PBRData()
    {
        _roughness  =   0.5f;
        _metallic   =   0.5f;
        _emissive   =   float4(0.1f,0.1f,0.1f,1.0f);
        _diffuse    =   float4(1.0f,0.0f,0.0f,1.0f);
        _spacular   =   float4(0.1f,0.1f,0.1f,1.0f);
        _reserver0  =   0;
        _reserver1  =   0;
        _reserver2  =   0;
        _reserver3  =   0;
        _reserver4  =   0;
        _reserver5  =   0;
    }
#endif
};
struct  CameraData
{
    mat4        _v;
    mat4        _p;
    mat4        _vp;
    mat4        _offsetVp;
    /// <summary>
    /// 相机位置小数部分
    /// </summary>
    float4      _decimal;
    /// <summary>
    /// 声明一个名为 _offset 的 4 字节整型变量
    /// </summary>
    int4        _offset;
    float4      _upDir;
    float4      _rightDir;
    
};

struct  ClipData
{
    float3  pos;
    float3  color;
};
/// <summary>
/// 天空盒(大气散射)参数 UBO:std140 布局,C++ 与 GLSL/WGSL 两侧结构体完全一致。
/// 全部字段用 vec4 承载,规避 vec3 在 std140 下 16 字节对齐产生的隐式填充歧义。
///   _sunDirection : xyz=归一化太阳方向, w=曝光(exposure,默认 0.05,对齐 three.js 示例)
///   _params       : x=turbidity, y=rayleigh, z=mieCoefficient, w=mieDirectionalG
///   _up           : xyz=世界上方向(引擎默认 Z 朝上,此处默认 (0,0,1)), w=保留
///   _cloud        : x=coverage 云量(0=无云), y=density 云密度, z=elevation 云层高度, w=保留
///   _anim         : x=累计时间(秒,驱动云流动), y=cloudScale 云层缩放, z=cloudSpeed 云速, w=保留
/// </summary>
struct  UBOSky
{
    float4  _sunDirection;
    float4  _params;
    float4  _up;
    float4  _cloud;
    /// 动画/全局开关: x=云时间(秒) y=cloudScale z=cloudSpeed
    /// w=太阳不可见时是否压暗环境光(1=开启,0=关闭)
    float4  _anim;
    /// 环境光照全局状态(所有材质共用,由 FESkyComponent 每帧在 CPU 侧算好):
    /// x=环境光门控(线性 0.02~1,与主方向光同一曲线)
    /// y=夜间压暗开关(1=开启,0=关闭)
    /// z=天空曝光(与 _sunDirection.w 同值,供解析天空 IBL 使用)
    /// w=天空盒开关(1=绘制天空并作为解析环境光,0=关闭绘制且解析天空 IBL 不计入)
    float4  _env;
#ifdef __cplusplus
    UBOSky()
    {
        _sunDirection   =   float4(0.0f,  0.0f,  1.0f,   0.05f);
        _params         =   float4(10.0f, 3.0f,  0.005f, 0.8f);
        _up             =   float4(0.0f,  0.0f,  1.0f,   0.0f);
        /// 云参数默认值:类默认是 0.4/0.4/0.5,此处取 webgpu_sky 示例的面板值,
        /// 视觉与参考截图一致
        _cloud          =   float4(0.36f, 0.71f, 0.44f,  0.0f);
        _anim           =   float4(0.0f,  0.0002f, 0.00002f, 0.0f);
        /// w=1:未挂天空组件的场景仍按解析天空光照,保持原有行为
        _env            =   float4(1.0f,  1.0f,  0.05f,  1.0f);
    }
#endif
};

#ifndef __cplusplus
/// <summary>
/// 环境光门控(线性):1=白天,约 0.02=夜间;关闭夜间压暗时恒为 1。
/// 所有材质统一读 SkyUBO 的 CPU 计算结果,避免各处重复实现衰减曲线
/// </summary>
float   skyEnvGain(UBOSky sky)
{
    return  sky._env.x;
}
/// <summary>
/// LDR 直出材质专用:把线性门控换算到显示空间(pow(1/2.2)),
/// 使其夜间亮度与 PBR(线性 HDR + ACES)观感一致;白天门控为 1 时无变化
/// </summary>
float   skyEnvGainDisplay(UBOSky sky)
{
    return  pow(clamp(sky._env.x,0.0,1.0),1.0 / 2.2);
}
/// <summary>
/// 天空盒开关:1=绘制天空并作为解析环境光,0=关闭绘制且解析天空 IBL 不计入。
/// 太阳圆盘已由主方向光的 GGX 高光表达,解析天空 IBL 只采样"无日面"版本
/// </summary>
float   skyEnabled(UBOSky sky)
{
    return  sky._env.w;
}
#endif

/// <summary>
/// 引擎全局 UBO(绑定槽 SB_Global):所有非 UI 材质共用的引擎级数据。
/// _sky 放在最前(偏移 0),只读天空的着色器可以直接声明 96 字节的 SkyBlock 子集;
/// 需要选中高亮的着色器声明完整布局:
///   _sky         : 96 字节
///   _engineState : 4 字节
///   _pad0~2      : 12 字节,补齐到 16 字节的整数倍(std140/WGSL uniform 对齐)
/// </summary>
struct  FEGlobalUBO
{
    UBOSky      _sky;
    EngineState _engineState;
    /// 对齐填充:补齐到 112 字节
    uint        _pad0;
    uint        _pad1;
    uint        _pad2;
#ifdef __cplusplus
    FEGlobalUBO()
    {
        _pad0   =   0;
        _pad1   =   0;
        _pad2   =   0;
    }
#endif
};
#ifdef __cplusplus
/// 与 GLSL/WGSL 侧 112 字节布局保持一致
static_assert(sizeof(FEGlobalUBO) == 112, "FEGlobalUBO 必须与 std140/WGSL 布局一致");
#endif

/// <summary>
/// OIT(PPLL) 链表节点,geometry/color pass 与 C++ 宿主侧布局必须完全一致
/// color 用 2 个 uint 存 4 通道 half(packColorHDR/unpackColorHDR),支持 HDR,
/// 整个节点 16 字节,std430 下数组步长为 16,与 C++ 端 sizeof(OITNode) 一致
/// </summary>
struct  OITNode
{
    /// <summary>
    /// HDR 颜色 RG 通道(packHalf2x16)
    /// </summary>
    uint    colorRG;
    /// <summary>
    /// HDR 颜色 BA 通道(packHalf2x16)
    /// </summary>
    uint    colorBA;
    /// <summary>
    /// fragment 深度(gl_FragCoord.z)
    /// </summary>
    float   depth;
    /// <summary>
    /// 下一节点索引,0xFFFFFFFF 表示链表尾
    /// </summary>
    uint    next;
};

struct  PointData
{
    mat4        _mvp;
    /// <summary>
    /// point range,min value~ max value
    /// </summary>
    uint        _point;
    /// <summary>
    /// 颜色
    /// </summary>
    uint        _color;
#ifdef __cplusplus
    PointData()
    {
        _mvp    =   mat4(1.0f);
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

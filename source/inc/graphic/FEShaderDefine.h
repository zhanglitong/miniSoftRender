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

// <summary>
/// KHR_materials_iridescence 鎵╁睍鐨勪綔鐢ㄦ槸鍦?glTF 鏉愯川涓ā鎷熺湡瀹炵殑铏瑰僵锛堣杽鑶滃共娑夛級鏁堝簲锛屼篃灏辨槸鎴戜滑甯歌鐨勫僵铏硅壊銆?
/// 杩欑鏁堟灉鍑虹幇鍦ㄨ偉鐨傛场銆佹补鑶溿€佹煇浜涙槅铏繀鑶€鎴栫壒娈婃秱灞傝〃闈紝鑳芥樉钁楀寮烘ā鍨嬬殑鐪熷疄鎰熷拰瑙嗚鍚稿紩鍔涖€?
/// 鑷劧鐜拌薄锛氳偉鐨傛场銆佹补鑶溿€佺敳铏垨铦磋澏缈呰唨銆?
/// 浜洪€犵墿浣擄細鐗规畩杞︽紗銆侀櫠鐡烽噳闈€佹墜鏈鸿儗澹崇瓑瑁呴グ鎬ф秱灞傘€?
/// 鍙€夌汗鐞嗭紝浣跨敤鍏?G 閫氶亾鍦ㄦ渶灏忎笌鏈€澶у帤搴︿箣闂磋繘琛屾彃鍊硷紝瀹炵幇琛ㄩ潰鍘氬害鐨勪笉鍧囧寑鍙樺寲锛屼粠鑰屼骇鐢熸洿澶嶆潅鐨勮櫣褰╁浘妗?
/// uniform sampler2D u_IridescenceTexture;             寮哄害璐村浘 (R閫氶亾)
/// uniform sampler2D u_IridescenceThicknessTexture;    鍘氬害璐村浘 (G閫氶亾)
/// </summary>
struct  KHRMatIridescence
{
    /// <summary>
    /// 鎺у埗铏瑰僵鏁堟灉鐨勫己搴︼紝鑼冨洿 0.0 (鏃犳晥鏋? 鍒?1.0 (瀹屽叏鏁堟灉)銆傚彲浣跨敤 R 閫氶亾鐨勭汗鐞嗚创鍥炬帶鍒躲€?
    /// </summary>
    float   _factor;
    /// <summary>
    /// 钖勮啘鏉愯川鐨勬姌灏勭巼 (IOR)锛屽吀鍨嬭寖鍥?1.0 - 2.0锛屽奖鍝嶈壊褰╁彉鍖栫殑骞呭害銆?
    /// </summary>
    float   _ior;
    /// <summary>
    /// 钖勮啘鐨勬渶灏忓帤搴︼紙鍗曚綅锛氱撼绫?nm锛夛紝鐢ㄤ簬瀹氫箟鍘氬害鑼冨洿鐨勪笅闄愩€?
    /// </summary>
    float   _thicknessMin;
    /// <summary>
    /// 钖勮啘鐨勬渶澶у帤搴︼紙鍗曚綅锛氱撼绫?nm锛夛紝鐢ㄤ簬瀹氫箟鍘氬害鑼冨洿鐨勪笂闄愩€傝嫢鏈彁渚涘帤搴︾汗鐞嗭紝鍒欎娇鐢ㄦ鍥哄畾鍊笺€?
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
/// 娓呮紗灞?
/// 娓呮紗寮哄害	strength = factor 脳 texture.r
/// 娓呮紗绮楃硻搴?roughness = factor 脳 texture.g
/// 鍙犲姞鏂瑰紡	final = base + clearcoat
/// 鏈€缁堥鑹?= 鍩虹鏉愯川棰滆壊 + 娓呮紗灞傞珮鍏?
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
/// 鏍稿績浣滅敤鏄鍏夌嚎鑳藉绌块€忕墿浣撹〃闈紝浠庤€屾ā鎷熷嚭鐜荤拑銆佹按銆佸鏂欑瓑鍗婇€忔槑鏉愯川鐨勬晥鏋?
/// 瀹冪殑鍏抽敭浣滅敤鍦ㄤ簬锛屽畠瀹炵幇鐨勬槸鍩轰簬鐗╃悊鐨勨€滈€忓皠鈥濓紝鑰屼笉浠呬粎鏄畝鍗曠殑鈥滈€忔槑鈥濄€?
/// 绠€鍗曢€忔槑锛圓lpha 娣峰悎锛夊彧鏄鐗╀綋鍙樻贰锛岃€岄€忓皠鍒欐ā鎷熶簡鍏夌嚎鐪熸绌胯繃鐗╀綋鐨勭墿鐞嗚繃绋?
/// 鐗╃悊閫忓皠	妯℃嫙鍏夌嚎绌胯繃鏉愯川鏃剁殑鐪熷疄琛屼负锛岃€岄潪绠€鍗曠殑閫忔槑搴﹀彔鍔?
/// 淇濈暀楂樺厜	鍗充娇鐗╀綋鏄€忔槑鐨勶紝琛ㄩ潰渚濈劧鑳藉憟鐜扮湡瀹炪€佹槑浜殑楂樺厜鍙嶅皠
/// 绮楃硻搴﹀奖鍝?鏉愯川鐨勭矖绯欏害鍙傛暟浼氬奖鍝嶉€忓皠鐨勬竻鏅板害锛?
/// - 鍏夋粦鐜荤拑 (绮楃硻搴︹増0) 鈫?閫忓皠鍥惧儚娓呮櫚
/// - 纾ㄧ爞鐜荤拑 (绮楃硻搴?0) 鈫?閫忓皠鍥惧儚鍙樺緱妯＄硦
/// 钖勫妯″紡	榛樿鎯呭喌涓嬶紝瀹冨皢鐗╀綋瑙嗕负鏃犻檺钖勭殑琛ㄩ潰銆傝繖瀵逛簬绐楁埛鐜荤拑銆佺伅娉″澹宠繖绫绘湰韬緢钖勭殑鐗╀綋鏉ヨ锛屾晥鏋滈潪甯哥悊鎯炽€?
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
    ///  鍚告敹棰滆壊
    /// </summary>
    float   _colorR;
    float   _colorG;
    float   _colorB;
    /// <summary>
    /// 鍘氬害
    /// </summary>
    float   _thickness;
    /// <summary>
    /// 鍚告敹璺濈
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
    /// 棰滆壊
    uint        _color;
#ifdef __cplusplus
    PointData()
    {
        _color  =   0;
        _color  |=  (static_cast<uint32_t>((0xFF))  << 24);  // R 鍦ㄩ珮8浣?
        _color  |=  (static_cast<uint32_t>((0x00))  << 16);  // G
        _color  |=  (static_cast<uint32_t>((0x00))  << 8 );  // B
        _color  |=  (static_cast<uint32_t>((0xFF))  << 0 );  // A 鍦ㄤ綆8浣?
       
        _point  =   0;
        _point  |=  (static_cast<uint32_t>((1))     << 24);  // 鏄惁瑕嗙洊椤剁偣棰滆壊
        _point  |=  (static_cast<uint32_t>((8))     << 16);  // 鐐圭殑min 8
        _point  |=  (static_cast<uint32_t>((16))    << 8 );  // 鐐圭殑max 16
        _point  |=  (static_cast<uint32_t>((0xFF))  << 0 );  // 闈?,琛ㄧず鐐?
    }
#endif
};


#endif

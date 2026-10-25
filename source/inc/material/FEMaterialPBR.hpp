#pragma     once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPBR,"{C253DF90-EA68-44D6-9558-68D244EE0066}");
    /// <summary>
    /// PBR 贴图参数(独立 UBO,绑定 binding 11),对应 shader.frag 中的 PBRTexBlock。
    /// 必须使用纯标量成员:std140 下偏移为 0/4/8/12,与 C++ 紧凑布局一致。
    /// 早期实现把这些参数放进 PBRData._reserver*:PBRData 在 C++ 下是紧凑布局
    /// (glm::vec4 对齐为 4,结构共 128 字节),而 GLSL 的 std140 会把其中的嵌套
    /// 结构体对齐到 16 字节,_reserver* 实际落在 144 之后;而 UBO 只有 128 字节,
    /// 着色器读取越界恒为 0,导致贴图标志/强度全部失效。
    /// </summary>
    struct  PBRTexParams
    {
        /// <summary>
        /// 贴图存在标志位(见 PBRTexFlag)
        /// </summary>
        uint    _flags;
        /// <summary>
        /// 法线贴图强度
        /// </summary>
        float   _normalScale;
        /// <summary>
        /// AO 强度
        /// </summary>
        float   _occlusionStrength;
        /// <summary>
        /// 自发光强度
        /// </summary>
        float   _emissiveStrength;
        /// <summary>
        /// ===== KHR 材质扩展参数(尾部追加,前 4 字段偏移不变,兼容旧数据)=====
        /// KHR_materials_transmission 透射因子(>0 时材质走 blend 管线,shader 中漫反射衰减)
        /// </summary>
        float   _transmissionFactor;
        /// <summary>
        /// KHR_materials_clearcoat 清漆强度与粗糙度
        /// </summary>
        float   _clearcoatStrength;
        float   _clearcoatRoughness;
        /// <summary>
        /// KHR_materials_iridescence 虹彩参数(强度/IOR/薄膜厚度范围)
        /// </summary>
        float   _iriFactor;
        float   _iriIor;
        float   _iriTmin;
        float   _iriTmax;
        /// <summary>
        /// AO 采样所用 UV 集(>0.5 表示用 TEXCOORD_1,对应 glTF occlusionTexture.texCoord)
        /// </summary>
        float   _aoTexCoord;
        /// <summary>
        /// ===== KHR_texture_transform(UV 变换,尾部追加)=====
        /// 6 类贴图各一套 2x2 矩阵 + 平移,每槽 8 floats:
        /// [m00,m01,m10,m11,ox,oy,0,0],与 shader 侧 vec4 _pbrUvTr[12] 逐字节一致
        /// (std140 下 vec4 数组从偏移 48 开始、元素步长 16 字节;前 12 标量占 48 字节;
        /// 槽序 0~5 = baseColor/mr/normal/ao/emissive/iriThickness,同 PBRTexSlot)
        /// </summary>
        float   _uvTransform[6 * 8];
        /// <summary>
        /// ===== KHR_materials_volume(尾部追加;2 个 vec4,保持总尺寸为 16 字节
        /// 倍数——WGSL uniform 结构尺寸必须 16 对齐,std140 vec4 数组天然对齐)=====
        /// [0]: x=thickness 折射厚度(米,0=薄壁无偏移),yzw=attenuationColor 吸收色 RGB
        /// [1]: x=attenuationDistance 吸收距离(<=0 视为无穷大=不吸收),yzw 保留
        /// 与 shader 侧 vec4 _pbrVolume[2] 逐字节一致(偏移 240/256)
        /// </summary>
        float   _volume[2 * 4];
        PBRTexParams()
            :_flags(0)
            ,_normalScale(1.0f)
            ,_occlusionStrength(1.0f)
            ,_emissiveStrength(1.0f)
            ,_transmissionFactor(0.0f)
            ,_clearcoatStrength(0.0f)
            ,_clearcoatRoughness(0.0f)
            ,_iriFactor(0.0f)
            ,_iriIor(1.3f)
            ,_iriTmin(100.0f)
            ,_iriTmax(400.0f)
            ,_aoTexCoord(0.0f)
        {
            /// 默认单位变换(无 UV 变换)
            for (uint slot = 0; slot < 6; ++slot)
                setUvIdentity(slot);
            /// 默认无体积(薄壁、不吸收)
            for (uint i = 0; i < 8; ++i)
                _volume[i] = 0.0f;
        }
        /// <summary>
        /// 把某贴图槽位的变换重置为单位矩阵
        /// </summary>
        void    setUvIdentity(uint slot)
        {
            float*  p   =   _uvTransform + slot * 8;
            p[0] = 1.0f; p[1] = 0.0f; p[2] = 0.0f; p[3] = 1.0f;
            p[4] = 0.0f; p[5] = 0.0f; p[6] = 0.0f; p[7] = 0.0f;
        }
    };
    /// <summary>
    /// PBR(金属度-粗糙度工作流)材质,支持 glTF 规范的全套贴图:
    ///   baseColor / metallicRoughness / normal / occlusion / emissive
    /// 贴图绑定到描述符 binding 6~10(与 pbr/shader.frag 中 layout(binding=N) 一致):
    ///   6=baseColor  7=metallicRoughness  8=normal  9=occlusion  10=emissive
    /// 未显式设置的贴图槽位使用 1x1 默认纹理(不影响 baseColor/roughness/metallic
    /// 的因子取值;法线为平面法线;AO=1;自发光=0)。
    /// 贴图存在标志与法线缩放/AO 强度/自发光强度放在独立的 PBRTexParams UBO
    /// (binding 11),避免复用 PBRData._reserver* 造成的 std140 布局错位。
    /// </summary>
    class   FE_API  FEMaterialPBR:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPBR)
        /// <summary>
        /// 贴图存在标志位,与 pbr/shader.frag 中的 PBR_TEX_* 宏一致
        /// </summary>
        enum    PBRTexFlag : uint
        {
            PBR_TEX_BASECOLOR      =   0x00000001u,
            PBR_TEX_METALLICROUGH  =   0x00000002u,
            PBR_TEX_NORMAL         =   0x00000004u,
            PBR_TEX_OCCLUSION      =   0x00000008u,
            PBR_TEX_EMISSIVE       =   0x00000010u,
            PBR_TEX_ENVIRONMENT    =   0x00000020u,
            /// 虹彩薄膜厚度贴图(KHR_materials_iridescence iridescenceThicknessTexture)
            PBR_TEX_IRITHICKNESS   =   0x00000040u,
        };
        /// <summary>
        /// 贴图描述符绑定号,须与 shader 中 layout(binding=N) 严格一致
        /// (WGSL 侧:12/13 为 cubemap,采样器挪到 14,由已绑定纹理自动合成)
        /// </summary>
        enum    PBRTexBinding : uint16
        {
            BIND_BASECOLOR         =   6,
            BIND_METALLICROUGH     =   7,
            BIND_NORMAL            =   8,
            BIND_OCCLUSION         =   9,
            BIND_EMISSIVE          =   10,
            BIND_TEX_PARAMS        =   11,
            BIND_ENVIRONMENT       =   12,
            BIND_IRRADIANCE        =   13,
            /// IBL split-sum BRDF 积分 LUT(WGSL 侧 14 为统一 sampler,此槽与 GLSL 一致)
            BIND_BRDF_LUT          =   15,
            /// 虹彩薄膜厚度贴图(G 通道;WGSL 复用 14 的统一 sampler)
            BIND_IRI_THICKNESS     =   16,
            /// transmission 场景折射纹理(不透明场景预渲染结果,屏幕空间采样)
            BIND_SCENE_TEX         =   17,
        };
        /// <summary>
        /// UV 变换槽位(KHR_texture_transform per-texture),
        /// 与 PBRTexParams::_uvTransform 的 8-float 分组一一对应
        /// </summary>
        enum    PBRTexSlot : uint
        {
            SLOT_BASECOLOR      =   0,
            SLOT_METALLICROUGH  =   1,
            SLOT_NORMAL         =   2,
            SLOT_OCCLUSION      =   3,
            SLOT_EMISSIVE       =   4,
            /// 虹彩薄膜厚度贴图(iridescenceThicknessTexture)
            SLOT_IRITHICKNESS   =   5,
        };
    public:
        FEMaterialPBR(FEContext& ctx,const String& pipelinePrefix = "pbr")
            :FEMaterial(ctx)
            ,_pbr(ctx)
            ,_texParams(ctx)
            ,_pipelinePrefix(pipelinePrefix)
        {
            setup(_pipelinePrefix);
            autoAttach();

            auto    lightSBO    =   ctx.device().queryCache(FEConstUuid::LightsId);

            /// 默认强度(标志位默认 0,即无贴图)
            _texParams._value._flags    =   0;
            setNormalScale(1.0f);
            setOcclusionStrength(1.0f);
            setEmissiveStrength(1.0f);
            _texParams.update();

            /// 创建并绑定默认贴图(未设置时的安全占位,采样结果等价于"无贴图")
            _baseColorMap  =  createDefaultColor(float4(1,1,1,1));
            _mrMap         =  createDefaultColor(float4(1,1,1,1));   /// G=roughness=1,B=metallic=1 → 取因子值
            _normalMap     =  createDefaultColor(float4(0.5f,0.5f,1,1)); /// 平面法线(0,0,1)
            _occlusionMap  =  createDefaultColor(float4(1,1,1,1));   /// AO=1
            _emissiveMap   =  createDefaultColor(float4(0,0,0,1));   /// 自发光=0
            _environmentMap = createDefaultCube(float4(0,0,0,1));    /// 默认黑环境(标志位默认关)
            _irradianceMap  = _environmentMap;
            /// 默认 BRDF LUT:R=1/G=0 → F*1+0 = F,与未挂 LUT 的旧行为一致
            _brdfLUT        = createDefaultColor(float4(1,0,0,1));
            /// 默认虹彩厚度贴图:G=1(标志位默认关,仅作占位;置位时等效厚度取 tmax)
            _iriThicknessMap = createDefaultColor(float4(1,1,1,1));
            /// 默认场景折射纹理:1x1 白(占位,由 FEScene 创建场景后挂真纹理)
            _sceneTexMap     = createDefaultColor(float4(1,1,1,1));
            /// 占位白图(不透明材质专用,永不被替换;见 _sceneTexWhite 注释)
            _sceneTexWhite   = _sceneTexMap;
            bindResources();

            lightSBO->as<FENotify>()->notify()  +=  {[this](Object object)
            {
                if (object->flags().hasFlag(FLAG_UPDATE))
                {
                    bind(0,SB_Light,{object});
                }
            }};
            update();
        }

        FEMaterialPBR(const FEMaterialPBR& other)
            :FEMaterial(other)
            ,_pbr(other._ctx)
            ,_texParams(other._ctx)
            ,_pipelinePrefix(other._pipelinePrefix)
        {
            _pbr._value =   other._pbr._value;
            _pbr.update();

            _texParams._value   =   other._texParams._value;
            _texParams.update();
            bind(0,BIND_TEX_PARAMS,{_texParams._gpu.get()});

            _baseColorMap  =   other._baseColorMap;
            _mrMap         =   other._mrMap;
            _normalMap     =   other._normalMap;
            _occlusionMap  =   other._occlusionMap;
            _emissiveMap   =   other._emissiveMap;
            _environmentMap =  other._environmentMap;
            _irradianceMap  =  other._irradianceMap;
            _brdfLUT        =  other._brdfLUT;
            _iriThicknessMap = other._iriThicknessMap;
            _sceneTexMap    =   other._sceneTexMap;
            _sceneTexWhite  =   other._sceneTexWhite;
            _hasRealSceneTex=   other._hasRealSceneTex;
            _autoSceneTex   =   other._autoSceneTex;
            bindAllTextures();
            updateSceneTexCount();
            update();
        }
        virtual ~FEMaterialPBR()
        {
            if (_sceneTexCounted)
                --s_transmissionMats;
            _pbr._gpu   =   nullptr;
        }

        /// <summary>
        /// ===== 贴图设置接口 =====
        /// </summary>
        inline  void    setBaseColorMap(TexPtr tex)
        {
            _baseColorMap = (tex != nullptr) ? tex : createDefaultColor(float4(1,1,1,1));
            setTexFlag(PBR_TEX_BASECOLOR, tex != nullptr);
            bind(0,BIND_BASECOLOR,{_baseColorMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        inline  void    setMetallicRoughnessMap(TexPtr tex)
        {
            _mrMap = (tex != nullptr) ? tex : createDefaultColor(float4(1,1,1,1));
            setTexFlag(PBR_TEX_METALLICROUGH, tex != nullptr);
            bind(0,BIND_METALLICROUGH,{_mrMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        inline  void    setNormalMap(TexPtr tex)
        {
            _normalMap = (tex != nullptr) ? tex : createDefaultColor(float4(0.5f,0.5f,1,1));
            setTexFlag(PBR_TEX_NORMAL, tex != nullptr);
            bind(0,BIND_NORMAL,{_normalMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        inline  void    setOcclusionMap(TexPtr tex)
        {
            _occlusionMap = (tex != nullptr) ? tex : createDefaultColor(float4(1,1,1,1));
            setTexFlag(PBR_TEX_OCCLUSION, tex != nullptr);
            bind(0,BIND_OCCLUSION,{_occlusionMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        inline  void    setEmissiveMap(TexPtr tex)
        {
            _emissiveMap = (tex != nullptr) ? tex : createDefaultColor(float4(0,0,0,1));
            setTexFlag(PBR_TEX_EMISSIVE, tex != nullptr);
            bind(0,BIND_EMISSIVE,{_emissiveMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        /// <summary>
        /// 设置环境贴图(cubemap,用于 IBL):
        ///   env         镜面预滤波 cubemap(带 mip 链,按 roughness 采样)
        ///   irradiance  漫反射辐照度 cubemap(可选,缺省复用 env —— 行为与旧版
        ///               "最低 mip 近似 irradiance" 兼容;建议传入
        ///               FETextureHelper::createIrradianceCube 的球谐预卷积结果)
        /// </summary>
        inline  void    setEnvironmentMap(TexPtr tex,TexPtr irradiance = nullptr)
        {
            _environmentMap = (tex != nullptr) ? tex : createDefaultCube(float4(0,0,0,1));
            _irradianceMap  = (irradiance != nullptr) ? irradiance : _environmentMap;
            setTexFlag(PBR_TEX_ENVIRONMENT, tex != nullptr);
            bind(0,BIND_ENVIRONMENT,{_environmentMap.get()});
            bind(0,BIND_IRRADIANCE,{_irradianceMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        /// <summary>
        /// 单独设置漫反射辐照度 cubemap(球谐预卷积结果)
        /// </summary>
        inline  void    setIrradianceMap(TexPtr tex)
        {
            _irradianceMap  = (tex != nullptr) ? tex : _environmentMap;
            bind(0,BIND_IRRADIANCE,{_irradianceMap.get()});
            update();
        }
        /// <summary>
        /// 获取当前环境贴图
        /// </summary>
        inline  TexPtr  environmentMap() const
        {
            return  _environmentMap;
        }
        /// <summary>
        /// 获取当前漫反射辐照度贴图
        /// </summary>
        inline  TexPtr  irradianceMap() const
        {
            return  _irradianceMap;
        }
        /// <summary>
        /// 设置 IBL split-sum BRDF 积分 LUT(FETextureHelper::createBRDFLUT 生成,
        /// 全局一份共享给所有 PBR 材质即可;缺省为 (1,0) 常量纹理,行为同旧版 F*prefiltered)
        /// </summary>
        inline  void    setBRDFLUT(TexPtr tex)
        {
            _brdfLUT    = (tex != nullptr) ? tex : createDefaultColor(float4(1,0,0,1));
            bind(0,BIND_BRDF_LUT,{_brdfLUT.get()});
            update();
        }
        /// <summary>
        /// 设置虹彩薄膜厚度贴图(KHR_materials_iridescence iridescenceThicknessTexture,
        /// G 通道在 [thicknessMin,thicknessMax] 内插值;缺省 G=1 占位,标志位关闭时
        /// shader 取厚度范围中值,不采样贴图)
        /// </summary>
        inline  void    setIriThicknessMap(TexPtr tex)
        {
            _iriThicknessMap = (tex != nullptr) ? tex : createDefaultColor(float4(1,1,1,1));
            setTexFlag(PBR_TEX_IRITHICKNESS, tex != nullptr);
            bind(0,BIND_IRI_THICKNESS,{_iriThicknessMap.get()});
            _texParams.update(); update();
        }

        /// <summary>
        /// ===== 标量参数 =====
        /// </summary>
        inline  void    setNormalScale(float s)
        {
            _texParams._value._normalScale  =   s;
            _texParams.update();
        }
        inline  void    setOcclusionStrength(float s)
        {
            _texParams._value._occlusionStrength    =   s;
            _texParams.update();
        }
        inline  void    setEmissiveStrength(float s)
        {
            _texParams._value._emissiveStrength     =   s;
            _texParams.update();
        }

        /// <summary>
        /// ===== KHR 材质扩展参数(对应 shader.frag 的 PBRTexBlock 扩展字段)=====
        /// KHR_materials_clearcoat 清漆层
        /// </summary>
        inline  void    setClearcoat(float strength,float roughness)
        {
            _texParams._value._clearcoatStrength    =   strength;
            _texParams._value._clearcoatRoughness   =   roughness;
            _texParams.update();
        }
        /// <summary>
        /// KHR_materials_transmission 透射(>0 时建议配合 setAlphaBlend(true))
        /// </summary>
        inline  void    setTransmission(float factor)
        {
            _texParams._value._transmissionFactor   =   factor;
            _texParams.update();
            /// <summary>
            /// 透射因子跨 0 时切换 BIND_SCENE_TEX 绑定(真纹理↔占位白图)
            /// </summary>
            bindSceneTex();
            updateSceneTexCount();
        }
        /// <summary>
        /// 挂接 transmission 场景折射纹理(手动模式:外部渲染器接管绑定,
        /// 自动刷新随之关闭)。标准场景内无需调用——transmission>0 的材质
        /// 会在渲染时自动从 device cache(FEConstUuid::SceneTexId)自取。
        /// </summary>
        inline  void    setSceneTexture(TexPtr tex)
        {
            _autoSceneTex   =   false;
            if (tex != nullptr)
            {
                _sceneTexMap        =   tex;
                _hasRealSceneTex    =   true;
            }
            bindSceneTex();
            update();
            updateSceneTexCount();
        }
        /// <summary>
        /// 是否存在需要场景折射双趟渲染的透射材质(全局)
        /// </summary>
        static  bool    anyTransmissionMaterial()
        {
            return  s_transmissionMats > 0;
        }
        /// <summary>
        /// KHR_materials_volume 体积参数(需配合 transmission>0 才有视觉效果):
        ///   thickness > 0 时折射光线按 IOR=1.5 折射并沿折射向量偏移厚度后再投影
        ///   采样 sceneTex(对标 glTF-Sample-Viewer getIBLVolumeRefraction);
        ///   attDist > 0 时按 Beer-Lambert 吸收:att = pow(attColor, thickness/attDist),
        ///   attDist <= 0 视为无穷大(不吸收);attColor 默认应为白(不吸收)。
        /// </summary>
        inline  void    setVolume(float thickness,const float3& attColor,float attDist)
        {
            float*  v   =   _texParams._value._volume;
            v[0] = thickness;
            v[1] = attColor.x; v[2] = attColor.y; v[3] = attColor.z;
            v[4] = attDist;    v[5] = 0.0f; v[6] = 0.0f; v[7] = 0.0f;
            _texParams.update();
        }
        /// <summary>
        /// 当前体积参数自检:返回 (thickness, attenuationDistance)
        /// </summary>
        inline  float2  volume() const
        {
            return  float2(_texParams._value._volume[0],_texParams._value._volume[4]);
        }
        /// <summary>
        /// 当前挂接的场景折射纹理(自动模式下即 device cache 中那份;未挂接时为 nullptr)
        /// </summary>
        TexPtr  sceneTexture() const
        {
            return  _hasRealSceneTex ? _sceneTexMap : TexPtr(nullptr);
        }
        /// <summary>
        /// KHR_materials_iridescence 虹彩(薄膜干涉)
        /// </summary>
        inline  void    setIridescence(float factor,float ior = 1.3f,float tmin = 100.0f,float tmax = 400.0f)
        {
            _texParams._value._iriFactor    =   factor;
            _texParams._value._iriIor       =   ior;
            _texParams._value._iriTmin      =   tmin;
            _texParams._value._iriTmax      =   tmax;
            _texParams.update();
        }
        /// <summary>
        /// AO 贴图使用的 UV 集(glTF occlusionTexture.texCoord,0/1)
        /// </summary>
        inline  void    setOcclusionTexCoord(uint idx)
        {
            _texParams._value._aoTexCoord   =   float(idx);
            _texParams.update();
        }
        /// <summary>
        /// 设置某贴图槽位的 UV 变换(KHR_texture_transform):
        ///   uv' = R(rotation) * S(scale) * uv + offset
        /// 对应 shader 侧 applyUvTr(slot, uv);默认全部为单位变换
        /// </summary>
        void    setUvTransform(uint slot,const float2& scale,const float2& offset,float rotation)
        {
            if (slot >= 6)
                return;
            const float c   =   cosf(rotation);
            const float s   =   sinf(rotation);
            float*  p       =   _texParams._value._uvTransform + slot * 8;
            p[0] = c * scale.x;     p[1] = -s * scale.y;    /// 第一列
            p[2] = s * scale.x;     p[3] =  c * scale.y;    /// 第二列
            p[4] = offset.x;        p[5] = offset.y;
            _texParams.update();
        }

        /// ===== 管线变体切换(glTF doubleSided / alphaMode BLEND / transmission)=====
        /// <summary>
        /// 按当前双面/混合状态重新选择管线并重绑资源:
        ///   pbr      单面 + 不混合(默认)
        ///   pbr2s    双面(cullMode=NONE)
        ///   pbrBlend 双面 + alpha 混合 + 不写深度
        /// </summary>
        void    applyPipeline()
        {
            String  prefix  =   _pipelinePrefix;
            if (_alphaBlend)
                prefix  +=  "Blend";
            else if (_doubleSided)
                prefix  +=  "2s";
            if (!setup(prefix))
            {
                /// 变体管线不存在(如自定义 prefix 未提供变体),回退基础管线
                setup(_pipelinePrefix);
                _alphaBlend =   false;
                _doubleSided=   false;
            }
            autoAttach();
            bindResources();
        }
        inline  void    setDoubleSided(bool sided)
        {
            if (_doubleSided == sided)
                return;
            _doubleSided    =   sided;
            /// 混合变体本身已是双面,无需切换
            if (!_alphaBlend)
                applyPipeline();
        }
        inline  void    setAlphaBlend(bool blend)
        {
            if (_alphaBlend == blend)
                return;
            _alphaBlend =   blend;
            applyPipeline();
        }

        /// <summary>
        /// 获取材质数据
        /// </summary>
        inline  auto&   data() 
        {
            return  _pbr;
        }
        const   auto&   data() const
        {
            return  _pbr;
        }
        /// <summary>
        /// 获取贴图标志/强度与 KHR 扩展参数
        /// </summary>
        inline  auto&   texParams()
        {
            return  _texParams;
        }
        const   auto&   texParams() const
        {
            return  _texParams;
        }
    public:
        virtual void        appDynamicState(CMDPtr cmd,EPrimitive pri) override
        {
            refreshSceneTexture();
            _pbr.checkDirty();
            _texParams.checkDirty();
            FEMaterial::appDynamicState(cmd,pri);
        }
        /// <summary>
        /// alpha 混合/transmission 材质为透明体,渲染排序时排到不透明几何之后
        /// </summary>
        virtual bool        isTransparent() const override
        {
            return  _alphaBlend;
        }
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
        virtual void        beginSetProp()
        {}
        virtual bool        setProperty(int prop,const KFValue& value)
        {
            UNUSED(prop);
            UNUSED(value);
            return  false;
        }
        virtual void        endSetProp(bool bModify)
        {
            UNUSED(bModify);
        }
    protected:
        /// <summary>
        /// 把 UBO 与全部贴图绑定到当前描述符集(setup/重 setup 后调用)
        /// </summary>
        void    bindResources()
        {
            auto    cameraUBO   =   _ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    lightSBO    =   _ctx.device().queryCache(FEConstUuid::LightsId);
            auto&   lights      =   _ctx.device().lights();
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Light,    {lightSBO},{lights.count() * sizeof(LightData)});
            bind(0,SB_Material, {_pbr._gpu.get()});
            bind(0,BIND_TEX_PARAMS,{_texParams._gpu.get()});
            bindAllTextures();
            update();
        }
        /// <summary>
        /// 设置/清除贴图存在标志
        /// </summary>
        inline  void    setTexFlag(PBRTexFlag flag,bool set)
        {
            if (set)    _texParams._value._flags |=  flag;
            else        _texParams._value._flags &= ~flag;
        }
        /// <summary>
        /// 把当前 6 张贴图绑定到对应描述符槽位
        /// </summary>
        inline  void    bindAllTextures()
        {
            bind(0,BIND_BASECOLOR,     {_baseColorMap.get()});
            bind(0,BIND_METALLICROUGH, {_mrMap.get()});
            bind(0,BIND_NORMAL,        {_normalMap.get()});
            bind(0,BIND_OCCLUSION,     {_occlusionMap.get()});
            bind(0,BIND_EMISSIVE,      {_emissiveMap.get()});
            bind(0,BIND_ENVIRONMENT,   {_environmentMap.get()});
            bind(0,BIND_IRRADIANCE,    {_irradianceMap.get()});
            bind(0,BIND_BRDF_LUT,      {_brdfLUT.get()});
            bind(0,BIND_IRI_THICKNESS, {_iriThicknessMap.get()});
            bind(0,BIND_SCENE_TEX,     {_sceneTexWhite.get()});
        /// 占位绑定随后由 bindSceneTex 按 transmission 因子修正
            bindSceneTex();
        }
        /// <summary>
        /// 创建 1x1 RGBA8 默认纹理
        /// </summary>
        TexPtr  createDefaultColor(const float4& color)
        {
            auto    tex =   _ctx.device().createTexture();
            FETexture::CreateInfo  ci;
            ci._width         =   1;
            ci._height        =   1;
            ci._format        =   FMT_R8G8B8A8_UNORM;
            ci._magFilter     =   FILTER_LINEAR;
            ci._minFilter     =   FILTER_LINEAR;
            ci._addressMode   =   ADDRESS_REPEAT;
            tex->create(ci);
            uint8   rgba[4];
            rgba[0] =   (uint8)(clamp(color.x,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[1] =   (uint8)(clamp(color.y,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[2] =   (uint8)(clamp(color.z,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[3] =   (uint8)(clamp(color.w,0.0f,1.0f) * 255.0f + 0.5f);
            tex->update(rgba,4);
            return  tex;
        }
        /// <summary>
        /// 创建 1x1x6 默认 cubemap(六面同色,单 mip)
        /// </summary>
        TexPtr  createDefaultCube(const float4& color)
        {
            auto    tex =   _ctx.device().createTexture();
            FETexture::CreateInfo  ci;
            ci._width         =   1;
            ci._height        =   1;
            ci._format        =   FMT_R8G8B8A8_UNORM;
            ci._magFilter     =   FILTER_LINEAR;
            ci._minFilter     =   FILTER_LINEAR;
            ci._addressMode   =   ADDRESS_CLAMP_TO_EDGE;
            ci._cube          =   true;
            tex->create(ci);
            uint8   rgba[4];
            rgba[0] =   (uint8)(clamp(color.x,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[1] =   (uint8)(clamp(color.y,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[2] =   (uint8)(clamp(color.z,0.0f,1.0f) * 255.0f + 0.5f);
            rgba[3] =   (uint8)(clamp(color.w,0.0f,1.0f) * 255.0f + 0.5f);
            uint8   faces[24];
            for (int i = 0; i < 6; ++i)
                memcpy(faces + i * 4, rgba, 4);
            tex->update(faces,sizeof(faces));
            return  tex;
        }
    protected:
        TBlock<PBRData>         _pbr;
        /// <summary>
        /// 贴图标志与强度参数 UBO(绑定 binding 11)
        /// </summary>
        TBlock<PBRTexParams>    _texParams;
        /// <summary>
        /// 管线 xml 前缀(默认 "pbr",变体为 "pbr2s"/"pbrBlend")
        /// </summary>
        String              _pipelinePrefix =   "pbr";
        /// <summary>
        /// 双面渲染(glTF doubleSided)→ 使用 pbr2s 管线
        /// </summary>
        bool                _doubleSided    =   false;
        /// <summary>
        /// alpha 混合(glTF BLEND/transmission)→ 使用 pbrBlend 管线
        /// </summary>
        bool                _alphaBlend     =   false;
        TexPtr              _baseColorMap;
        TexPtr              _mrMap;
        TexPtr              _normalMap;
        TexPtr              _occlusionMap;
        TexPtr              _emissiveMap;
        /// <summary>
        /// 环境贴图(cubemap,IBL 镜面预滤波用)
        /// </summary>
        TexPtr              _environmentMap;
        /// <summary>
        /// 漫反射辐照度 cubemap(球谐预卷积,IBL 漫反射用;缺省复用 _environmentMap)
        /// </summary>
        TexPtr              _irradianceMap;
        /// <summary>
        /// IBL split-sum BRDF 积分 LUT(RG float;缺省 (1,0) 常量,行为同旧版)
        /// </summary>
        TexPtr              _brdfLUT;
        /// <summary>
        /// 虹彩薄膜厚度贴图(G 通道;缺省 G=1 常量,标志位关闭时不参与采样)
        /// </summary>
        TexPtr              _iriThicknessMap;
        /// <summary>
        /// transmission 场景折射纹理(FEScene 挂接;缺省 1x1 白占位)
        /// </summary>
        TexPtr              _sceneTexMap;
        /// <summary>
        /// 占位白图:不透明材质的 BIND_SCENE_TEX 绑它而非真场景纹理——
        /// wgpu 禁止同一 render pass 内纹理既为 COLOR_TARGET 又为 RESOURCE,
        /// pass1(渲染到 sceneTex)会绘制不透明材质,故其不可绑真 sceneTex
        /// </summary>
        TexPtr              _sceneTexWhite;
        /// <summary>
        /// 是否已通过 setSceneTexture 挂接真场景纹理(区别于构造期占位)
        /// </summary>
        bool                _hasRealSceneTex    =   false;
        /// <summary>
        /// 自动模式(默认):渲染时从 device cache(SceneTexId)自取场景纹理,
        /// sceneTex 重建(resize)后下一帧自动重绑;外部 setSceneTexture 后转手动
        /// </summary>
        bool                _autoSceneTex       =   true;
        /// <summary>
        /// 本材质当前是否计入 s_transmissionMats(真纹理 + transmission>0)
        /// </summary>
        bool                _sceneTexCounted    =   false;
        /// <summary>
        /// 全局"已挂场景纹理且 transmission>0"材质计数(定义于 FEMaterialPBR.cpp)
        /// </summary>
        static  int         s_transmissionMats;
        /// <summary>
        /// 按 transmission 因子选择 BIND_SCENE_TEX 的绑定对象:
        /// transmission>0(仅 pass2 绘制)绑真场景纹理;否则绑占位白图
        /// (pass1 渲染到 sceneTex,wgpu 禁止该纹理同 pass 再作 RESOURCE)
        /// </summary>
        inline  void        bindSceneTex()
        {
            if (_hasRealSceneTex && _texParams._value._transmissionFactor > 0.0f)
                bind(0,BIND_SCENE_TEX,{_sceneTexMap.get()});
            else
                bind(0,BIND_SCENE_TEX,{_sceneTexWhite.get()});
        }
        /// <summary>
        /// 自动模式下每帧(渲染调用 appDynamicState 时)从 device cache 自取
        /// 场景折射纹理:构造时 sceneTex 可能尚未创建(resize 时才建),建成后
        /// 下一帧自动检测指针变化并重绑;sceneTex 重建后亦然。指针未变时零开销。
        /// </summary>
        inline  void        refreshSceneTexture()
        {
            if (!_autoSceneTex)
                return;
            auto    obj =   _ctx.device().queryCache(FEConstUuid::SceneTexId);
            auto    tex =   obj ? obj->as<FETexture>() : nullptr;
            if (tex == _sceneTexMap.get())
                return;
            _sceneTexMap        =   tex;
            _hasRealSceneTex    =   (tex != nullptr);
            bindSceneTex();
            update();
            updateSceneTexCount();
        }
        /// <summary>
        /// 按当前 纹理/透射因子 状态刷新全局计数
        /// </summary>
        inline  void        updateSceneTexCount()
        {
            const bool  should  =   _hasRealSceneTex
                                    && (_texParams._value._transmissionFactor > 0.0f);
            if (should && !_sceneTexCounted) { ++s_transmissionMats; _sceneTexCounted = true;  }
            if (!should && _sceneTexCounted) { --s_transmissionMats; _sceneTexCounted = false; }
        }
    };
}

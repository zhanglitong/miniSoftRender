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
        /// 贴图存在标志位(见 PBRTexFlag)
        uint    _flags;
        /// 法线贴图强度
        float   _normalScale;
        /// AO 强度
        float   _occlusionStrength;
        /// 自发光强度
        float   _emissiveStrength;
        PBRTexParams()
            :_flags(0)
            ,_normalScale(1.0f)
            ,_occlusionStrength(1.0f)
            ,_emissiveStrength(1.0f)
        {}
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
        };
        /// <summary>
        /// 贴图描述符绑定号,须与 shader 中 layout(binding=N) 严格一致
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
        };
    public:
        FEMaterialPBR(FEContext& ctx)
            :FEMaterial(ctx)
            ,_pbr(ctx)
            ,_texParams(ctx)
        {
            setup("pbr");
            autoAttach();
            _pbr.update();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    lightSBO    =   ctx.device().queryCache(FEConstUuid::LightsId);

            auto&   lights      =   ctx.device().lights();
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Light,    {lightSBO},{lights.count() * sizeof(LightData)});
            bind(0,SB_Material, {_pbr._gpu.get()});
            bind(0,BIND_TEX_PARAMS,{_texParams._gpu.get()});

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
            bindAllTextures();

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
            bindAllTextures();
            update();
        }
        virtual ~FEMaterialPBR()
        {
            _pbr._gpu   =   nullptr;
        }

        /// ===== 贴图设置接口 =====
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
        /// 设置环境贴图(cubemap,用于 IBL:最低 mip 作 irradiance 漫反射,
        /// 按 roughness 选择 mip 作预滤波镜面反射)
        /// </summary>
        inline  void    setEnvironmentMap(TexPtr tex)
        {
            _environmentMap = (tex != nullptr) ? tex : createDefaultCube(float4(0,0,0,1));
            setTexFlag(PBR_TEX_ENVIRONMENT, tex != nullptr);
            bind(0,BIND_ENVIRONMENT,{_environmentMap.get()});
            _pbr.update(); _texParams.update(); update();
        }
        /// 获取当前环境贴图
        inline  TexPtr  environmentMap() const
        {
            return  _environmentMap;
        }

        /// ===== 标量参数 =====
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
    public:
        virtual void        appDynamicState(CMDPtr cmd,EPrimitive pri) override
        {
            _pbr.checkDirty();
            _texParams.checkDirty();
            FEMaterial::appDynamicState(cmd,pri);
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
        /// 设置/清除贴图存在标志
        inline  void    setTexFlag(PBRTexFlag flag,bool set)
        {
            if (set)    _texParams._value._flags |=  flag;
            else        _texParams._value._flags &= ~flag;
        }
        /// 把当前 6 张贴图绑定到对应描述符槽位
        inline  void    bindAllTextures()
        {
            bind(0,BIND_BASECOLOR,     {_baseColorMap.get()});
            bind(0,BIND_METALLICROUGH, {_mrMap.get()});
            bind(0,BIND_NORMAL,        {_normalMap.get()});
            bind(0,BIND_OCCLUSION,     {_occlusionMap.get()});
            bind(0,BIND_EMISSIVE,      {_emissiveMap.get()});
            bind(0,BIND_ENVIRONMENT,   {_environmentMap.get()});
        }
        /// 创建 1x1 RGBA8 默认纹理
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
        /// 创建 1x1x6 默认 cubemap(六面同色,单 mip)
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
        /// 贴图标志与强度参数 UBO(绑定 binding 11)
        TBlock<PBRTexParams>    _texParams;
        TexPtr              _baseColorMap;
        TexPtr              _mrMap;
        TexPtr              _normalMap;
        TexPtr              _occlusionMap;
        TexPtr              _emissiveMap;
        /// 环境贴图(cubemap,IBL 用)
        TexPtr              _environmentMap;
    };
}

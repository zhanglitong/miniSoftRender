#pragma     once

#include    "../FEComponent.hpp"
#include    "../FEMath.hpp"
#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "../node/FENode.hpp"
#include    "../geometry/FEGeometryBox.hpp"
#include    "../material/FEMaterialSky.hpp"

namespace   FE
{
    /// <summary>
    /// 天空盒组件:实时程序化天空(移植 three.js webgl_shaders_sky 的
    /// Preetham 大气散射模型),不加载任何图片。
    /// 组件的职责:
    ///   1. 持有天空参数(turbidity/rayleigh/mie/luminance/太阳方向...)并写入
    ///      引擎缓存的 SkyUBO(绑定槽 SB_Sky),供 sky 管线实时求值;
    ///   2. 组装天空盒节点(立方体壳 + FEMaterialSky);
    ///   3. 每帧 update() 可自动推进太阳,模拟日升日落。
    /// 用法:
    ///   Component  sky  =   new FESkyComponent(ctx);
    ///   Node       node =   sky->createNode();
    ///   node->addComponent(sky);
    ///   scene->dispatchNodesToSystem({node});
    ///   sky->setSunElevationAzimuth(15.0f, 180.0f);   // 写入 UBO
    /// </summary>
    class   FESkyComponent:public FEComponent
    {
    public:
        FESkyComponent(FEContext& ctx)
            :FEComponent(ctx)
        {
            /// 天空参数属于渲染末端,优先级最低、最后执行
            _priority.setPriority(EPriority::EP_Last);
            _priority.setOrder(MaxInt16);
            /// 缓存引擎注册的天空 UBO(由 FEScene::initializeUBO 写入 device cache)
            auto    obj =   ctx.device().queryCache(FEConstUuid::SkyUBOId);
            _ubo        =   (obj != nullptr) ? obj->as<FEGPUBuffer>() : nullptr;
            apply();
        }
        FESkyComponent(const FESkyComponent& other)
            :FEComponent(other)
        {
            _value      =   other._value;
            _autoSun    =   other._autoSun;
            _elevation  =   other._elevation;
            _azimuth    =   other._azimuth;
            _elevSpeed  =   other._elevSpeed;
            _cloudCoverage  =   other._cloudCoverage;
            _cloudDensity   =   other._cloudDensity;
            _cloudElevation =   other._cloudElevation;
            _cloudSpeed     =   other._cloudSpeed;
            _ubo        =   other._ubo;
        }
        virtual ~FESkyComponent()
        {}
        /// <summary>
        /// 组装天空盒节点:立方体壳(pos+uv)+ FEMaterialSky
        /// 尺寸无关紧要(裁剪空间 z 被强制到远平面),返回节点需加入场景
        /// </summary>
        Node    createNode()
        {
            FEGeometryBox   geo(_ctx);
            /// 天空盒以相机为中心,壳尺寸只影响采样方向,取单位立方体即可
            geo.param()._size   =   float3(2.0f, 2.0f, 2.0f);
            geo.param()._segs   =   uint3(1, 1, 1);

            Mesh    mesh    =   geo.triangular({
                {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

            Node    node    =   new FENode(_ctx);
            node->setMesh(mesh);
            node->setMaterial(new FEMaterialSky(_ctx));
            node->makeDirty();
            node->update();
            return  node;
        }
        /// <summary>
        /// ===== 参数设置(立即写入 SkyUBO) =====
        /// </summary>
        void    setTurbidity(float v)
        {
            _value._params.x    =   v;
            apply();
        }
        void    setRayleigh(float v)
        {
            _value._params.y    =   v;
            apply();
        }
        void    setMieCoefficient(float v)
        {
            _value._params.z    =   v;
            apply();
        }
        void    setMieDirectionalG(float v)
        {
            _value._params.w    =   v;
            apply();
        }
        void    setLuminance(float v)
        {
            _value._sunDirection.w  =   v;
            apply();
        }
        /// <summary>
        /// ===== 云参数(对齐 three.js SkyMesh 的 uniforms)=====
        /// 云量:0 表示无云(着色器内直接跳过云的计算)
        /// </summary>
        void    setCloudCoverage(float v)
        {
            _cloudCoverage  =   v;
            _value._cloud.x =   v;
            apply();
        }
        /// <summary>
        /// 云密度:越大云越厚实、越不透明
        /// </summary>
        void    setCloudDensity(float v)
        {
            _cloudDensity   =   v;
            _value._cloud.y =   v;
            apply();
        }
        /// <summary>
        /// 云层高度:越大云看起来越低、越靠近地平线
        /// </summary>
        void    setCloudElevation(float v)
        {
            _cloudElevation =   v;
            _value._cloud.z =   v;
            apply();
        }
        /// <summary>
        /// 云速:驱动云流动与翻涌的速度(单位/秒)
        /// </summary>
        void    setCloudSpeed(float v)
        {
            _cloudSpeed     =   v;
            _value._anim.z  =   v;
            apply();
        }
        void    setUp(const float3& up)
        {
            _value._up  =   float4(up.x, up.y, up.z, 0.0f);
            apply();
        }
        void    setSunDirection(const float3& dir)
        {
            _value._sunDirection    =   float4(dir.x, dir.y, dir.z, _value._sunDirection.w);
            apply();
        }
        /// <summary>
        /// 由太阳高度角 / 方位角(角度制)设置太阳方向
        /// 引擎默认 Z 朝上:elevation 0=地平线, 90=天顶(+Z);azimuth 绕 Z 轴(0=+Y, 90=+X)
        /// </summary>
        void    setSunElevationAzimuth(float elevationDeg,float azimuthDeg)
        {
            _elevation  =   elevationDeg;
            _azimuth    =   azimuthDeg;
            const float theta   =   elevationDeg * (float(PI) / 180.0f);
            const float phi     =   azimuthDeg   * (float(PI) / 180.0f);
            const float ct      =   cosf(theta);
            const float st      =   sinf(theta);
            /// 高度角映射到 +Z(引擎 up),方位角绕 Z 轴
            setSunDirection(float3(ct * sinf(phi), ct * cosf(phi), st));
        }
        /// <summary>
        /// 自动推进太阳(日升日落):timeScale 单位 度/秒
        /// </summary>
        void    setAutoSun(bool enable,float elevationStartDeg = 10.0f,float timeScaleDegPerSec = 6.0f)
        {
            _autoSun    =   enable;
            _elevation  =   elevationStartDeg;
            _elevSpeed  =   timeScaleDegPerSec;
            setSunElevationAzimuth(_elevation, _azimuth);
        }
        const   UBOSky& sky() const     {   return  _value;    }
        UBOSky&         sky()           {   return  _value;    }
        /// <summary>
        /// 组件每一帧更新:推进云动画时间并写回 UBO;自动模式下同时推进太阳
        /// </summary>
        virtual bool    update(const real& deltaTm) override
        {
            /// 云动画:每帧累计时间(着色器用它驱动云流动与翻涌)
            _timeAccum      +=  float(deltaTm);
            _value._anim.x  =   _timeAccum;
            if  (_autoSun)
            {
                _elevation  +=  _elevSpeed * float(deltaTm);
                if  (_elevation > 90.0f)
                    _elevation  =   -10.0f;
                /// 内部会 apply(),一并写入上面的云时间
                setSunElevationAzimuth(_elevation, _azimuth);
            }
            else
            {
                apply();
            }
            return  false;
        }
    protected:
        /// <summary>
        /// 同步天空参数到 GPU
        /// </summary>
        void    apply()
        {
            if  (_ubo != nullptr)
                _ubo->update(&_value, sizeof(_value), 0);
        }
    protected:
        UBOSky          _value;
        FEGPUBuffer*    _ubo        =   nullptr;
        bool            _autoSun    =   false;
        float           _elevation  =   10.0f;
        float           _azimuth    =   180.0f;
        float           _elevSpeed  =   6.0f;
        /// <summary>
        /// 云参数(默认值取 three.js webgpu_sky 示例面板值)
        /// </summary>
        float           _cloudCoverage  =   0.4f;
        float           _cloudDensity   =   0.4f;
        float           _cloudElevation =   0.5f;
        float           _cloudSpeed     =   0.00002f;
        /// <summary>
        /// 云动画累计时间(秒)
        /// </summary>
        float           _timeAccum      =   0.0f;
    };
}

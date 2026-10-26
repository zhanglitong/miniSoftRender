#pragma     once

#include    "FEMaterialPBR.hpp"
#include    "../graphic/FEOITManager.h"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialOIT,"{7A1C2E94-3B5D-4F68-9C0A-52E8D1B4F7A3}");
    /// <summary>
    /// OIT (PPLL) 透明材质:继承 PBR 材质,但管线/DSets 走 OIT geometry pass。
    /// fragment 在 geometry pass 写入每像素链表,由 OIT color pass 排序合成。
    /// 使用前提:FEOITManager::init() 已完成(注册 oitGeometry 管线)并 setActive()。
    /// </summary>
    class   FE_API  FEMaterialOIT:public FEMaterialPBR
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialOIT)
    public:
        FEMaterialOIT(FEContext& ctx)
            :FEMaterialPBR(ctx)
        {
            setupOITBindings();
        }

        FEMaterialOIT(const FEMaterialOIT& other)
            :FEMaterialPBR(other)
        {
            _pbr._value =   other._pbr._value;
            _pbr.update();
            setupOITBindings();
        }
        virtual ~FEMaterialOIT()
        {}
        /// <summary>
        /// OIT 双趟过滤标记:LIST 趟画进 OIT geometry pipeline
        /// </summary>
        virtual bool        isOIT() const override
        {
            return  true;
        }
    protected:
        /// <summary>
        /// 用 OIT geometry 管线替换基类 pbr 管线,并重绑 DSets:
        /// camera/全局 UBO/lights 自动关联 + 材质 UBO + OIT SSBO(10/11/12)。
        /// (基类构造中注册的 lights notify 回调捕获 this->bind,换 dsets 后依然有效)
        /// </summary>
        void    setupOITBindings()
        {
            setup("oitGeometry");
            if (_dsets.empty())
                return;
            autoAttach();
            auto    cameraUBO   =   _ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    lightSBO    =   _ctx.device().queryCache(FEConstUuid::LightsId);
            auto&   lights      =   _ctx.device().lights();
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Light,    {lightSBO},{lights.count() * sizeof(LightData)});
            bind(0,SB_Material, {_pbr._gpu.get()});
            /// 天空环境(昼夜门控):OIT 几何趟写入的颜色同样随昼夜压暗
            bindSkyEnvironment();
            /// OIT SSBO (binding 10/11/12)
            if (auto mgr = FEOITManager::active())
            {
                for (auto& dset : _dsets)
                    mgr->bindToDSet(dset);
            }
            update();
        }
    };
}

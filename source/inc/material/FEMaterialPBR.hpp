#pragma     once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPBR,"{C253DF90-EA68-44D6-9558-68D244EE0066}");
    class   FEMaterialPBR:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPBR)
    public:
        FEMaterialPBR(FEContext& ctx)
            :FEMaterial(ctx)
            ,_pbr(ctx)
        {
            setup("pbr");
            _pbr.update();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    lightSBO    =   ctx.device().queryCache(FEConstUuid::LightsId);

            auto&   lights      =   ctx.device().lights();
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Light,    {lightSBO},{lights.count() * sizeof(LightData)});
            bind(0,SB_Material, {_pbr._gpu.get()});
            
            /// 灯光对象
            lightSBO->as<FENotify>()->addFlagChangedNotify(this,[this](Object object)
            {
                if (object->flags().hasFlag(FLAG_UPDATE))
                {
                    bind(0,SB_Light,{object});
                }
            });
            /// 同步数据到显卡
            update();
        }

        FEMaterialPBR(const FEMaterialPBR& other)
            :FEMaterial(other)
            ,_pbr(_ctx)
        {}
        virtual ~FEMaterialPBR()
        {}
        /// <summary>
        /// 获取材质数据
        /// </summary>
        /// <returns></returns>
        inline  auto&   data() 
        {
            return  _pbr;
        }
        const   auto&   data() const
        {
            return  _pbr;
        }
    protected:
        TBlock<PBRData>  _pbr;
    };
}
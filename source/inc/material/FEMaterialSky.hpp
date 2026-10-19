#pragma once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialSky,"{A1C7E5D9-3B42-4F86-9C10-7D2E4A6B8F31}");
    /// <summary>
    /// 天空盒材质:大气散射(Preetham)程序化天空管线(sky/*),
    /// 相机 UBO 绑定到 SB_Camera,天空参数 UBO 绑定到 SB_Sky,
    /// 参数由 FESkyComponent 写入并经本材质上传
    /// </summary>
    class   FE_API  FEMaterialSky:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEMaterialSky)
    public:
        FEMaterialSky(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("sky");
            /// 自动关联内置 UBO(camera/engineState 等)
            autoAttach();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    skyUBO      =   ctx.device().queryCache(FEConstUuid::SkyUBOId);
            /// 天空参数块名为 SkyBlock,autoAttach 覆盖不到,这里显式绑定
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Sky,      {skyUBO});

            /// 同步数据到显卡
            update();
        }
        FEMaterialSky(const FEMaterialSky& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialSky()
        {}
        /// <summary>
        /// 相机位于天空盒壳内部:基类对三角形图元默认设置背面剔除(CM_BACK),
        /// 盒体内表面朝向相机、在光栅化阶段全部被裁掉,导致天空不可见。
        /// 这里追加关闭剔除(CM_NULL),双面绘制整个天空壳。
        /// (cullMode 是动态状态,会覆盖管线 XML 中的静态设置,故必须在此覆盖)
        /// </summary>
        virtual void    appDynamicState(CMDPtr cmd,EPrimitive pri) override
        {
            FEMaterial::appDynamicState(cmd,pri);
            cmd->setCullMode(CullMode::CM_NULL);
        }
    };
}

#pragma     once
#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialV3N3C4,"{B3F6A2D1-5E7C-4A89-9B04-6C2D8E1F7A35}");
    /// <summary>
    /// 受光的顶点色材质:pos + normal + color0。
    /// 用于矢量字体挤出等自带法线与顶点色的实体网格,
    /// 片元着色器用固定方向光 + 环境光表现厚度/侧壁明暗。
    /// </summary>
    class   FE_API  FEMaterialV3N3C4:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialV3N3C4)
    public:
        FEMaterialV3N3C4(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("v3n3c4");
            autoAttach();
            /// 相机 UBO:着色器 CameraBlock 经反射块名为 "CameraBlock",
            /// autoAttach 的名称匹配覆盖不到,需显式绑定
            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bind(0,SB_Camera,   {cameraUBO});
            bindSkyEnvironment();
            update();
        }
        FEMaterialV3N3C4(const FEMaterialV3N3C4& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialV3N3C4()
        {}
        /// <summary>
        /// 字形侧壁/背盖的绕序取决于字体轮廓方向,统一关闭背面剔除,
        /// 避免部分字形或部分侧壁被剔除而残缺
        /// </summary>
        virtual void    appDynamicState(CMDPtr cmd,EPrimitive pri) override
        {
            FEMaterial::appDynamicState(cmd,pri);
            cmd->setCullMode(CullMode::CM_NULL);
        }
    };
}

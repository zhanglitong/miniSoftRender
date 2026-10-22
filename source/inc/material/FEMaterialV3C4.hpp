#pragma     once
#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialV3C4,"{8C072784-4024-4D67-9215-65744DB266FC}");
    class   FE_API  FEMaterialV3C4:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialV3C4)
    public:
        FEMaterialV3C4(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("v3c4");
            /// <summary>
            /// 自动将系统数据与材质所需数据关联起来
            /// </summary>
            autoAttach();
            /// 显式绑定相机 UBO:v3c4 着色器的 CameraBlock 经反射得到的块名为
            /// "CameraBlock" 而非 "_camera",autoAttach 的名称匹配覆盖不到,
            /// 缺少此绑定会导致所有 v3c4 绘制(线框/纯色三角形等)因相机
            /// VP 矩阵未定义而整体不可见
            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bind(0,SB_Camera,   {cameraUBO});
            /// <summary>
            /// 更新数据
            /// </summary>
            update();
        }
        FEMaterialV3C4(const FEMaterialV3C4& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialV3C4()
        {}
    };
}

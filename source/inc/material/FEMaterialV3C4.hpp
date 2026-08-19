#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialV3C4,"{8C072784-4024-4D67-9215-65744DB266FC}");
    class   FEMaterialV3C4:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialV3C4)
    public:
        FEMaterialV3C4(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("v3c4");
            /// 自动将系统数据与材质所需数据关联起来
            autoAttach();
            /// 更新数据
            update();
        }
        FEMaterialV3C4(const FEMaterialV3C4& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialV3C4()
        {}
    };
}

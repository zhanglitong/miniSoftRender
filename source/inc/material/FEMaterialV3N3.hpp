#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialV3N3,"{10A254C4-1C3C-444A-9ED8-F32C972761A6}");
    class   FEMaterialV3N3:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialV3N3)
    public:
        FEMaterialV3N3(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("v3c4");
        }

        FEMaterialV3N3(const FEMaterialV3N3& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialV3N3()
        {}
    };
}
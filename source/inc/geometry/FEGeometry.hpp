#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEUserObject.hpp"
#include    "../FEMeshLibrary.hpp"
#include    "../FEWriterHelper.hpp"
#include    "../FEReaderHelper.hpp"
#include    "../mesh/FEAttributeLibrary.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometry,"{13D495E7-5B19-4B4D-9A71-8AD7AC8033B2}");

    class   FE_API  FEGeometry :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometry)
    public:
        FEGeometry(FEContext& ctx)
            :FEObject(ctx)
        {}

        FEGeometry(const FEGeometry& other) = default;
        virtual ~FEGeometry()
        {}
    };
    using   Geometry    =   SharedPtr<FEGeometry>;
    using   Geometrys   =   std::vector<Geometry>;
}


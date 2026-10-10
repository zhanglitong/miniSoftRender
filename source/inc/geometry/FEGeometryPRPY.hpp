#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPRPYImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPRPY,"{1493C877-E0DE-4E4E-B453-67F1411C1290}");

    /// <summary>
    /// 棱台体 (Rectangular Frustum)
    /// bottomXlen / bottomYlen / topXlen / topYlen / xOffset / yOffset / height
    /// </summary>
    class   FE_API  FEGeometryPRPY : public TFEGeometryParam<FEParamPRPY>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPRPY)
    public:
        FEGeometryPRPY(FEContext& ctx)
            : TFEGeometryParam<FEParamPRPY>(ctx)
        {}
        FEGeometryPRPY(const FEGeometryPRPY& other)
            : TFEGeometryParam<FEParamPRPY>(other)
        {}
        virtual ~FEGeometryPRPY()
        {}

        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPRPYImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FEPRPYImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEPRPYImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEPRPYImpl::build<false,   true>(_param,data);
            else
                FEPRPYImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

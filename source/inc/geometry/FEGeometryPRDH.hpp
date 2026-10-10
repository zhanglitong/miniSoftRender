#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPRDHImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPRDH,"{8A53C8D0-C51A-4324-8F88-617F4BFE72EC}");

    /// <summary>
    /// 椭球体 (Spheroid)
    /// radius / height
    /// </summary>
    class   FE_API  FEGeometryPRDH : public TFEGeometryParam<FEParamPRDH>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPRDH)
    public:
        FEGeometryPRDH(FEContext& ctx)
            : TFEGeometryParam<FEParamPRDH>(ctx)
        {}
        FEGeometryPRDH(const FEGeometryPRDH& other)
            : TFEGeometryParam<FEParamPRDH>(other)
        {}
        virtual ~FEGeometryPRDH()
        {}

        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPRDHImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FEPRDHImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEPRDHImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEPRDHImpl::build<false,   true>(_param,data);
            else
                FEPRDHImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

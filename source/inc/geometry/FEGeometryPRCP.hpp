#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPRCPImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPRCP,"{6F21332F-4836-449F-AB12-36AC9CC685AA}");

    /// <summary>
    /// 球冠体 (Spherical Cap)
    /// radius / height
    /// </summary>
    class   FE_API  FEGeometryPRCP : public TFEGeometryParam<FEParamPRCP>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPRCP)
    public:
        FEGeometryPRCP(FEContext& ctx)
            : TFEGeometryParam<FEParamPRCP>(ctx)
        {}
        FEGeometryPRCP(const FEGeometryPRCP& other)
            : TFEGeometryParam<FEParamPRCP>(other)
        {}
        virtual ~FEGeometryPRCP()
        {}

        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPRCPImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FEPRCPImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEPRCPImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEPRCPImpl::build<false,   true>(_param,data);
            else
                FEPRCPImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

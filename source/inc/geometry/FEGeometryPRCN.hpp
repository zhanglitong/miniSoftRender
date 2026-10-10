#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPRCNImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPRCN,"{9891D8CB-01AB-41D2-829B-1534C734A882}");

    /// <summary>
    /// 圆台体 (Frustum with offset and tilt)
    /// bottomRadius / topRadius / height / xOffset / yOffset
    /// bottomTiltX / bottomTiltY / topTiltX / topTiltY
    /// </summary>
    class   FE_API  FEGeometryPRCN : public TFEGeometryParam<FEParamPRCN>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPRCN)
    public:
        FEGeometryPRCN(FEContext& ctx)
            : TFEGeometryParam<FEParamPRCN>(ctx)
        {}
        FEGeometryPRCN(const FEGeometryPRCN& other)
            : TFEGeometryParam<FEParamPRCN>(other)
        {}
        virtual ~FEGeometryPRCN()
        {}

        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPRCNImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FEPRCNImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEPRCNImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEPRCNImpl::build<false,   true>(_param,data);
            else
                FEPRCNImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

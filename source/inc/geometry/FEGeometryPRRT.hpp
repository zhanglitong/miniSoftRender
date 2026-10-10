#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPRRTImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPRRT,"{F1ABD843-4ABC-4364-BF5F-3CEDBA24C567}");

    /// <summary>
    /// 方环体 (Rectangular Ring Torus)
    /// innerRadius / outerRadius / height / arcAngle
    /// </summary>
    class   FE_API  FEGeometryPRRT : public TFEGeometryParam<FEParamPRRT>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPRRT)
    public:
        FEGeometryPRRT(FEContext& ctx)
            : TFEGeometryParam<FEParamPRRT>(ctx)
        {}
        FEGeometryPRRT(const FEGeometryPRRT& other)
            : TFEGeometryParam<FEParamPRRT>(other)
        {}
        virtual ~FEGeometryPRRT()
        {}

        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPRRTImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FEPRRTImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEPRRTImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEPRRTImpl::build<false,   true>(_param,data);
            else
                FEPRRTImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

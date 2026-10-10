#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FERingImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryRing,"{FE570AA5-36CA-4633-94DD-C21F78171CB8}");

    /// <summary>
    /// 圆环
    /// </summary>
    class   FE_API  FEGeometryRing :public TFEGeometryParam<FEParamRing>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryRing)
    public:
        FEGeometryRing(FEContext& ctx)
            : TFEGeometryParam<FEParamRing>(ctx)
        {}
        FEGeometryRing(const FEGeometryRing& other)
            : TFEGeometryParam<FEParamRing>(other)
        {}
        virtual ~FEGeometryRing()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FERingImpl  data;
            bool        attrPos =   false;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            /// 获取槽对应的实际属性对象
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_POS))          attrPos =    true;
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))     attrUV   =   true;
            }
            if (attrNor && attrUV)
                FERingImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FERingImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FERingImpl::build<false,   true>(_param,data);
            else
                FERingImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

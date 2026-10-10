#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FETorusKnotImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryTorusKnot,"{7CD68877-CAE5-4EAA-A887-5EF81A34CF12}");

    /// <summary>
    /// 圆环扭结体
    /// </summary>
    class   FE_API  FEGeometryTorusKnot :public TFEGeometryParam<FEParamTorusKnot>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryTorusKnot)
    public:
        FEGeometryTorusKnot(FEContext& ctx)
            : TFEGeometryParam<FEParamTorusKnot>(ctx)
        {}
        FEGeometryTorusKnot(const FEGeometryTorusKnot& other)
            : TFEGeometryParam<FEParamTorusKnot>(other)
        {}
        virtual ~FEGeometryTorusKnot()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FETorusKnotImpl data;
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
                FETorusKnotImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FETorusKnotImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FETorusKnotImpl::build<false,   true>(_param,data);
            else
                FETorusKnotImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

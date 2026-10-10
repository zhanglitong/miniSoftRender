#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FECircleImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryCircle,"{94FBF60C-98C0-4B94-9AC7-D4EA00876596}");

    /// <summary>
    /// 圆形
    /// </summary>
    class   FE_API  FEGeometryCircle :public TFEGeometryParam<FEParamCircle>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryCircle)
    public:
        FEGeometryCircle(FEContext& ctx)
            : TFEGeometryParam<FEParamCircle>(ctx)
        {}
        FEGeometryCircle(const FEGeometryCircle& other)
            : TFEGeometryParam<FEParamCircle>(other)
        {}
        virtual ~FEGeometryCircle()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FECircleImpl data;
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
                FECircleImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FECircleImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FECircleImpl::build<false,   true>(_param,data);
            else
                FECircleImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

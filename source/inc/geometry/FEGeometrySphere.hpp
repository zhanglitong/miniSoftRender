#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FESphereImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometrySphere,"{340A4ABF-EA24-41B4-AB8D-BBC916632585}");

    /// <summary>
    /// 球体
    /// </summary>
    class   FE_API  FEGeometrySphere :public TFEGeometryParam<FEParamSphere>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometrySphere)
    public:
        FEGeometrySphere(FEContext& ctx)
            : TFEGeometryParam<FEParamSphere>(ctx)
        {}
        FEGeometrySphere(const FEGeometrySphere& other)
            : TFEGeometryParam<FEParamSphere>(other)
        {}
        virtual ~FEGeometrySphere()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FESphereImpl data;
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
                FESphereImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FESphereImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FESphereImpl::build<false,   true>(_param,data);
            else
                FESphereImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

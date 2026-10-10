#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEEllipsoidEGMImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryEllipsoidEGM,"{020BCD78-6DB0-4A7E-82CE-1B5F38CDF2DC}");

    /// <summary>
    /// 椭球体(Elliptical Grid Mapping 算法)
    /// </summary>
    class   FE_API  FEGeometryEllipsoidEGM :public TFEGeometryParam<FEParamEllipsoidEGM>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryEllipsoidEGM)
    public:
        FEGeometryEllipsoidEGM(FEContext& ctx)
            : TFEGeometryParam<FEParamEllipsoidEGM>(ctx)
        {}
        FEGeometryEllipsoidEGM(const FEGeometryEllipsoidEGM& other)
            : TFEGeometryParam<FEParamEllipsoidEGM>(other)
        {}
        virtual ~FEGeometryEllipsoidEGM()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEEllipsoidEGMImpl  data;
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
                FEEllipsoidEGMImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEEllipsoidEGMImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEEllipsoidEGMImpl::build<false,   true>(_param,data);
            else
                FEEllipsoidEGMImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FETorusImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryTorus,"{2E6436BA-4329-45BC-A900-89C4531438A1}");

    /// <summary>
    /// 圆环体
    /// </summary>
    class   FE_API  FEGeometryTorus :public TFEGeometryParam<FEParamTorus>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryTorus)
    public:
        FEGeometryTorus(FEContext& ctx)
            : TFEGeometryParam<FEParamTorus>(ctx)
        {}
        FEGeometryTorus(const FEGeometryTorus& other)
            : TFEGeometryParam<FEParamTorus>(other)
        {}
        virtual ~FEGeometryTorus()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FETorusImpl data;
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
                FETorusImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FETorusImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FETorusImpl::build<false,   true>(_param,data);
            else
                FETorusImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

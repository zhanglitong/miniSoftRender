#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEBoxImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryBox,"{B4917A28-437D-4396-A534-3F300CF2DCE1}");

    class   FE_API  FEGeometryBox :public TFEGeometryParam<FEParamBox>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryBox)
    public:
        FEGeometryBox(FEContext& ctx)
            : TFEGeometryParam<FEParamBox>(ctx)
        {}

        FEGeometryBox(const FEGeometryBox& other)
            : TFEGeometryParam<FEParamBox>(other)
        {}
        virtual ~FEGeometryBox()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEBoxImpl   data;
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
                FEBoxImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FEBoxImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FEBoxImpl::build<false,   true>(_param,data);
            else
                FEBoxImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}


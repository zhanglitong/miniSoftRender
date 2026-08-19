#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEBoxImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryBox,"{B4917A28-437D-4396-A534-3F300CF2DCE1}");

    struct  FEParamBox
    {
        /// <summary>
        /// 立方体的大小
        /// </summary>
        float3  _size   =   float3(1,1,1);
        /// <summary>
        /// 分段数量
        /// </summary>
        uint3   _segs   =   uint3(1,1,1);
    };

    class   FEGeometryBox :public TFEGeometryParam<FEParamBox>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryBox)
    public:
        FEGeometryBox(FEContext& ctx)
            :TFEGeometryParam<FEParamBox>(ctx)
        {}

        FEGeometryBox(const FEGeometryBox& other)
            :TFEGeometryParam<FEParamBox>(other)
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
            FEBoxImplPtr    ptr     =   nullptr;
            bool            attrPos =   false;
            bool            attrNor =   false;
            bool            attrUV  =   false;
            /// 获取槽对应的实际属性对象
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_POS) )         attrPos =    true; 
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                ptr =   new TBoxHelper<true,    true>(_ctx,_param._size,_param._segs);
            else if(attrNor)
                ptr =   new TBoxHelper<true,    false>(_ctx,_param._size,_param._segs);
            else if(attrUV)
                ptr =   new TBoxHelper<false,   true>(_ctx,_param._size,_param._segs);
            else
                ptr =   new TBoxHelper<false,   false>(_ctx,_param._size,_param._segs);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,ptr->vertices,ptr->normals,{},ptr->uvs,ptr->indices);
        }
    };
}


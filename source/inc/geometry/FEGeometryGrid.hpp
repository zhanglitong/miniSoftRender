#pragma     once
#include    "FEGeometryParam.hpp"
#include    "detail/FEGridImpl.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryGrid,"{7FBAF090-6D30-4E94-83AD-C9C3A5BACDFB}");
    
    class FEGeometryGrid :public TFEGeometryParam<FEParamGrid>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryGrid)
    public:
        /// <summary>
        /// 
        /// </summary>
        /// <param name="ctx"></param>
        FEGeometryGrid(FEContext& ctx)
            : TFEGeometryParam<FEParamGrid>(ctx)
        {}
        FEGeometryGrid(const FEGeometryGrid& other)
            : TFEGeometryParam<FEParamGrid>(other)
        {}
        virtual ~FEGeometryGrid()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEGridImpl      data;
            bool            attrPos     =   false;
            bool            attrColor   =   false;
            /// 获取槽对应的实际属性对象
            for (auto& var : inputs)
            {
                if (var.slot() && (IS_VERTEX_POS) )   attrPos     =   true; 
                if (var.slot() && (IS_VERTEX_COLOR0)) attrColor   =   true;
            }
            if (attrColor)
                FEGridImpl::build<true>(_param,data);
            else
                FEGridImpl::build<false>(_param,data);
            _mesh   =   FEMeshBuilder::makeMeshLine(_ctx,inputs,data.vertices,data.colors);
            return  _mesh;
        }
    };
}

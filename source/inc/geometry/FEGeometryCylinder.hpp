#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FECylinderImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryCylinder,"{731AAD1C-4BF9-46C8-9B83-8282BD8A2F59}");
    /// <summary>
    /// 圆柱
    /// 目标形状	radiusTop	radiusBottom	radialSegments
    /// 标准圆锥	0	1	32 (或更高)
    /// 三棱锥	    0	1	3
    /// 四棱锥     (金字塔)	0	1	4
    /// 五棱锥	    0	1	5
    /// 六棱锥	    0	1	6
    /// </summary>
    
    class   FEGeometryCylinder : public TFEGeometryParam<FEParamCylinder>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryCylinder)
    public:
		/// <summary>
        /// 构造设置输入顶点的属性数据
        /// </summary>
        FEGeometryCylinder(FEContext& ctx)
            : TFEGeometryParam<FEParamCylinder>(ctx)
        {}
        FEGeometryCylinder(const FEGeometryCylinder& other)
            : TFEGeometryParam<FEParamCylinder>(other)
        {}
        virtual ~FEGeometryCylinder()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FECylinderImpl  data;
            bool            attrPos =   false;
            bool            attrNor =   false;
            bool            attrUV  =   false;
            /// 获取槽对应的实际属性对象
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_POS))          attrPos =    true; 
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FECylinderImpl::build<true,    true>(_param,data);
            else if(attrNor)
                FECylinderImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FECylinderImpl::build<false,   true>(_param,data);
            else
                FECylinderImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

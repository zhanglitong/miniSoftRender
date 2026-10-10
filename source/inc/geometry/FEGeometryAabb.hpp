#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEAabbImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryAabb,"{BA9D9450-DEA8-4ABC-8E1E-95DB768A1059}");

    /// <summary>
    /// 轴对齐包围盒(线框)
    /// </summary>
    class   FE_API  FEGeometryAabb :public TFEGeometryParam<FEParamAabb>
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometryAabb)
    public:
        FEGeometryAabb(FEContext& ctx)
            : TFEGeometryParam<FEParamAabb>(ctx)
        {}
        FEGeometryAabb(const FEGeometryAabb& other)
            : TFEGeometryParam<FEParamAabb>(other)
        {}
        virtual ~FEGeometryAabb()
        {}
        /// <summary>
        /// 根据属性生成mesh(线框)
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEAabbImpl  data;
            /// AABB 生成位置、颜色与线索引，不需要法线/UV
            FEAabbImpl::build<false,   false>(_param,data);

            Mesh    mesh    =   FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,data.colors,data.uvs,data.indices);
            /// AABB 为线框渲染,将拓扑从默认 PRI_TRIANGLES 改为 PRI_LINES
            if (mesh)
            {
                Primitives   pris    =   mesh->primitives();
                for (auto& pri : pris)
                    pri->setPrimitive(PRI_LINES);
                mesh->setPrimitives(pris);
            }
            return  mesh;
        }
    };
}

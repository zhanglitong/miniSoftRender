#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FETubeImpl.hpp"
#include    "../mesh/FEMeshHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryTube,"{C4E17A2B-9D3F-4A6E-B857-2F1D9C83E406}");
    /// <summary>
    /// 管道: 沿 3D 样条曲线(直线/二次贝塞尔/三次贝塞尔/Catmull-Rom)挤出圆形截面
    /// 移植自 three.js r160 TubeGeometry + extras/curves
    /// </summary>
    class   FE_API  FEGeometryTube : public TFEGeometryParam<FEParamTube>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryTube)
    public:
        FEGeometryTube(FEContext& ctx)
            : TFEGeometryParam<FEParamTube>(ctx)
        {}
        FEGeometryTube(const FEGeometryTube& other)
            : TFEGeometryParam<FEParamTube>(other)
        {}
        virtual ~FEGeometryTube()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FETubeImpl  data;
            bool        attrNor =   false;
            bool        attrUV  =   false;
            for (auto& var : inputs)
            {
                if (var.slot() & (IS_VERTEX_NOR))          attrNor  =   true;
                if (var.slot() & (IS_VERTEX_TEXCOORD0))    attrUV   =   true;
            }
            if (attrNor && attrUV)
                FETubeImpl::build<true,    true >(_param,data);
            else if(attrNor)
                FETubeImpl::build<true,    false>(_param,data);
            else if(attrUV)
                FETubeImpl::build<false,   true >(_param,data);
            else
                FETubeImpl::build<false,   false>(_param,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

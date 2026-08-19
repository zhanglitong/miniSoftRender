#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FEPlaneImpl.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPlane,"{623FBD2A-3EB4-4C5A-AED8-94EB37006EDC}");

    struct  FEParamPlane
    {
        /// <summary>
        /// 大小
        /// </summary>
        float   _width  =   1.0f;
        float   _height =   1.0f;
        /// <summary>
        /// 分段数量
        /// </summary>
        uint    _wSegs  =   1;
        uint    _hSegs  =   1;
    };

    /// <summary>
    /// 描述的是xoy平面
    /// </summary>
    class   FEGeometryPlane : public TFEGeometryParam<FEParamPlane>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPlane)
    public:
		/// <summary>
        /// 构造设置输入顶点的属性数据
        /// </summary>
        FEGeometryPlane(FEContext& ctx)
            : TFEGeometryParam<FEParamPlane>(ctx)
        {}
        FEGeometryPlane(const FEGeometryPlane& other)
            : TFEGeometryParam<FEParamPlane>(other)
        {}
        virtual ~FEGeometryPlane()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            FEPlaneImpl     data;
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
                FEPlaneImpl::build<true,    true>(_param._width,_param._height,_param._wSegs,_param._hSegs,data);
            else if(attrNor)
                FEPlaneImpl::build<true,    false>(_param._width,_param._height,_param._wSegs,_param._hSegs,data);
            else if(attrUV)
                FEPlaneImpl::build<false,   true>(_param._width,_param._height,_param._wSegs,_param._hSegs,data);
            else
                FEPlaneImpl::build<false,   false>(_param._width,_param._height,_param._wSegs,_param._hSegs,data);

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,{},data.uvs,data.indices);
        }
    };
}

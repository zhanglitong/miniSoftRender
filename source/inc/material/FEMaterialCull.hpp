#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialCull,"{2C8750B6-9D4A-4DFE-894E-DD88C182AA07}");
    class   FEMaterialCull:public FEMaterial
    {
    public:
        using   CullParamObject     =   TBlock<CullParam,HOST_COHERENT_BIT>;                   
        using   CullResultObject    =   TStorge<CullResult,HOST_COHERENT_BIT>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialCull)
    public:
        FEMaterialCull(FEContext& ctx)
            :FEMaterial(ctx)
            ,_cullParam(ctx)
            ,_cullResult(ctx)
        {  
            Pipeline    result;
            if(_ctx.device().pipelines().query("computeCull/default",result))
            {
                for (size_t i = 0; i < PRI_MAX; i++)
                {
                    _pipelines[i]   =   result;
                }
                _dsets  =   result->createDSets();
            }
            /// 自动将系统数据与材质所需数据关联起来
            autoAttach();
            /// 更新数据
            update();
        }
        FEMaterialCull(const FEMaterialCull& other)
            :FEMaterial(other)
            ,_cullParam(other._ctx)
            ,_cullResult(other._ctx)
        {
            _cullParam._value   =   other._cullParam._value;
            _cullParam.update();
            setup("computeCull/default");
            /// 自动将系统数据与材质所需数据关联起来
            autoAttach();
            /// 更新数据
            update();
        }
        virtual ~FEMaterialCull()
        {}
    public:
        CullParamObject     _cullParam;
        CullResultObject    _cullResult;
    };

    using   MaterialCull    =   SharedPtr<FEMaterialCull>;
}

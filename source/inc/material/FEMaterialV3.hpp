#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialV3,"{924598F0-6271-4008-AAC1-D46BC4CE7B1F}");
    class   FE_API  FEMaterialV3:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialV3)
    public:
        FEMaterialV3(FEContext& ctx)
            :FEMaterial(ctx)
        {  
            _pushConst._point   =   0;
            setup("v3");
            /// <summary>
            /// 自动将系统数据与材质所需数据关联起来
            /// </summary>
            autoAttach();
            /// 天空环境(昼夜门控):与其他非 UI 材质共用同一份 SkyUBO
            bindSkyEnvironment();
            /// <summary>
            /// 更新数据
            /// </summary>
            update();
        }
        FEMaterialV3(const FEMaterialV3& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialV3()
        {}

        /// <summary>
        /// 子类重写，基类使用，返回0,则不写入
        /// </summary>
        /// <returns></returns>
        virtual uint        pushConstantSize() const override
        {
            return  sizeof(_pushConst);
        }
        virtual PCVOID      pushConstantData() const override
        {
            return  &_pushConst;
        }

    protected:
        PointData   _pushConst;
    };

    using   MaterialV3  =   SharedPtr<FEMaterialV3>;
}

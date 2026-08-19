#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPoint,"{5503589E-6D04-4C8B-8836-5ED302C1A623}");
    class   FEMaterialPoint:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPoint)
    public:
        FEMaterialPoint(FEContext& ctx)
            :FEMaterial(ctx)
        {  
            setup("point");
            /// 自动将系统数据与材质所需数据关联起来
            autoAttach();
            /// 更新数据
            update();
        }
        FEMaterialPoint(const FEMaterialPoint& other)
            :FEMaterial(other)
        {}
        virtual ~FEMaterialPoint()
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
}

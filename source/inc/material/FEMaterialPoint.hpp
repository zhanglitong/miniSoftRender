#pragma     once
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPoint,"{5503589E-6D04-4C8B-8836-5ED302C1A623}");
    class   FE_API  FEMaterialPoint:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPoint)
    public:
        FEMaterialPoint(FEContext& ctx)
            :FEMaterialPoint(ctx,"point")
        {}
        /// <summary>
        /// 指定管线名称构造，供派生的点材质复用不同的颜色格式管线
        /// </summary>
        /// <param name="ctx">上下文</param>
        /// <param name="pipeline">管线名称，例如 point/point565</param>
        FEMaterialPoint(FEContext& ctx,const String& pipeline)
            :FEMaterial(ctx)
        {  
            setup(pipeline);
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
        /// <summary>
        /// 设置点大小的距离衰减系数: 点大小 = clamp(scale / 相机距离, min, max)，0 表示固定为 min
        /// </summary>
        void                setPointSizeScale(float scale)
        {
            _pushConst._sizeScale   =   scale;
        }

    protected:
        PointData   _pushConst;
    };
}

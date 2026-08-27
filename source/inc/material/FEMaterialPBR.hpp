#pragma     once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPBR,"{C253DF90-EA68-44D6-9558-68D244EE0066}");
    class   FEMaterialPBR:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPBR)
    public:
        FEMaterialPBR(FEContext& ctx)
            :FEMaterial(ctx)
            ,_pbr(ctx)
        {
            setup("pbr");
            _pbr.update();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            auto    lightSBO    =   ctx.device().queryCache(FEConstUuid::LightsId);

            auto&   lights      =   ctx.device().lights();
            bind(0,SB_Camera,   {cameraUBO});
            bind(0,SB_Light,    {lightSBO},{lights.count() * sizeof(LightData)});
            bind(0,SB_Material, {_pbr._gpu.get()});
            
            /// 灯光对象
            lightSBO->as<FENotify>()->addNotify(this,[this](Object object)
            {
                if (object->flags().hasFlag(FLAG_UPDATE))
                {
                    bind(0,SB_Light,{object});
                }
            });
            /// 同步数据到显卡
            update();
        }

        FEMaterialPBR(const FEMaterialPBR& other)
            :FEMaterial(other)
            ,_pbr(other._ctx)
        {
            _pbr._value   =   other._pbr._value;
            _pbr.update();
        }
        virtual ~FEMaterialPBR()
        {}
        /// <summary>
        /// 获取材质数据
        /// </summary>
        /// <returns></returns>
        inline  auto&   data() 
        {
            return  _pbr;
        }
        const   auto&   data() const
        {
            return  _pbr;
        }
    public:
        /// <summary>
        /// 动画系统独有接口
        /// 通用设置对象属性接口，子类实现
        /// </summary>
        virtual void        beginSetProp()
        {}
        /// <summary>
        /// 动画系统独有接口
        /// 设置属性
        /// </summary>
        /// <param name="prop">属性索引(别名)</param>
        /// <param name="value">属性值</param>
        /// <returns>true,表示修改成功,否则没有修改</returns>
        virtual bool        setProperty(int prop,const KFValue& value)
        {
            UNUSED(prop);
            UNUSED(value);
            return  false;
        }
        /// <summary>
        /// 动画系统独有接口
        /// @ref setProperty 返回结果作为输入参数，用来决定是否需要更新操作
        /// </summary>
        /// <param name="bModify"></param>
        virtual void        endSetProp(bool bModify)
        {
            UNUSED(bModify);
        }
    protected:
        TBlock<PBRData>  _pbr;
    };
}

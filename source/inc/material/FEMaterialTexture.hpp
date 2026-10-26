#pragma once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialTexture,"{9A21E4B7-3C58-4D0F-A1D2-6B7E90FC3548}");
    /// <summary>
    /// 纹理材质:漫反射贴图管线(texture/*),
    /// 贴图通过 combined image sampler 绑定到 SB_Material(4)
    /// </summary>
    class   FE_API  FEMaterialTexture:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEMaterialTexture)
    public:
        FEMaterialTexture(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("texture");
            /// <summary>
            /// 自动将内置 UBO(camera/全局 UBO)与 shader 反射绑定关联
            /// </summary>
            autoAttach();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bind(0,SB_Camera,   {cameraUBO});
            /// 天空环境(昼夜门控):与其他非 UI 材质共用同一份 SkyUBO
            bindSkyEnvironment();
            if (_texture)
                bind(0,SB_Material, {_texture.get()});

            /// <summary>
            /// 同步数据到显卡
            /// </summary>
            update();
        }

        FEMaterialTexture(const FEMaterialTexture& other)
            :FEMaterial(other)
        {
            _texture    =   other._texture;
            if (_texture)
            {
                bind(0,SB_Material, {_texture.get()});
                update();
            }
        }
        virtual ~FEMaterialTexture()
        {}
        /// <summary>
        /// 设置漫反射贴图(内部完成描述符绑定与上传)
        /// </summary>
        inline  void    setTexture(TexPtr texture)
        {
            _texture    =   texture;
            if (_texture)
            {
                bind(0,SB_Material, {_texture.get()});
                update();
            }
        }
        /// <summary>
        /// 获取漫反射贴图
        /// </summary>
        inline  const   TexPtr& texture() const
        {
            return  _texture;
        }
    protected:
        TexPtr      _texture    =   nullptr;
    };
}

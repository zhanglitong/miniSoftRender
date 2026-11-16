#pragma once

#include    "../graphic/FEConstUuid.h"
#include    "../graphic/FEDevice.h"
#include    "FEMaterial.hpp"


namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialCube,"{C4E9A1B2-7D3F-4E58-9A6B-2C1D0F8E7A45}");
    /// <summary>
    /// 立方体贴图材质:cubemap 采样管线(cube/*),
    /// cubemap 通过 combined image sampler(samplerCube)绑定到 SB_Material(4)
    /// </summary>
    class   FE_API  FEMaterialCube:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEMaterialCube)
    public:
        FEMaterialCube(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("cube");
            /// <summary>
            /// 自动将内置 UBO(camera/全局 UBO)与 shader 反射绑定关联
            /// </summary>
            autoAttach();

            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bind(0,SB_Camera,   {cameraUBO});
            if (_cube)
                bindTexture(0,SB_Material, _cube);

            /// <summary>
            /// 同步数据到显卡
            /// </summary>
            update();
        }

        FEMaterialCube(const FEMaterialCube& other)
            :FEMaterial(other)
        {
            _cube   =   other._cube;
            if (_cube)
            {
                bindTexture(0,SB_Material, _cube);
                update();
            }
        }
        virtual ~FEMaterialCube()
        {
            /// 先退订纹理通知,再释放 cubemap 引用
            unwatchTexture(_cube.get());
            /// 成员析构发生在本体之后,先持锁释放 cubemap 引用
            FEMutex::ScopeLock  lock(_ctx.device().gpuMutex());
            _cube   =   nullptr;
        }
        /// <summary>
        /// 设置立方体贴图(内部完成描述符绑定与上传)
        /// </summary>
        inline  void    setCube(TexPtr cube)
        {
            TexPtr  old     =   _cube;
            _cube   =   cube;
            if (old.get() != _cube.get())
                unwatchTexture(old.get());
            if (_cube)
            {
                bindTexture(0,SB_Material, _cube);
                update();
            }
        }
        /// <summary>
        /// 获取立方体贴图
        /// </summary>
        inline  const   TexPtr& cube() const
        {
            return  _cube;
        }
    protected:
        TexPtr      _cube   =   nullptr;
    };
}

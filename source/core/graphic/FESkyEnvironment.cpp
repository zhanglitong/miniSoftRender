#include    "../inc/graphic/FESkyEnvironment.h"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/material/FEMaterialSky.hpp"
#include    "../inc/geometry/FEGeometryBox.hpp"
#include    "../inc/FELog.hpp"

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 标准 cubemap 面朝向,与 FEReflectionProbe 保持一致:
        /// 面索引 +X,-X,+Y,-Y,+Z,-Z 对应 Vulkan/GL 的 layer 0~5
        /// </summary>
        struct  SkyFace
        {
            real3   _forward;
            real3   _up;
        };
        const   SkyFace kSkyFaces[6]    =
        {
            { real3(real(1),real(0),real(0)),  real3(real(0),real(-1),real(0)) },
            { real3(real(-1),real(0),real(0)), real3(real(0),real(-1),real(0)) },
            { real3(real(0),real(1),real(0)),  real3(real(0),real(0),real(1))  },
            { real3(real(0),real(-1),real(0)), real3(real(0),real(0),real(-1)) },
            { real3(real(0),real(0),real(1)),  real3(real(0),real(-1),real(0)) },
            { real3(real(0),real(0),real(-1)), real3(real(0),real(-1),real(0)) },
        };
    }

    FESkyEnvironment::~FESkyEnvironment()
    {
        destroy();
    }

    bool    FESkyEnvironment::setup(FEScene& scene,uint size)
    {
        if (size == 0)
            return  false;
        destroy();

        _size   =   size;
        /// 完整 mip 链:128 → 1 共 8 级;PBR 用 roughness 选级做镜面预滤波,
        /// 漫反射取最高几级近似余弦卷积
        _mips   =   1;
        while ((size >> _mips) > 0)
            ++_mips;

        auto&   device  =   _ctx.device();

        /// 1. cubemap 颜色目标(HDR,color attachment + sampled 双用途)
        {
            _cube   =   device.createTexture();
            FETexture::CreateInfo   ci;
            ci._width           =   size;
            ci._height          =   size;
            ci._format          =   FMT_R16G16B16A16_FLOAT;
            ci._magFilter       =   FILTER_LINEAR;
            ci._minFilter       =   FILTER_LINEAR;
            /// cubemap 的 mip 是逐级单独渲染的,不跨面混合,可安全使用 CLAMP
            ci._addressMode     =   ADDRESS_CLAMP_TO_EDGE;
            ci._mips            =   _mips;
            ci._cube            =   true;
            ci._renderTarget    =   true;
            if (_cube == nullptr || !_cube->create(ci) || _cube->imageView() == nullptr)
            {
                LOG_ERR("FESkyEnvironment.setup: create cube target failed");
                destroy();
                return  false;
            }
        }

        /// 2. 每级 mip、每个面各建一张 2D 视图(作为逐面渲染的 color attachment)
        _faceViews.resize(_mips);
        for (uint mip = 0; mip < _mips; ++mip)
        {
            _faceViews[mip].reserve(6);
            for (uint face = 0; face < 6; ++face)
            {
                GImgView    view    =   _cube->image()->createView(mip,1,face,1);
                if (view == nullptr)
                {
                    LOG_ERR("FESkyEnvironment.setup: create face view(mip %u face %u) failed",mip,face);
                    destroy();
                    return  false;
                }
                _faceViews[mip].push_back(view);
            }
        }

        /// 3. 6 面共用一张深度目标(取 mip0 尺寸,低级别 mip 用更小的 renderArea 即可)
        _depthImage =   device.createGImage();
        {
            FEGImage::CreateInfo    info    =   {};
            info._width     =   size;
            info._height    =   size;
            info._depth     =   1;
            info._usage     =   USAGE_DEPTH_STENCIL_ATTACHMENT;
            info._type      =   IT_2D;
            info._format    =   FMT_D32_S8_UNORM;
            info._aspect    =   ASPECT_DEPTH_BIT;
            info._layout    =   IL_UNDEFINED;
            info._samples   =   1;
            if (!_depthImage->create(info))
            {
                LOG_ERR("FESkyEnvironment.setup: create depth target failed");
                destroy();
                return  false;
            }
            _depthView  =   _depthImage->createView();
        }

        /// 4. 6 个 90° 面相机:天空盒以相机为中心,相机固定放在原点,
        ///    逐面朝 +X/-X/... 方向看即可得到完整的立方体贴图
        _cameras.clear();
        const   real3   origin  =   real3(real(0));
        for (uint face = 0; face < 6; ++face)
        {
            Camera  cam =   new FECamera(_ctx,origin + kSkyFaces[face]._forward,origin,real3(real(1),real(0),real(0)));
            cam->setUp(kSkyFaces[face]._up);
            cam->setViewSize(real2(real(size),real(size)));
            cam->setFov(DEG2RAD(90));
            cam->setNearFar(real2(real(0.1),real(1000)));
            cam->updatePerspective();
            cam->update();
            _cameras.push_back(cam);
        }

        /// 5. 烘焙专用节点与渲染工厂:
        ///    节点不加入场景,渲染工厂只用于本类的逐面绘制,避免污染主场景的工厂列表
        {
            FEGeometryBox   geo(_ctx);
            geo.param()._size   =   float3(2.0f, 2.0f, 2.0f);
            geo.param()._segs   =   uint3(1, 1, 1);
            Mesh    mesh    =   geo.triangular({
                {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

            _material   =   new FEMaterialSky(_ctx,"skyenv");
            _node       =   new FENode(_ctx);
            _node->setMesh(mesh);
            _node->setMaterial(_material);
            _node->makeDirty();
            _node->update();

            RFactorys   factorys    =   FEFactoryRender::addNodesToFactory(_ctx,scene,{_node},false);
            if (factorys.empty())
            {
                LOG_ERR("FESkyEnvironment.setup: create bake factory failed");
                destroy();
                return  false;
            }
            _factory    =   factorys.front();
            /// 烘焙只有天空盒自身的 12 个三角形,不需要 GPU 裁剪(否则要预先准备裁剪数据)
            _factory->setGPUCull(false);
        }
        return  true;
    }

    void    FESkyEnvironment::destroy()
    {
        if (_factory != nullptr)
        {
            _factory->destroy();
            _factory    =   nullptr;
        }
        _material   =   nullptr;
        _node       =   nullptr;
        _cameras.clear();
        _faceViews.clear();
        _depthView  =   nullptr;
        _depthImage =   nullptr;
        _cube       =   nullptr;
        _mips       =   0;
        _size       =   0;
    }

    bool    FESkyEnvironment::bake(FEScene& scene,CMDPtr cmd)
    {
        if (!isValid() || cmd == nullptr)
            return  false;

        /// 实例/间接绘制缓冲需要先更新(本工厂不在场景的工厂列表中,不会被自动更新)
        _factory->update(cmd);

        for (uint face = 0; face < 6; ++face)
        {
            /// 每面把该面相机写入全局相机 UBO 一次即可覆盖整条 mip 链。
            /// 天空顶点着色器只用视图矩阵的旋转部分(outDir=inPos),相机固定在原点
            scene.writeCameraData(cmd,_cameras[face]);

            for (uint mip = 0; mip < _mips; ++mip)
            {
                const   uint    dim =   _size >> mip;
                if (dim == 0)
                    break;

                FECmdBuffer::RenderInfo rs  =   {};
                rs._depth       =   _depthView;
                rs._colors      =   {_faceViews[mip][face]};
                rs._clearColor  =   float4(0,0,0,1);
                rs._rect.set(0,0,dim,dim);

                cmd->beginRender(rs);

                /// 天空管线使用动态视口/裁剪矩形,按当前 mip 尺寸设置
                const   FECmdBuffer::Viewport    vp  =   {0.0f,0.0f,float(dim),float(dim),0.0f,1.0f};
                const   RectU32                 scissor(0,0,dim,dim);
                cmd->setViewport(0,1,&vp);
                cmd->setScissor(0,1,&scissor);

                _factory->render(cmd);
                cmd->endRender(rs);
            }
        }
        return  true;
    }
}

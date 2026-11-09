
#include    "../inc/graphic/FEReflectionProbe.h"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/graphic/FEFactoryMgr.hpp"
#include    "../inc/FELog.hpp"

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 标准 cubemap 面朝向(面索引与 Vulkan/GL 一致:+X,-X,+Y,-Y,+Z,-Z),
        /// up 向量按同一约定给出,保证 6 面拼接方向一致
        /// </summary>
        struct  ProbeFace
        {
            real3   _forward;
            real3   _up;
        };
        const   ProbeFace   kProbeFaces[6]  =
        {
            { real3(real(1),real(0),real(0)),  real3(real(0),real(-1),real(0)) },
            { real3(real(-1),real(0),real(0)), real3(real(0),real(-1),real(0)) },
            { real3(real(0),real(1),real(0)),  real3(real(0),real(0),real(1))  },
            { real3(real(0),real(-1),real(0)), real3(real(0),real(0),real(-1)) },
            { real3(real(0),real(0),real(1)),  real3(real(0),real(-1),real(0)) },
            { real3(real(0),real(0),real(-1)), real3(real(0),real(-1),real(0)) },
        };
    }

    FEReflectionProbe::~FEReflectionProbe()
    {
        destroy();
    }

    bool    FEReflectionProbe::setup(const Node& owner,FEFormat format,uint size)
    {
        if (size == 0)
            return  false;
        destroy();

        _owner  =   owner;
        _size   =   size;
        auto&   device  =   _ctx.device();

        /// 1. cubemap 颜色目标(color attachment + sampled 双用途)
        {
            _cube   =   device.createTexture();
            FETexture::CreateInfo   ci;
            ci._width           =   size;
            ci._height          =   size;
            ci._format          =   format;
            ci._magFilter       =   FILTER_LINEAR;
            ci._minFilter       =   FILTER_LINEAR;
            ci._addressMode     =   ADDRESS_CLAMP_TO_EDGE;
            ci._mips            =   1;
            ci._cube            =   true;
            ci._renderTarget    =   true;
            if (_cube == nullptr || !_cube->create(ci) || _cube->imageView() == nullptr)
            {
                LOG_ERR("FEReflectionProbe.setup: create cube target failed");
                destroy();
                return  false;
            }
        }

        /// 2. 6 个单面 2D 视图(作为逐面渲染的 color attachment)
        _faceViews.clear();
        for (uint face = 0; face < 6; ++face)
        {
            GImgView    view    =   _cube->image()->createView(0,1,face,1);
            if (view == nullptr)
            {
                LOG_ERR("FEReflectionProbe.setup: create face view %u failed",face);
                destroy();
                return  false;
            }
            _faceViews.push_back(view);
        }

        /// 3. 6 面共用一张深度目标(逐面顺序渲染,无需每面一份)
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
                LOG_ERR("FEReflectionProbe.setup: create depth target failed");
                destroy();
                return  false;
            }
            _depthView  =   _depthImage->createView();
        }

        /// 4. 探针位置:owner 的世界坐标(球心);无 owner 时退化为原点
        const   real3   origin  =   (_owner != nullptr) ? _owner->globalTranslation() : real3(real(0));

        /// 近裁剪面取 owner 包围盒外接半径:相机位于球心,球体表面在半径处,
        /// 用大于半径的 near 把 owner 自身整体裁掉(不能按工厂隐藏——
        /// 同材质的其他对象与该球共用同一个渲染工厂,隐藏会一并消失)
        real    nearPlane   =   real(0.01);
        if (_owner != nullptr)
        {
            /// 节点的 _aabb 只有在 updateAabb 之后才反映真实网格,
            /// 否则仍是默认的 ±0.5,近裁剪面会小于球半径,球自身内表面
            /// 会把整个立方体贴图铺满(反射里看不到任何外部对象)
            if (Mesh mesh = _owner->mesh())
                mesh->updateAabb();
            _owner->updateAabb(false);

            const   aabb3dr box =   _owner->globalAabb();
            if (box._extent == aabb3dr::EXTENT_FINITE)
            {
                const   real3   half    =   (box._maximum - box._minimum) * real(0.5);
                const   real    radius  =   (std::max)(half.x,(std::max)(half.y,half.z));
                if (radius > nearPlane)
                    nearPlane   =   radius * real(1.02);
            }
        }

        /// 5. 6 个 90° 面相机
        _cameras.clear();
        for (uint face = 0; face < 6; ++face)
        {
            /// FECamera 构造签名为 (ctx,target,eye,right):眼睛在探针中心,
            /// 目标点沿该面朝向,保证相机朝 +forward 方向看
            Camera  cam =   new FECamera(_ctx,origin + kProbeFaces[face]._forward,origin,real3(real(1),real(0),real(0)));
            cam->setUp(kProbeFaces[face]._up);
            cam->setViewSize(real2(real(size),real(size)));
            cam->setFov(DEG2RAD(90));
            /// near 取 owner 外接半径的 1.02 倍:相机在球心,
            /// 该 near 恰好把 owner 球面整体裁掉,避免自反射
            cam->setNearFar(real2(nearPlane,real(1000)));
            cam->updatePerspective();
            /// 相机不与场景更新队列绑定,必须显式刷新视图矩阵
            cam->update();
            _cameras.push_back(cam);
        }

        /// 6. 探针专用 viewer:渲染场景全部工厂(天空同属 USAGE_Scene),
        /// 渲染尺寸用 setViewSize 覆盖成单面边长
        _viewer =   new FEViewer(_ctx,nullptr,nullptr, FEViewer::ViewerUsages(uint32_t(USAGE_Scene)));
        _viewer->setViewSize(size,size);
        return  true;
    }

    void    FEReflectionProbe::destroy()
    {
        _cameras.clear();
        _viewer     =   nullptr;
        _faceViews.clear();
        _depthView  =   nullptr;
        _depthImage =   nullptr;
        _cube       =   nullptr;
        _size       =   0;
        _owner      =   nullptr;
    }

    bool    FEReflectionProbe::capture(FEScene& scene,CMDPtr cmd)
    {
        if (!isValid() || cmd == nullptr)
            return  false;

        /// 探针相机与主相机视锥不同,主相机的 GPU 裁剪结果(上一帧)
        /// 会让主视角之外的对象(如相机背后的物体)缺席反射,
        /// 因此捕获期间临时关闭 GPU 裁剪,渲染全部实例。
        /// owner 自身由近裁剪面剔除(见 setup),不按工厂隐藏
        RFactorys   factories   =   scene.factoryMgr().objects();
        std::vector<std::pair<RFactory,bool>>   cullRestore;
        for (auto& var : factories)
        {
            const bool  oldCull =   var->supportGPUCull();
            if (oldCull)
            {
                var->setGPUCull(false);
                cullRestore.push_back({var,oldCull});
            }
        }

        for (uint face = 0; face < 6; ++face)
        {
            /// 每面渲染前把该面相机写入全局相机 UBO
            scene.writeCameraData(cmd,_cameras[face]);

            FECmdBuffer::RenderInfo rs  =   {};
            rs._depth       =   _depthView;
            rs._colors      =   {_faceViews[face]};
            rs._clearColor  =   float4(0,0,0,1);
            rs._rect.set(0,0,_size,_size);

            cmd->beginRender(rs);
            FEFramInfo  info    =   {0,cmd};
            _viewer->onMessage(MsgRender(info));
            cmd->endRender(rs);
        }

        /// 恢复 GPU 裁剪开关(捕获只影响本次录制)
        for (auto& item : cullRestore)
            item.first->setGPUCull(item.second);

        return  true;
    }
}

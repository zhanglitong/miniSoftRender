/// sample.animation
/// 动画关键帧示例:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometryBox 生成立方体 mesh
///   3. 为立方体节点添加 FEAnimation 组件,clip 中只含一条 Position 轨道
///      共两个关键帧: t=0 -> (0,0,0); t=2s -> (10,0,0),线性插值,循环播放
/// 用法: sample.animation.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId = RS_WEBGPU;
        }
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
        {
            rendererId = RS_VULKAN;
        }
    }

    FEContext&  ctx =   FEContext::instance();

    /// 场景指针在引擎创建后赋值;notify 把窗口消息(更新/渲染/缩放/输入)转发给场景
    FEScene*    pScene  =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    /// 2. 创建引擎并初始化渲染后端
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 用 FEGeometryBox 生成立方体 mesh
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setMesh(mesh);

    /// 明亮的漫反射与自发光,使立方体在默认平行光下清晰可见
    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
    pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
    pMat->data().update();
    node->setMaterial(pMat);

    /// 4. 创建动画:只含一条 Position(real3)轨道,并添加两个关键帧
    ///    t=0s -> (0,0,0); t=2s -> (10,0,0),轨道默认线性插值
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(ctx, NodeProperyBits(NP_TRANSFORM_XYZ));
    anim->setName("BoxMove");
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0f, KFValue(real3(0.0, 0.0, 0.0)));
    clip->addKeyFrame(2.0f, KFValue(real3(10.0, 0.0, 0.0)));

    /// 动画作为组件挂到节点上,节点变换混合时以动画变换为准(权重 1.0)
    node->addComponent(anim.get());
    node->update();

    /// 5. 分发到渲染工厂与动画系统,然后启动默认 Action 播放(默认 PT_Loop 循环)
    scene->dispatchNodesToSystem({node});
    scene->animationSystem()->getOrCreate()->play();

    std::cout << "animation running: box (0,0,0) -> (10,0,0), entering render loop..." << std::endl;

    /// 6. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

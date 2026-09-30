/// sample.constraint
/// 平移约束(FEConstraintTrans)示例:
///   1. 创建引擎 + 渲染窗口
///   2. 目标 box(红色)：FEAnimation Position 轨道，两个关键帧
///      t=0 -> (0,0,0); t=2s -> (1.5,0,0)，线性插值，循环播放
///   3. 跟随 box(蓝色)：初始位于 (0,0.75,0)，添加 FEConstraintTrans
///      并绑定目标节点；maintainOffset 默认开启，附加时自动记录偏移 (0,0.75,0)，
///      因此跟随 box 始终保持在目标 box 上方 0.75 个单位
/// 用法: sample.constraint.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/constraint/FEConstraintTrans.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 用一个 box 几何 + 指定材质颜色创建节点
static  Node    createBoxNode(FEContext& ctx, real boxSize, const float4& diffuse, const float4& emissive)
{
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(boxSize, boxSize, boxSize);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setMesh(mesh);

    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   diffuse;
    pMat->data()._value._emissive =   emissive;
    pMat->data().update();
    node->setMaterial(pMat);
    return  node;
}

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

    /// 3. 目标 box(红色)：(0,0,0) -> (1.5,0,0) 循环移动
    ///    (相机在 2.5 距离、45 度视场下视野有限，移动范围控制在可见区域内)
    Node    targetNode  =   createBoxNode(ctx, 0.25f,
        float4(0.95f, 0.25f, 0.15f, 1.0f),
        float4(0.65f, 0.12f, 0.08f, 1.0f));

    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(ctx, NodeProperyBits(NP_TRANSFORM_XYZ));
    anim->setName("TargetMove");
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0f, KFValue(real3(0.0, 0.0, 0.0)));
    clip->addKeyFrame(2.0f, KFValue(real3(1.5, 0.0, 0.0)));
    targetNode->addComponent(anim.get());
    targetNode->update();

    /// 4. 跟随 box(蓝色)：初始位于目标上方 0.75 个单位
    Node    followNode  =   createBoxNode(ctx, 0.25f,
        float4(0.15f, 0.55f, 0.95f, 1.0f),
        float4(0.10f, 0.35f, 0.65f, 1.0f));
    followNode->setLocalTranslation(real3(0.0f, 0.75f, 0.0f));
    followNode->update();

    /// 5. 创建平移约束并绑定目标节点；maintainOffset 默认开启，
    ///    addComponent -> attach 时自动记录偏移 (0,0.75,0)，激活瞬间不发生跳变
    ConstraintTrans constraint  =   new FEConstraintTrans(ctx);
    constraint->setTargetNode(targetNode);
    followNode->addComponent(constraint.get());
    followNode->update();

    /// 6. 两个节点分发到渲染工厂与各组件系统,然后启动默认 Action 播放
    scene->dispatchNodesToSystem({targetNode, followNode});
    scene->animationSystem()->getOrCreate()->play();
    scene->nodeTree().addToplevelNodes({targetNode, followNode});

    std::cout << "constraint running: blue box follows red box with offset (0,2,0)..." << std::endl;

    /// 7. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

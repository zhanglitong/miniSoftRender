/// test.constraint.visual
/// 可视化约束测试: 动画驱动的目标节点 + 四类约束同时运行
///   1. leader  : 由动画驱动位置,沿 X 轴往返(-2.4,1.4,0) -> (2.4,1.4,0),循环播放
///   2. follower: Follow 约束,按附加时记录的偏移跟随 leader(橙色方块上方的青色方块)
///   3. trans   : Trans  约束,targetNode=leader,附加偏移(0,1.2,0),始终悬在 leader 正上方
///   4. lookAt  : LookAt 约束,细长方块(本地 +Z 朝前)始终指向 leader
///   5. path    : Path  约束,绿色方块沿矩形路径循环移动,路径点用灰色小方块标记
/// 断言: 动画采样 -> 约束求解同帧生效(约束系统在动画系统之后更新)
/// 用法: test.constraint.visual.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <cmath>
#include    <chrono>
#include    <thread>
#include    <vector>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/animation/FEPlayback.hpp"
#include    "../../inc/constraint/FEConstraintFollow.hpp"
#include    "../../inc/constraint/FEConstraintLookAt.hpp"
#include    "../../inc/constraint/FEConstraintPath.hpp"
#include    "../../inc/constraint/FEConstraintTrans.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

static  int     g_failed    =   0;

static  void    checkVec(const char* name, const real3& actual, const real3& expect, real eps = 2e-2f)
{
    real    dist    =   real(glm::length(actual - expect));
    std::cout << (dist <= eps ? "[PASS] " : "[FAIL] ") << name
              << "  actual(" << actual.x << ", " << actual.y << ", " << actual.z << ")"
              << "  expect(" << expect.x << ", " << expect.y << ", " << expect.z << ")" << std::endl;
    if (dist > eps)
        ++g_failed;
}

static  void    checkDir(const char* name, const real3& dir, const real3& expect, real eps = 2e-2f)
{
    real    dot =   real(glm::dot(glm::normalize(dir), glm::normalize(expect)));
    std::cout << (dot >= real(1) - eps ? "[PASS] " : "[FAIL] ") << name << "  dot=" << dot << std::endl;
    if (dot < real(1) - eps)
        ++g_failed;
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        if (_stricmp(argv[i], "--webgpu") == 0 || _stricmp(argv[i], "-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(argv[i], "--vulkan") == 0 || _stricmp(argv[i], "-vulkan") == 0)
            rendererId  =   RS_VULKAN;
    }

    FEContext&  ctx     =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

    /// ==================== 1. 创建窗口与场景 ====================
    FEApp::CreateInfo   info    =   {};
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

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// ==================== 2. 共享网格与节点工厂 ====================
    FEGeometryBox   geo(ctx);
    geo.param()._size   =   float3(1.0f, 1.0f, 1.0f);
    geo.param()._segs   =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    auto    createNodeS =   [&](const real3& pos, const real3& scale, const float4& color) -> Node
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);

        FEMaterialPBR*  mat =   new FEMaterialPBR(ctx);
        mat->data()._value._diffuse     =   color;
        mat->data()._value._emissive    =   float4(color.x * 0.35f, color.y * 0.35f, color.z * 0.35f, 1.0f);
        mat->data().update();
        node->setMaterial(mat);

        node->setLocalTranslation(pos);
        node->setLocalScaling(scale);
        node->makeDirty();
        node->update();
        return  node;
    };
    auto    createNode  =   [&](const real3& pos, real scale, const float4& color) -> Node
    {
        return  createNodeS(pos, real3(scale), color);
    };

    /// ==================== 3. 节点 ====================
    /// 动画目标(橙色)
    Node    leader      =   createNode(real3(-2.4, 1.4, 0.0), 0.6, float4(1.00f, 0.55f, 0.15f, 1.0f));
    /// Follow(青色): 初始位于 leader 上方偏右,附加时记录偏移
    Node    follower    =   createNode(real3(0.0, 2.6, 0.0), 0.45, float4(0.10f, 0.80f, 0.95f, 1.0f));
    /// Trans(紫色): 初始位于 leader 正上方
    Node    transNode   =   createNode(real3(-2.4, 2.6, 0.0), 0.4, float4(0.75f, 0.35f, 0.95f, 1.0f));
    /// LookAt(黄色): 细长方块,本地 +Z 为长边,用来观察朝向
    Node    lookNode    =   createNodeS(real3(3.2, 1.4, 3.2), real3(0.12, 0.12, 0.9), float4(0.95f, 0.85f, 0.15f, 1.0f));
    /// Path(绿色): 沿矩形路径循环移动
    Node    pathNode    =   createNode(real3(0.0, 0.4, 0.0), 0.35, float4(0.20f, 0.90f, 0.35f, 1.0f));

    /// 路径点标记(灰色小方块)
    std::vector<real3>  pathPoints  =
    {
        real3(-3.0, 0.4, -2.0), real3(3.0, 0.4, -2.0),
        real3( 3.0, 0.4,  2.0), real3(-3.0, 0.4, 2.0)
    };
    Nodes   allNodes    =   { leader, follower, transNode, lookNode, pathNode };
    for (auto& p : pathPoints)
        allNodes.push_back(createNode(p, 0.12, float4(0.60f, 0.60f, 0.65f, 1.0f)));

    /// ==================== 4. 约束 ====================
    ConstraintFollow    follow  =   new FEConstraintFollow(ctx);
    follow->setTargetNode(leader);
    follower->addComponent(Component(follow.get()));

    ConstraintTrans     trans   =   new FEConstraintTrans(ctx);
    trans->setTargetNode(leader);
    transNode->addComponent(Component(trans.get()));

    ConstraintLookAt    look    =   new FEConstraintLookAt(ctx);
    look->setTargetNode(leader);
    lookNode->addComponent(Component(look.get()));

    ConstraintPath      path    =   new FEConstraintPath(ctx);
    path->setPath(pathPoints);
    path->setSpeed(1.8);
    path->setLoop(true);
    pathNode->addComponent(Component(path.get()));

    /// 固定取景: 俯视前方,保证全部约束节点都在视野内
    if (auto    cam =   scene->camera())
    {
        cam->setEye(real3(0.0, 5.0, 10.0));
        cam->setTarget(real3(0.0, 1.0, 0.0));
        cam->setViewSize(real2(1280.0, 720.0));
        cam->update();
    }

    /// ==================== 5. 动画驱动 leader ====================
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(ctx, NP_TRANSFORM_XYZ);
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0, real3(-2.4, 1.4, 0.0));
    clip->addKeyFrame(2.5, real3( 2.4, 1.4, 0.0));
    leader->addComponent(Component(anim.get()));

    /// 必须在挂载组件之后分发,系统才会收集动画/约束组件
    scene->dispatchNodesToSystem(allNodes);

    /// ==================== 6. 同帧联动断言 ====================
    /// 动画系统(排序靠前)采样 -> 刷新 leader 全局变换
    /// 约束系统(排序靠后)读取该结果求解 -> 刷新约束节点
    if (auto  sys =   scene->animationSystem())
    {
        auto    action  =   sys->getOrCreate();
        action->setMode(FEPlayback::PT_Loop);
        action->play();
    }

    for (int i = 0; i < 4; ++i)
    {
        /// 拉开帧间隔,让动画时间推进,便于验证约束确实读到动画的最新采样
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        scene->onFrameStart();
        scene->onFrameUpdate();
        scene->onFrameRender();
        scene->onFrameEnd();
    }

    real3   leaderPos   =   leader->globalTranslation();
    std::cout << "leader pos=[" << leaderPos.x << ", " << leaderPos.y << ", " << leaderPos.z << "]" << std::endl;

    checkVec("Follow 与动画目标保持附加偏移", follower->globalTranslation(), leaderPos + real3(2.4, 1.2, 0.0));
    checkVec("Trans  悬停在动画目标上方",    transNode->globalTranslation(), leaderPos + real3(0.0, 1.2, 0.0));
    {
        /// 朝向断言: 本地 +Z 旋转后应与目标方向一致
        real3   forward =   quatr(lookNode->globalFETransform().rotation()) * real3(0.0, 0.0, 1.0);
        checkDir("LookAt 本地 +Z 对准目标", forward, leaderPos - lookNode->globalTranslation());
    }
    {
        real3   p   =   pathNode->globalTranslation();
        bool    onPath  =   std::abs(p.y - 0.4f) <= 2e-2f
                        &&  p.x >= -3.05f && p.x <= 3.05f
                        &&  p.z >= -2.05f && p.z <= 2.05f;
        std::cout << (onPath ? "[PASS] " : "[FAIL] ") << "Path  在矩形路径上  pos("
                  << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
        if (!onPath)
            ++g_failed;
    }

    if (g_failed == 0)
        std::cout << "test.constraint.visual PASSED" << std::endl;
    else
        std::cout << "test.constraint.visual FAILED, " << g_failed << " case(s)" << std::endl;

    /// ==================== 7. 进入渲染循环持续观察 ====================
    app->run();

    scene   =   nullptr;
    app->destroy();
    return  (g_failed == 0) ? 0 : 1;
}

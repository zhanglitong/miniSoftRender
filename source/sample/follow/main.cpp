/// sample.follow
/// 跟随约束(FEConstraintFollow) + 路径约束(FEConstraintPath) 示例:
///   1. 创建引擎 + 渲染窗口
///   2. Leader(红色立方体, 原点): NP_ROTATE_Y 动画, 0° -> 360° 持续自旋
///      Leader 带一个黄色 "nose" 子节点(+Z 方向小方块),用于目视旋转方向
///   3. Follower(蓝色立方体): 同节点挂两个约束
///      - FEConstraintFollow(EP_Second 先执行): 绑定 Leader,完整跟随其旋转
///      - FEConstraintPath  (EP_Last   后执行): 沿菱形路径移动,覆盖位置
///      Follower 带一个青色 "nose" 子节点,用于目视旋转方向
///   4. 执行顺序验证:
///      Follow 先执行 -> 写入旋转 = Leader 旋转
///      Path  后执行 -> 覆盖位置为路径点,保留 Follow 写入的旋转
///      最终: Follower 沿菱形路径移动, nose 始终与 Leader 的 nose 同向
///   5. 路径点标记(绿色小方块)标示菱形顶点
/// 用法: sample.follow.exe [--vulkan|--webgpu] [--msaa N]

#include    <iostream>
#include    <cstdlib>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/constraint/FEConstraintFollow.hpp"
#include    "../../inc/constraint/FEConstraintPath.hpp"

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
    uint    msaa        =   1;   ///< 多采样数(MSAA),默认 1x
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
        else if (_stricmp(arg, "--msaa") == 0 || _stricmp(arg, "-msaa") == 0 ||
                 _stricmp(arg, "/msaa") == 0)
        {
            if (i + 1 < argc)
                msaa    =   (uint)atoi(argv[++i]);
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
    /// 多采样数必须在 setup 之前设置,setup 内会按设备能力钳制
    scene->setSampleCount(msaa);
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 2.5 相机: 拉远以便完整观察 Leader 与 Follower 的路径
    scene->camera()->setEye(real3(0.0, 0.0, -6.0));
    scene->camera()->setTarget(real3(0.0, 0.0, 0.0));

    /// 3. Leader(红色立方体, 原点): NP_ROTATE_Y 动画持续自旋
    Node    leaderNode   =   createBoxNode(ctx, 0.30f,
        float4(0.95f, 0.25f, 0.15f, 1.0f),
        float4(0.65f, 0.12f, 0.08f, 1.0f));
    leaderNode->setLocalTranslation(real3(0.0f, 0.0f, 0.0f));

    ///    旋转动画: 0° -> 360°, 3 秒一周,默认 PT_Loop 持续循环
    Animation   leaderAnim  =   FEAnimationHelper::createNodeAnimtion(ctx, NodeProperyBits(NP_ROTATE_Y));
    leaderAnim->setName("LeaderSpin");
    AnimClip    leaderClip  =   leaderAnim->clip();
    leaderClip->addKeyFrame(0.0f, KFValue(real(0.0)));
    leaderClip->addKeyFrame(3.0f, KFValue(real(360.0)));
    leaderNode->addComponent(leaderAnim.get());

    ///    Leader 的 "nose": 黄色小方块,放 local +Z 方向,旋转时绕 Leader 中心公转
    Node    leaderNose  =   createBoxNode(ctx, 0.10f,
        float4(0.95f, 0.85f, 0.20f, 1.0f),
        float4(0.65f, 0.55f, 0.10f, 1.0f));
    leaderNose->setLocalTranslation(real3(0.0f, 0.0f, 0.25f));
    leaderNode->addChild(leaderNose);
    leaderNode->update();

    /// 4. Follower(蓝色立方体): 同时挂 Follow 约束 + Path 约束
    ///    初始位置设在路径起点(右顶点),attach 时自动记录偏移
    Node    followerNode    =   createBoxNode(ctx, 0.30f,
        float4(0.15f, 0.55f, 0.95f, 1.0f),
        float4(0.10f, 0.35f, 0.65f, 1.0f));
    followerNode->setLocalTranslation(real3(1.6f, 0.0f, 0.0f));

    ///    Follower 的 "nose": 青色小方块,放 local +Z 方向
    ///    由于 Follower 旋转跟随 Leader, nose 方向始终与 Leader 的 nose 同向
    Node    followerNose  =   createBoxNode(ctx, 0.10f,
        float4(0.20f, 0.85f, 0.95f, 1.0f),
        float4(0.10f, 0.55f, 0.65f, 1.0f));
    followerNose->setLocalTranslation(real3(0.0f, 0.0f, 0.25f));
    followerNode->addChild(followerNose);
    followerNode->update();

    /// 4.1 Follow 约束(EP_Second 先执行): 绑定 Leader
    ///     maintainOffset 默认开启, attach 时记录 Leader/Follower 当前相对偏移
    ConstraintFollow   follow  =   new FEConstraintFollow(ctx);
    follow->setTargetNode(leaderNode);
    followerNode->addComponent(follow.get());
    followerNode->update();

    /// 4.2 Path 约束(EP_Last 后执行): 沿菱形路径移动,覆盖 Follow 写入的位置
    ///     菱形路径(世界坐标,z=0 平面): 右 -> 上 -> 左 -> 下 -> 回到右
    ConstraintPath  path    =   new FEConstraintPath(ctx);
    path->addPoint(real3( 1.6f,  0.0f, 0.0f));
    path->addPoint(real3( 0.0f,  1.6f, 0.0f));
    path->addPoint(real3(-1.6f,  0.0f, 0.0f));
    path->addPoint(real3( 0.0f, -1.6f, 0.0f));
    path->addPoint(real3( 1.6f,  0.0f, 0.0f));
    path->setSpeed(2.0f);     /// 2 单位/秒
    path->setLoop(true);
    followerNode->addComponent(path.get());
    followerNode->update();

    /// 5. 路径点标记(绿色小方块)标示菱形顶点
    Nodes   markerNodes;
    {
        const real  markerSize  =   0.12f;
        const real3 points[]  =   {
            real3( 1.6f,  0.0f, 0.0f),
            real3( 0.0f,  1.6f, 0.0f),
            real3(-1.6f,  0.0f, 0.0f),
            real3( 0.0f, -1.6f, 0.0f),
        };
        for (const auto& p : points)
        {
            Node    marker  =   createBoxNode(ctx, markerSize,
                float4(0.30f, 0.85f, 0.35f, 1.0f),
                float4(0.15f, 0.45f, 0.18f, 1.0f));
            marker->setLocalTranslation(p);
            marker->update();
            markerNodes.push_back(marker);
        }
    }

    /// 6. 节点分发到渲染工厂与各组件系统,然后启动默认 Playback 播放
    ///    只传顶层节点: 工厂与组件系统都会递归子节点
    Nodes   allNodes    =   markerNodes;
    allNodes.push_back(leaderNode);
    allNodes.push_back(followerNode);
    scene->dispatchNodesToSystem(allNodes);
    scene->animationSystem()->getOrCreate()->play();

    std::cout << "follow constraint running:" << std::endl;
    std::cout << "  - Leader (red box + yellow nose) spins at origin" << std::endl;
    std::cout << "  - Follower (blue box + cyan nose) moves along the diamond path" << std::endl;
    std::cout << "  - Follower's nose always points same direction as Leader's nose" << std::endl;
    std::cout << "  - Follow (EP_Second) sets rotation, Path (EP_Last) overrides position" << std::endl;

    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的节点
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        allNodes.clear();
        markerNodes.clear();
        leaderNode      =   nullptr;
        leaderNose      =   nullptr;
        followerNode    =   nullptr;
        followerNose    =   nullptr;
    });
    /// 7. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

/// sample.path
/// 路径约束(FEConstraintPath)验证示例:
///   1. 创建引擎 + 渲染窗口
///   2. 目标 box(红色)：挂 FEConstraintPath 路径约束,沿给定的路径点序列
///      (右 -> 上 -> 左 -> 下 -> 右,构成菱形)以恒定速度移动并循环
///   3. 在每个路径点放置一个小方块(绿色)作为路径顶点标记,便于目视验证
///      红盒始终沿相邻顶点间的直线段移动,证明被约束在路径上
/// 用法: sample.path.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
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

    /// 2.5 相机: 拉远以便完整观察路径
    scene->camera()->setEye(real3(0.0, 0.0, -6.0));
    scene->camera()->setTarget(real3(0.0, 0.0, 0.0));

    /// 3. 定义路径点(菱形,世界坐标,z=0 平面)
    ///    右 -> 上 -> 左 -> 下 -> 回到右
    std::vector<real3>   pathPoints  =   {
        real3( 1.8f,  0.0f, 0.0f),
        real3( 0.0f,  1.8f, 0.0f),
        real3(-1.8f,  0.0f, 0.0f),
        real3( 0.0f, -1.8f, 0.0f),
        real3( 1.8f,  0.0f, 0.0f),
    };

    /// 4. 在每个路径点放置绿色小方块作为顶点标记(便于目视验证路径)
    Nodes   markerNodes;
    for (size_t i = 0; i < pathPoints.size(); ++i)
    {
        Node    marker  =   createBoxNode(ctx, 0.15f,
            float4(0.30f, 0.85f, 0.35f, 1.0f),
            float4(0.15f, 0.45f, 0.18f, 1.0f));
        marker->setLocalTranslation(pathPoints[i]);
        marker->update();
        markerNodes.push_back(marker);
    }

    /// 5. 目标 box(红色)：挂路径约束,沿 pathPoints 移动
    Node    targetNode  =   createBoxNode(ctx, 0.30f,
        float4(0.95f, 0.25f, 0.15f, 1.0f),
        float4(0.65f, 0.12f, 0.08f, 1.0f));

    ConstraintPath  path    =   new FEConstraintPath(ctx);
    path->setPath(pathPoints);
    path->setSpeed(2.5f);     /// 2.5 单位/秒
    path->setLoop(true);      /// 循环
    targetNode->addComponent(path.get());
    targetNode->update();

    /// 6. 节点分发到渲染工厂与各组件系统
    ///    标记节点和红盒都是顶层节点,直接传入
    Nodes   allNodes    =   markerNodes;
    allNodes.push_back(targetNode);
    scene->dispatchNodesToSystem(allNodes);

    std::cout << "path constraint running: red box moves along the diamond path (green markers)..." << std::endl;

    /// 7. 进入渲染循环
    app->run();

    /// 释放本地引用
    scene   =   nullptr;
    app->destroy();

    return  0;
}

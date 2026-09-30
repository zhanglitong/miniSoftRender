/// sample.lookat
/// 朝向约束(FEConstraintLookAt) + 路径约束(FEConstraintPath) 示例:
///   1. 创建引擎 + 渲染窗口
///   2. 目标 box(红色)：挂 FEConstraintPath 路径约束,沿给定的菱形路径点
///      (右->上->左->下->右)以恒定速度移动并循环
///   3. 跟随者是一支"箭": followerNode(原点,无网格)挂 LookAt 约束,
///      其下用绕 X +90° 的子节点把圆柱长轴对齐 local +Z:
///        - shaftNode 蓝色圆柱(箭杆)
///        - tipNode   青色圆锥(箭头,尖端朝 +Z)
///      FEConstraintLookAt 每帧使 follower 的 local +Z 指向目标,
///      因此箭头方向应始终指向红色目标当前位置
/// 用法: sample.lookat.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/constraint/FEConstraintLookAt.hpp"
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

/// 用一组圆柱参数 + 指定材质颜色创建节点
static  Node    createCylinderNode(FEContext& ctx, const FEParamCylinder& param,
                                    const float4& diffuse, const float4& emissive)
{
    FEGeometryCylinder   geo(ctx);
    geo.param()  =   param;

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

    /// 2.5 相机:
    ///     拉远相机以便完整观察红盒的运动轨迹
    scene->camera()->setEye(real3(0.0, 0.0, -6.0));
    scene->camera()->setTarget(real3(0.0, 0.0, 0.0));

    /// 3. 目标 box(红色)：由 FEConstraintPath 路径约束驱动
    ///    红盒只能在给定的路径点序列上移动: 右->上->左->下->右 构成菱形,
    ///    以恒定速度沿路径行进并循环;FEConstraintLookAt 使箭始终指向它
    Node    targetNode  =   createBoxNode(ctx, 0.25f,
        float4(0.95f, 0.25f, 0.15f, 1.0f),
        float4(0.65f, 0.12f, 0.08f, 1.0f));

    ConstraintPath  path    =   new FEConstraintPath(ctx);
    /// 菱形路径(世界坐标,z=0 平面): 右顶点->上顶点->左顶点->下顶点->回到右顶点
    path->addPoint(real3( 1.4f,  0.0f, 0.0f));
    path->addPoint(real3( 0.0f,  1.4f, 0.0f));
    path->addPoint(real3(-1.4f,  0.0f, 0.0f));
    path->addPoint(real3( 0.0f, -1.4f, 0.0f));
    path->addPoint(real3( 1.4f,  0.0f, 0.0f));
    path->setSpeed(2.0f);     /// 2 单位/秒
    path->setLoop(true);
    targetNode->addComponent(path.get());
    targetNode->update();

    /// 4. 跟随者根节点(原点,无网格),挂 LookAt 约束:
    ///    约束每帧把 follower 的 local +Z 转到指向红盒
    Node    followerNode    =   new FENode(ctx);
    followerNode->setLocalTranslation(real3(0.0f, 0.0f, 0.0f));
    followerNode->update();

    /// 圆柱/圆锥几何默认长轴沿 Y,绕 X +90° 后长轴对齐 local +Z(LookAt 朝向轴)
    quatr   rotAlignZ   =   glm::angleAxis(glm::radians(90.0), real3(1.0f, 0.0f, 0.0f));

    /// 箭杆:蓝色圆柱,高 0.5,旋转后范围 z ∈ [-0.25, 0.25]
    FEParamCylinder  shaftParam;
    shaftParam.radiusTop      =   0.07f;
    shaftParam.radiusBottom   =   0.07f;
    shaftParam.height         =   0.5f;
    shaftParam.radialSegments =   24;
    Node    shaftNode   =   createCylinderNode(ctx, shaftParam,
        float4(0.15f, 0.55f, 0.95f, 1.0f),
        float4(0.10f, 0.35f, 0.65f, 1.0f));
    shaftNode->setLocalRotation(rotAlignZ);
    followerNode->addChild(shaftNode);
    shaftNode->update();

    /// 箭头:青色圆锥(radiusTop=0,尖端原在 +Y),旋转后尖端朝 +Z;
    /// 前移使底圆盘接住箭杆前端(杆端 z=0.25,锥半高 0.125 → 中心 z=0.375)
    FEParamCylinder  tipParam;
    tipParam.radiusTop      =   0.0f;
    tipParam.radiusBottom   =   0.10f;
    tipParam.height         =   0.25f;
    tipParam.radialSegments =   24;
    Node    tipNode     =   createCylinderNode(ctx, tipParam,
        float4(0.20f, 0.85f, 0.95f, 1.0f),
        float4(0.10f, 0.55f, 0.65f, 1.0f));
    tipNode->setLocalRotation(rotAlignZ);
    tipNode->setLocalTranslation(real3(0.0f, 0.0f, 0.375f));
    followerNode->addChild(tipNode);
    tipNode->update();

    /// 5. 创建朝向约束并绑定目标节点
    ConstraintLookAt constraint  =   new FEConstraintLookAt(ctx);
    constraint->setTargetNode(targetNode);
    followerNode->addComponent(constraint.get());
    followerNode->update();

    /// 6. 节点分发到渲染工厂与各组件系统,然后启动默认 Action 播放
    ///    只传顶层节点: 工厂与组件系统都会递归子节点,
    ///    若再显式传入 shaft/tip 会被收集两次、画出重复实例(一个停在初始朝向,
    ///    与跟随朝向垂直,形成交叉十字)
    scene->dispatchNodesToSystem({targetNode, followerNode});
    scene->animationSystem()->getOrCreate()->play();

    std::cout << "lookat running: blue arrow (cylinder shaft + cone tip) tracks the moving red target..." << std::endl;

    /// 7. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

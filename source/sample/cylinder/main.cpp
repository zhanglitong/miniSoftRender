/// sample.cylinder
/// 圆柱体绘制(FEGeometryCylinder)示例:
///   1. 创建引擎 + 渲染窗口
///   2. 用同一套布局常量(半径/间距)生成三个节点,验证圆柱几何的不同参数路径:
///      - 标准圆柱体(中间,橙色): radiusTop=radiusBottom,带顶/底盖
///      - 圆锥(左侧,绿色):       radiusTop=0,只带底盖
///      - 开口管(右侧,蓝色):     openEnded=true,无盖管状
/// 用法: sample.cylinder.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 统一布局参数(单一计算来源,三个节点都从这里推导)
constexpr  real     C_RADIUS    =   0.35f;  ///< 三个物体的半径
constexpr  real     C_SPACING   =   1.10f;  ///< 相邻物体中心间距
constexpr  uint32_t C_SEGMENTS  =   32;     ///< 圆周分段数

/// 用一组圆柱参数 + 指定材质颜色创建节点
static  Node    createCylinderNode(FEContext& ctx, const FEParamCylinder& param,
                                   const float4& diffuse, const float4& emissive)
{
    FEGeometryCylinder geo(ctx);
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

    /// 3. 标准圆柱体(中间):高 1.2,顶底半径相同,封口
    FEParamCylinder   cylParam;
    cylParam.radiusTop      =   C_RADIUS;
    cylParam.radiusBottom   =   C_RADIUS;
    cylParam.height         =   1.2f;
    cylParam.radialSegments =   C_SEGMENTS;
    Node  cylinder  =   createCylinderNode(ctx, cylParam,
        float4(0.95f, 0.55f, 0.15f, 1.0f),
        float4(0.55f, 0.28f, 0.06f, 1.0f));
    cylinder->setLocalTranslation(real3(0.0f, 0.0f, 0.0f));
    cylinder->update();

    /// 4. 圆锥(左侧):radiusTop=0,高 1.0
    FEParamCylinder   coneParam;
    coneParam.radiusTop      =   0.0f;
    coneParam.radiusBottom   =   C_RADIUS;
    coneParam.height         =   1.0f;
    coneParam.radialSegments =   C_SEGMENTS;
    Node  cone  =   createCylinderNode(ctx, coneParam,
        float4(0.25f, 0.85f, 0.30f, 1.0f),
        float4(0.10f, 0.45f, 0.12f, 1.0f));
    cone->setLocalTranslation(real3(-C_SPACING, 0.0f, 0.0f));
    cone->update();

    /// 5. 开口管(右侧):openEnded=true,高 1.0,无顶/底盖
    FEParamCylinder   tubeParam;
    tubeParam.radiusTop      =   C_RADIUS;
    tubeParam.radiusBottom   =   C_RADIUS;
    tubeParam.height         =   1.0f;
    tubeParam.radialSegments =   C_SEGMENTS;
    tubeParam.openEnded      =   true;
    Node  tube  =   createCylinderNode(ctx, tubeParam,
        float4(0.20f, 0.50f, 0.95f, 1.0f),
        float4(0.08f, 0.25f, 0.60f, 1.0f));
    tube->setLocalTranslation(real3(C_SPACING, 0.0f, 0.0f));
    tube->update();

    /// 6. 三个节点分发到渲染工厂与各组件系统
    scene->dispatchNodesToSystem({cone, cylinder, tube});

    std::cout << "cylinder running: cone | cylinder | open tube..." << std::endl;

    /// 7. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

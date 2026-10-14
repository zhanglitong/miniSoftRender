/// sample.oit
/// Order-Independent Transparency (PPLL) 演示:
///   多个半透明球相互穿插,用链表按像素排序后 alpha 合成
///
/// 用法: sample.oit.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/graphic/FEOITManager.h"
#include    "../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/material/FEMaterialOIT.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 创建一个半透明球节点:OIT 可用时走 FEMaterialOIT(geometry pass 收集进链表),
/// 否则回退普通 PBR alpha 混合
static  Node    createTransparentSphere(FEContext& ctx, uint32_t segments,
                                        const float4& color, float3 position)
{
    FEGeometrySphere geo(ctx);
    FEParamSphere param;
    param.radius        = 0.5f;
    param.widthSegments = segments;
    param.heightSegments = segments;
    geo.param()         = param;

    Mesh mesh = geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT} });

    Node node = new FENode(ctx);
    node->setMesh(mesh);

    FEMaterial* pMat = nullptr;
    if (FEOITManager::active())
    {
        // OIT 材质: alpha < 1.0 表示半透明,fragment 写入 PPLL 链表
        FEMaterialOIT* pOIT = new FEMaterialOIT(ctx);
        pOIT->data()._value._diffuse  = color;   // a 通道 = 透明度
        pOIT->data().update();
        pMat = pOIT;
    }
    else
    {
        // PBR 材质: 普通 alpha 混合回退
        FEMaterialPBR* pPBR = new FEMaterialPBR(ctx);
        pPBR->data()._value._diffuse  = color;
        pPBR->data().update();
        pMat = pPBR;
    }
    node->setMaterial(pMat);
    node->setLocalTranslation(position);
    node->update();
    return node;
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId = RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0)
            rendererId = RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0)
            rendererId = RS_VULKAN;
    }

    FEContext&  ctx   =   FEContext::instance();
    FEScene*    pScene = nullptr;

    FEApp::CreateInfo info = {};
    info._width   = 1280;
    info._height  = 720;
    info._appInst = GetModuleHandle(nullptr);
    info._notify  = [&](const FEMessage& msg) { if (pScene) pScene->onMessage(msg); };
    App app = FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return 1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene scene = new FEScene(ctx);
    pScene = scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }

    // ===== 初始化 OITManager（必须在创建 OIT 材质之前:
    //        FEMaterialOIT 依赖 oitGeometry 管线注册与 SSBO 绑定）=====
    uint32_t w = app->cInfo()._width;
    uint32_t h = app->cInfo()._height;

    auto oit = OITManager(new FEOITManager(ctx));
    if (!oit->init(w, h, 20))
    {
        std::cerr << "OITManager init failed (falling back to normal alpha)" << std::endl;
        oit = nullptr;
    }
    else
    {
        std::cout << "OITManager initialized: " << w << "x" << h
                  << ", maxNodes=" << oit->screenWidth() * oit->screenHeight() * 20 << std::endl;
        FEOITManager::setActive(oit.get());
        scene->setOITManager(oit);
    }

    // ===== 创建半透明球阵列 =====
    const uint32_t SEG = 24;

    // 中心大球（红色, 半透明）
    Node center = createTransparentSphere(ctx, SEG,
        float4(1.0f, 0.2f, 0.2f, 0.5f), float3(0, 0, 0));

    // 左侧球（绿色）
    Node left = createTransparentSphere(ctx, SEG,
        float4(0.2f, 1.0f, 0.2f, 0.5f), float3(-1.2f, 0, 0));

    // 右侧球（蓝色）
    Node right = createTransparentSphere(ctx, SEG,
        float4(0.2f, 0.4f, 1.0f, 0.5f), float3(1.2f, 0, 0));

    // 前方球（黄色）
    Node front = createTransparentSphere(ctx, SEG,
        float4(1.0f, 0.9f, 0.2f, 0.5f), float3(0, 0.3f, -1.5f));

    // 后方球（紫色）
    Node back = createTransparentSphere(ctx, SEG,
        float4(0.9f, 0.3f, 1.0f, 0.5f), float3(0, -0.3f, 1.5f));

    scene->dispatchNodesToSystem({center, left, right, front, back});

    std::cout << "sample.oit running (" << (oit ? "OIT PPLL" : "alpha blending") << ")..." << std::endl;

    app->run();

    scene = nullptr;
    app->destroy();
    return 0;
}

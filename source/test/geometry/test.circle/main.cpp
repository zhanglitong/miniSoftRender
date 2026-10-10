/// test.circle
/// 圆形(FEGeometryCircle)绘制测试:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometryCircle 按 pos/nor/uv 生成圆形 mesh,校验几何非空
///   3. 经 FEMaterialPBR 分发到渲染工厂并进入渲染循环显示
/// 用法: test.circle.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometryCircle.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
            rendererId = RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
            rendererId = RS_VULKAN;
    }

    FEContext&  ctx =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

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

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 用 FEGeometryCircle 生成圆形 mesh
    bool    passed  =   false;
    {
        FEGeometryCircle    geo(ctx);
        geo.param().radius     =   1.0f;
        geo.param().segments   =   32;

        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

        uint    posCount    =   0;
        uint    norCount    =   0;
        if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
            posCount    =   p->count();
        if (auto* n = mesh->get(FEAttribute(IS_VERTEX_NOR)))
            norCount    =   n->count();
        std::cout << "circle buffers=" << mesh->buffers().size()
                  << " pos=" << posCount
                  << " nor=" << norCount
                  << " primitives=" << mesh->primitives().size() << std::endl;

        passed  =   (mesh->buffers().size() >= 3)
                 && (posCount > 0)
                 && (norCount == posCount)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
            pMat->data()._value._diffuse  =   float4(0.15f, 0.85f, 0.35f, 1.0f);
            pMat->data()._value._emissive =   float4(0.10f, 0.55f, 0.25f, 1.0f);
            pMat->data().update();
            node->setMaterial(pMat);
            /// 圆形在 XY 平面,旋转使其面向相机
            //node->setLocalRotation(quatr(glm::radians(real3(22.0, 38.0, 0.0))));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "circle build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "circle build ok, entering render loop..." << std::endl;

    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

/// test.tube
/// 管道(FEGeometryTube)绘制测试 —— 沿 Catmull-Rom 样条挤出圆形截面:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometryTube 按 pos/nor/uv 生成管道 mesh,校验几何非空
///   3. 经 FEMaterialPBR 分发到渲染工厂并进入渲染循环显示
/// 用法: test.tube.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometryTube.hpp"
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

    /// 用 FEGeometryTube(Catmull-Rom) 生成管道 mesh
    bool    passed  =   false;
    {
        FEGeometryTube  geo(ctx);
        geo.param().type             =   FECurveType::CatmullRom;
        geo.param().points           =   {
            float3(-2.0f, -0.8f,  0.0f),
            float3(-1.0f,  0.8f,  0.6f),
            float3( 0.0f,  0.2f,  0.0f),
            float3( 1.0f, -0.8f, -0.6f),
            float3( 2.0f,  0.8f,  0.0f),
        };
        geo.param().radius           =   0.15f;
        geo.param().tubularSegments  =   128;
        geo.param().radialSegments   =   12;
        geo.param().closed           =   false;
        geo.param().catmullType      =   FECatmullRomType::Centripetal;

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
        std::cout << "tube buffers=" << mesh->buffers().size()
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
            pMat->data()._value._diffuse  =   float4(0.20f, 0.65f, 0.90f, 1.0f);
            pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.55f, 1.0f);
            pMat->data().update();
            node->setMaterial(pMat);
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "tube build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "tube build ok, entering render loop..." << std::endl;

    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

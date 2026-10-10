/// test.aabb
/// 包围盒(FEGeometryAabb)线框绘制测试:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometryAabb 按 pos+color 生成包围盒 mesh,校验几何非空
///   3. 经 FEMaterialV3C4 分发到渲染工厂并进入渲染循环显示
/// 用法: test.aabb.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometryAabb.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"

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

    /// 用 FEGeometryAabb 生成包围盒线框 mesh
    bool    passed  =   false;
    {
        FEGeometryAabb  geo(ctx);
        geo.param()._size  =   float3(2.0f, 2.0f, 2.0f);
        geo.param()._color =   Rgba8(100, 200, 255, 255);

        /// AABB 生成位置+颜色,线框渲染
        Mesh    mesh    =   geo.triangular({{IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                                            {IS_VERTEX_COLOR0,    FMT_R8G8B8A8_UNORM} });

        uint    posCount    =   0;
        if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
            posCount    =   p->count();
        std::cout << "aabb buffers=" << mesh->buffers().size()
                  << " pos=" << posCount
                  << " primitives=" << mesh->primitives().size() << std::endl;

        passed  =   (mesh->buffers().size() >= 1)
                 && (posCount == 8)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            FEMaterialV3C4* pMat  =   new FEMaterialV3C4(ctx);
            node->setMaterial(pMat);

            node->setLocalRotation(quatr(glm::radians(real3(22.0, 38.0, 0.0))));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "aabb build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "aabb build ok, entering render loop..." << std::endl;

    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

/// test.svg
/// SVG 文件加载、解析与渲染测试:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometrySVG 加载 data/svg/demo.svg,解析 path/rect/circle/polygon 等图元
///   3. 生成带顶点色的三角形网格,经 FEMaterialV3C4 渲染
/// 用法: test.svg.exe [svg文件] [--vulkan|--webgpu]

#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometrySVG.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    FEUuid          rendererId  =   RS_VULKAN;
    const   char*   svgArg      =   nullptr;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
            rendererId = RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
            rendererId = RS_VULKAN;
        else if (arg[0] != '-')
            svgArg  =   arg;
    }

    FEContext&  ctx     =   FEContext::instance();
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

    /// 正交相机:位于 +Z,便于观察平面 SVG 图形
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0f,0.0f,10.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->setNear(0.1f);
        cam->setFar(100.0f);
        cam->setScaler(1.0f);
        cam->updateOrtho();
    }
    /// 隐藏默认地面网格,避免与平面 SVG 重叠
    scene->showGrid(false);

    const   String  svgFile =   svgArg != nullptr
                            ?   String(svgArg)
                            :   ctx.resourcePath() + "/data/svg/demo.svg";
    std::cout << "loading svg: " << svgFile << std::endl;

    FEGeometrySVG   geo(ctx);
    geo.options()._curveSegments    =   24;
    geo.options()._flipY            =   true;
    geo.options()._center           =   true;

    bool    passed  =   false;
    if (geo.loadFile(svgFile))
    {
        const   auto&   doc =   geo.document();
        size_t          subCount    =   0;
        for (const auto& path : doc.paths)
            subCount += path.subPaths.size();
        std::cout << "parsed paths=" << doc.paths.size()
                  << " subPaths=" << subCount
                  << " viewBox=" << doc.hasViewBox
                  << " width=" << doc.width
                  << " height=" << doc.height << std::endl;

        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,      FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,      FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT},
            {IS_VERTEX_COLOR0,   FMT_R8G8B8A8_UNORM} });

        if (mesh != nullptr)
        {
            uint    posCount    =   0;
            uint    colCount    =   0;
            if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
                posCount    =   p->count();
            if (auto* c = mesh->get(FEAttribute(IS_VERTEX_COLOR0)))
                colCount    =   c->count();

            std::cout << "svg buffers=" << mesh->buffers().size()
                      << " pos=" << posCount
                      << " color=" << colCount
                      << " primitives=" << mesh->primitives().size() << std::endl;

            passed  =   (doc.paths.size() > 0)
                     && (posCount > 0)
                     && (colCount == posCount)
                     && (!mesh->primitives().empty());

            if (passed)
            {
                FEMaterialV3C4* pMat    =   new FEMaterialV3C4(ctx);
                if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
                {
                    std::cerr << "pipeline v3c4/TRIANGLES not found" << std::endl;
                    passed  =   false;
                }
                else
                {
                    Node    node    =   new FENode(ctx);
                    node->setMesh(mesh);
                    node->setMaterial(pMat);
                    node->setColor(Rgba8(255,255,255,255));
                    node->makeDirty();
                    node->update();

                    /// 立即常驻,避免后续 dispatch 销毁该节点所在工厂
                    DispatchResult  dr;
                    scene->dispatchNodesToSystem({node},&dr);
                    for (auto& f : dr.rFactorys)
                    {
                        f->setResident(true);
                        f->setGPUCull(false);
                    }
                }
            }
        }
    }

    if (!passed)
    {
        std::cerr << "svg build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "svg build ok, entering render loop..." << std::endl;

    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

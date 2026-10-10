/// test.curveline
/// 样条曲线(FE 曲线, 移植自 three.js)有窗口线框绘制测试:
///   用 getSpacedPoints 等距采样 4 种曲线, 展开为 PRI_LINES 线段顶点,
///   以不同颜色横向排列: 直线 / 二次贝塞尔 / 三次贝塞尔 / Catmull-Rom
///   1. 创建引擎 + 渲染窗口
///   2. FEMeshBuilder::makeMeshLine 生成线 mesh,校验非空
///   3. 经 FEMaterialV3C4 分发到渲染工厂并进入渲染循环显示
/// 用法: test.curveline.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <memory>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/mesh/FEMeshHelper.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"
#include    "../../../inc/geometry/curve/FELineCurve3.hpp"
#include    "../../../inc/geometry/curve/FEQuadraticBezierCurve3.hpp"
#include    "../../../inc/geometry/curve/FECubicBezierCurve3.hpp"
#include    "../../../inc/geometry/curve/FECatmullRomCurve3.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 将曲线等距采样并展开为 PRI_LINES 需要的顶点对(p0,p1,p1,p2,...)
static  void    sampleLineSegments(const FECurve& curve, uint divisions,
                                   float3 offset, const Rgba8& color,
                                   float3s& outVerts, Rgba8s& outColors)
{
    float3s pts =   curve.getSpacedPoints(divisions);
    for (size_t i = 0; i + 1 < pts.size(); ++i)
    {
        outVerts.push_back(pts[i]     + offset);
        outVerts.push_back(pts[i + 1] + offset);
        outColors.push_back(color);
        outColors.push_back(color);
    }
}

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

    /// 构造 4 条形状一致的曲线(横向错开 6 个单位排列)
    const uint  divisions =   120;

    FELineCurve3            line(float3(-1, 0, 0), float3(1, 1.2f, 0));
    FEQuadraticBezierCurve3 quad(float3(-1, 0, 0), float3(-1, 1.6f, 0), float3(1, 0, 0));
    FECubicBezierCurve3     cubic(float3(-1, 0, 0), float3(-1.5f, 1.8f, 0.8f),
                                  float3(1.5f, 1.8f, -0.8f), float3(1, 0, 0));
    FECatmullRomCurve3      catmull(float3s{
        float3(-1.0f, 0.0f,  0.0f),
        float3(-0.5f, 1.4f,  0.6f),
        float3( 0.0f, 0.2f,  0.0f),
        float3( 0.5f,-1.2f, -0.6f),
        float3( 1.0f, 0.0f,  0.0f),
    }, false, FECatmullRomType::Centripetal);

    float3s verts;
    Rgba8s  colors;
    sampleLineSegments(line,     divisions, float3(-9.0f, 0, 0), Rgba8(255,  90,  90, 255), verts, colors);
    sampleLineSegments(quad,     divisions, float3(-3.0f, 0, 0), Rgba8( 90, 255, 120, 255), verts, colors);
    sampleLineSegments(cubic,    divisions, float3( 3.0f, 0, 0), Rgba8( 90, 160, 255, 255), verts, colors);
    sampleLineSegments(catmull,  divisions, float3( 9.0f, 0, 0), Rgba8(230, 200,  80, 255), verts, colors);

    bool    passed  =   false;
    {
        Attrs   attrs   =   {
            {IS_VERTEX_POS,    FMT_R32G32B32_FLOAT},
            {IS_VERTEX_COLOR0, FMT_R8G8B8A8_UNORM},
        };
        Mesh    mesh    =   FEMeshBuilder::makeMeshLine(ctx, attrs, verts, colors);

        uint    posCount    =   0;
        if (mesh && (mesh->get(FEAttribute(IS_VERTEX_POS))))
            posCount    =   mesh->get(FEAttribute(IS_VERTEX_POS))->count();
        std::cout << "curveline verts=" << verts.size()
                  << " pos=" << posCount
                  << " primitives=" << (mesh ? mesh->primitives().size() : 0) << std::endl;

        passed  =   (mesh != nullptr)
                 && (posCount == verts.size())
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            FEMaterialV3C4* pMat  =   new FEMaterialV3C4(ctx);
            node->setMaterial(pMat);
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "curveline build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "curveline build ok, entering render loop..." << std::endl;

    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

/// test.texture.ktx.cube
/// KTX/KTX2 立方体贴图(cubemap)+ 贴到立方体测试:
///   1. 引擎初始化(与 test.texture.ktx 一致)
///   2. 从 ktx/ktx2 文件加载 cubemap(6 面,RGBA8;libktx transcode / 直拷)
///   3. 用 FETextureHelper::create 由 6 层 FEImage 创建 cubemap 纹理
///   4. 用 FEGeometryBox 生成立方体 mesh(pos + uv)
///   5. FEMaterialCube 分发渲染 → 渲染循环显示
/// 用法:
///   test.texture.ktx.cube.exe [--vulkan|--webgpu] [--ktx <file.ktx|file.ktx2>]
/// 默认 ktx: source/external/ktx/tests/resources/ktx2/cubemap_yokohama_blze.ktx2

#include    <iostream>
#include    <cstdint>
#include    <string>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FETexture.h"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/material/FEMaterialCube.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    const char* kDefaultKtxRel =   "source/external/ktx/tests/resources/ktx2/cubemap_yokohama_blze.ktx2";

    std::string findFileUpwards(const std::string& rel)
    {
        char    cwd[MAX_PATH]   =   {};
        GetCurrentDirectoryA(MAX_PATH, cwd);
        std::string cur = cwd;
        for (int depth = 0; depth < 6; ++depth)
        {
            std::string candidate = cur + "/" + rel;
            if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
                return  candidate;
            auto    pos = cur.find_last_of("/\\");
            if (pos == std::string::npos)
                break;
            cur = cur.substr(0, pos);
        }
        return  {};
    }
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    std::string     ktxPath;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if ((_stricmp(arg, "--ktx") == 0 || _stricmp(arg, "-ktx") == 0) && i + 1 < argc)
            ktxPath =   argv[++i];
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

    /// ---- 加载 cubemap 并创建纹理 ----
    if (ktxPath.empty())
        ktxPath =   findFileUpwards(kDefaultKtxRel);
    if (ktxPath.empty())
    {
        std::cerr << "ktx file not found (try --ktx <path>)" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }
    std::cout << "[ktx] loading: " << ktxPath << std::endl;

    ImagePtr    img =   FEImageHelper::loadKtx(ctx, ktxPath.c_str(), FMT_R8G8B8A8_UNORM);
    if (img == nullptr)
    {
        std::cerr << "loadKtx returned nullptr" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }
    const auto& ici =   img->cInfo();
    std::cout << "[ktx] " << ici._extent.x << "x" << ici._extent.y
              << " fmt=" << (int)ici._format
              << " layers=" << ici._layers
              << " mips=" << ici._levels << std::endl;

    bool    passed  =   (ici._layers == 6) && (ici._extent.x == ici._extent.y);
    if (!passed)
    {
        std::cerr << "not a valid cubemap (expect 6 square faces)" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }

    TexPtr  cube    =   FETextureHelper::create(ctx,*img,
                                                FILTER_LINEAR,FILTER_LINEAR,
                                                ADDRESS_CLAMP_TO_EDGE);
    if (cube == nullptr || !cube->isCube())
    {
        std::cerr << "create cubemap texture failed" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }
    std::cout << "[cube] texture " << cube->width() << "x" << cube->height()
              << " cube=" << (cube->isCube() ? 1 : 0)
              << " mips=" << cube->mips() << std::endl;

    /// ---- box mesh + material ----
    {
        FEGeometryBox   geo(ctx);
        geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
        geo.param()._segs  =   uint3(1, 1, 1);

        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

        uint    posCount = 0, uvCount = 0;
        if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS))) posCount = p->count();
        if (auto* uv = mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0))) uvCount = uv->count();
        std::cout << "[geo] box buffers=" << mesh->buffers().size()
                  << " pos=" << posCount << " uv=" << uvCount
                  << " primitives=" << mesh->primitives().size() << std::endl;

        passed  =   passed && (mesh->buffers().size() >= 2)
                 && (posCount > 0) && (uvCount == posCount)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            FEMaterialCube* pMat    =   new FEMaterialCube(ctx);
            pMat->setCube(cube);
            if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
            {
                std::cerr << "pipeline cube/TRIANGLES not found" << std::endl;
                passed = false;
            }
            node->setMaterial(pMat);
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "validation failed" << std::endl;
        scene   =   nullptr; app->destroy(); return 1;
    }
    std::cout << "ktx cubemap box ok, entering render loop..." << std::endl;

    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的 cubemap 纹理
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        cube        =   nullptr;
    });
    app->run();

    scene   =   nullptr;
    app->destroy();
    return  0;
}

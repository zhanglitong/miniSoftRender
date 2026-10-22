/// test.texture.ktx
/// KTX/KTX2 纹理 + 贴图立方体测试:
///   1. 引擎初始化(与 test.texture.box 一致)
///   2. 从 ktx/ktx2 文件加载 RGBA8 纹理(libktx transcode / 直拷)
///      支持 --programtex 模式:程序生成棋盘格纹理(用于隔离 ktx 路径差异)
///   3. 用 FEGeometryBox 生成立方体 mesh(pos + uv)
///   4. FEMaterialTexture 分发渲染 → 渲染循环显示
/// 用法:
///   test.texture.ktx.exe [--vulkan|--webgpu] [--ktx <file.ktx|file.ktx2>] [--programtex]
/// 默认 ktx: source/external/ktx/tests/resources/ktx2/r8g8b8a8_srgb.ktx2

#include    <iostream>
#include    <vector>
#include    <cstdint>
#include    <cstring>
#include    <string>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FETexture.h"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/material/FEMaterialTexture.hpp"
#include    "../../../inc/FEFormatHelper.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    const char* kDefaultKtxRel =   "source/external/ktx/tests/resources/ktx2/r8g8b8a8_srgb.ktx2";

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

    std::vector<uint8>  makeCheckerBoard(uint width,uint height,uint cells)
    {
        std::vector<uint8>  data(size_t(width) * height * 4);
        uint    cellW   =   width  / cells;
        uint    cellH   =   height / cells;
        for (uint y = 0; y < height; ++y)
            for (uint x = 0; x < width; ++x)
            {
                bool    odd =   ((x / cellW) + (y / cellH)) & 1;
                data[(size_t(y) * width + x) * 4 + 0]   =   odd ? 0x1Au : 0xF2u;
                data[(size_t(y) * width + x) * 4 + 1]   =   odd ? 0x2Eu : 0x8Cu;
                data[(size_t(y) * width + x) * 4 + 2]   =   odd ? 0x7Eu : 0x2Eu;
                data[(size_t(y) * width + x) * 4 + 3]   =   0xFFu;
            }
        return  data;
    }

    /// 从 FEImage level 0 提取紧凑 RGBA8 像素(RGB 自动补 alpha=255)
    std::vector<uint8>  extractLevel0RGBA8(const FEImage& img,uint32& outW,uint32& outH)
    {
        outW    =   img.cInfo()._extent.x;
        outH    =   img.cInfo()._extent.y;
        auto    srcPs   =   FEFormatHelper::sizeOf(img.cInfo()._format);
        const uint8*    src =   img.pixel(0,0);
        if (src == nullptr || srcPs == 0 || outW == 0 || outH == 0)
            return  {};

        /// 计算源每行字节数(考虑 FEImage 内部 align padding)
        size_t  lvl0Size    =   img.offsetOfLevel(1) - img.offsetOfLevel(0);
        uint32  srcRowBytes =   uint32(lvl0Size / outH);
        if (srcRowBytes < outW * srcPs)
            srcRowBytes =   outW * srcPs;

        const uint32    dstPs   =   4;   /// RGBA8
        std::vector<uint8>  out(size_t(outW) * outH * dstPs, 0xFF);
        for (uint32 y = 0; y < outH; ++y)
        {
            const uint8*    srcRow = src + size_t(y) * srcRowBytes;
            uint8*          dstRow = out.data() + size_t(y) * outW * dstPs;
            for (uint32 x = 0; x < outW; ++x)
            {
                const uint8*    sp  =   srcRow + size_t(x) * srcPs;
                uint8*          dp  =   dstRow + size_t(x) * dstPs;
                for (uint32 c = 0; c < srcPs && c < 4; ++c)
                    dp[c] = sp[c];
            }
        }
        return  out;
    }
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    std::string     ktxPath;
    bool    programTex  =   false;
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
        else if (_stricmp(arg, "--programtex") == 0 || _stricmp(arg, "-programtex") == 0)
            programTex  =   true;
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

    /// ---- 纹理创建 ----
    TexPtr  texture =   ctx.device().createTexture();
    bool    texOk   =   false;
    uint32  tw = 0, th = 0;
    std::vector<uint8>  pixels;

    if (programTex)
    {
        constexpr uint  kTexSize = 512;
        tw  =   th =   kTexSize;
        pixels  =   makeCheckerBoard(kTexSize, kTexSize, 8);
        std::cout << "[tex] program-generated checkerboard "
                  << kTexSize << "x" << kTexSize
                  << " " << pixels.size() << " bytes" << std::endl;
    }
    else
    {
        if (ktxPath.empty())
            ktxPath =   findFileUpwards(kDefaultKtxRel);
        if (ktxPath.empty())
        {
            std::cerr << "ktx file not found (try --ktx <path> or --programtex)" << std::endl;
            scene = nullptr; app->destroy(); return 1;
        }
        std::cout << "[ktx] loading: " << ktxPath << std::endl;

        ImagePtr    img =   FEImageHelper::loadKtx(ctx, ktxPath.c_str(), FMT_R8G8B8A8_UNORM);
        if (img == nullptr)
        {
            std::cout << "[ktx] loadKtx returned nullptr, fallback to FEImageHelper::load()" << std::endl;
            img =   FEImageHelper::load(ctx, ktxPath.c_str());
        }
        if (img == nullptr)
        {
            std::cerr << "texture load failed (use --programtex as fallback)" << std::endl;
            scene = nullptr; app->destroy(); return 1;
        }
        const auto& ici =   img->cInfo();
        std::cout << "[ktx] " << ici._extent.x << "x" << ici._extent.y
                  << " fmt=" << (int)ici._format
                  << " mips=" << ici._levels << std::endl;

        pixels  =   extractLevel0RGBA8(*img, tw, th);
        if (pixels.empty())
        {
            std::cerr << "level 0 pixel extraction failed" << std::endl;
            scene = nullptr; app->destroy(); return 1;
        }
    }

    if (texture == nullptr)
    {
        std::cerr << "device.createTexture() return nullptr" << std::endl;
    }
    else
    {
        FETexture::CreateInfo   texInfo =   {};
        texInfo._width      =   tw;
        texInfo._height     =   th;
        texInfo._format     =   FMT_R8G8B8A8_UNORM;
        texInfo._magFilter  =   FILTER_LINEAR;
        texInfo._minFilter  =   FILTER_LINEAR;
        texInfo._addressMode =  ADDRESS_REPEAT;
        texOk   =   texture->create(texInfo);
        std::cout << "[tex] create " << tw << "x" << th
                  << " fmt=" << (int)texInfo._format
                  << ": " << (texOk ? "ok" : "failed") << std::endl;
        if (texOk)
        {
            texOk   =   texture->update(pixels.data(), pixels.size());
            std::cout << "[tex] upload " << pixels.size() << " bytes: "
                      << (texOk ? "ok" : "failed") << std::endl;
        }
    }

    /// ---- box mesh + material ----
    bool    passed  =   false;
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

        passed  =   texOk && (mesh->buffers().size() >= 2)
                 && (posCount > 0) && (uvCount == posCount)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            FEMaterialTexture*  pMat    =   new FEMaterialTexture(ctx);
            pMat->setTexture(texture);
            if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
            {
                std::cerr << "pipeline texture/TRIANGLES not found" << std::endl;
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
        scene   = nullptr; app->destroy(); return 1;
    }
    std::cout << "ktx texture box ok, entering render loop..." << std::endl;

    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的 ktx 纹理
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        texture     =   nullptr;
    });
    app->run();

    scene   = nullptr;
    app->destroy();
    return  0;
}

/// test.texture.compressed
/// 压缩纹理作为贴图的端到端示例(检测 / 加载 / 上传 / 渲染):
///   1. 程序生成 256x256 RGBA8 棋盘格 FEImage
///   2. FEImageHelper::compress 编码为 BC3_UNORM 块压缩数据
///      (检测:输出格式 / 级数 / 块数据字节数)
///   3. FETexture 以压缩格式直接创建并上传块数据(不解码,零拷贝语义)
///   4. FEMaterialTexture 贴图盒子 → 渲染循环显示
/// 另支持 --ktx <file>:从压缩 ktx2 文件透传加载块数据(如 pattern_02_bc2.ktx2)
/// 用法:
///   test.texture.compressed.exe [--ktx <file.ktx2>]
/// 说明:BC 系列为桌面 GPU 标配;设备不支持时 create/update 会失败并打日志

#include    <iostream>
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
    constexpr uint  kTexSize    =   256;

    /// 向 RGBA8 图像写入棋盘格(检测用基准图案,块压缩后仍可肉眼分辨)
    bool    fillCheckerBoard(FEImage& img)
    {
        auto    sp      =   img.plane(0,0);
        if  (sp._buffer == nullptr)
            return  false;
        const uint  cells   =   8;
        const uint  cellW   =   sp.width() / cells;
        const uint  cellH   =   sp.height() / cells;
        for (uint y = 0; y < sp.height(); ++y)
            for (uint x = 0; x < sp.width(); ++x)
            {
                uint8*  p   =   sp.dataOffset<uint8>(y,x);
                const bool  odd  =  ((x / cellW) + (y / cellH)) & 1;
                p[0]    =   odd ? 0x1Au : 0xF2u;
                p[1]    =   odd ? 0x2Eu : 0x8Cu;
                p[2]    =   odd ? 0x7Eu : 0x2Eu;
                p[3]    =   0xFFu;
            }
        return  true;
    }

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
    std::string ktxPath;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if ((_stricmp(arg, "--ktx") == 0 || _stricmp(arg, "-ktx") == 0) && i + 1 < argc)
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
    if (!scene->setup(app, RS_VULKAN))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// ---- 1/2) 加载:程序生成 RGBA8 → 压缩为 BC3 块数据;或 ktx2 压缩透传 ----
    ImagePtr    blk     =   nullptr;
    if (ktxPath.empty())
    {
        /// 1) 源图像:RGBA8 棋盘格
        FEImage srcImg(ctx);
        FEImage::CreateInfo ci = {};
        ci._extent  =   uint3(kTexSize, kTexSize, 1);
        ci._format  =   FMT_R8G8B8A8_UNORM;
        ci._layers  =   uint16_t(1);
        ci._levels  =   uint16_t(1);
        ci._align   =   uint16_t(4);
        if (!srcImg.create(ci) || !fillCheckerBoard(srcImg))
        {
            std::cerr << "source image create/fill failed" << std::endl;
            scene = nullptr; app->destroy(); return 1;
        }
        std::cout << "[gen] " << kTexSize << "x" << kTexSize
                  << " RGBA8 checkerboard" << std::endl;

        /// 2) 编码为 BC3_UNORM(端点 + 索引 alpha,块字节 16)
        blk =   FEImageHelper::compress(srcImg, FMT_BC3_UNORM);
    }
    else
    {
        /// 压缩 ktx2 透传加载:不解码,保留块数据与压缩格式
        std::cout << "[ktx] loading compressed: " << ktxPath << std::endl;
        blk =   FEImageHelper::loadKtx(ctx, ktxPath.c_str(), FMT_NONE);
    }
    if (blk == nullptr)
    {
        std::cerr << "compressed image acquire failed" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }

    /// ---- 3) 检测:压缩数据的格式 / 级数 / 块字节数自洽 ----
    const auto&     bci     =   blk->cInfo();
    const uint32    bw      =   bci._extent.x;
    const uint32    bh      =   bci._extent.y;
    const bool      bBlock  =   FEFormatHelper::isBlockFormat(bci._format);
    const auto      be      =   FEFormatHelper::blockExtent(bci._format);
    const uint64    expect  =   uint64((bw + be.x - 1) / be.x)
                            *   ((bh + be.y - 1) / be.y)
                            *   FEFormatHelper::sizeOf(bci._format);
    const bool      detectOk    =   bBlock
                                    &&  blk->byteOfLevel(0) == expect
                                    &&  blk->cInfo()._levels >= 1;
    std::cout << "[chk] " << bw << "x" << bh
              << " fmt=" << (int)bci._format
              << " block=" << be.x << "x" << be.y
              << " mips=" << bci._levels
              << " level0=" << blk->byteOfLevel(0)
              << " bytes (expect " << expect << ")"
              << (detectOk ? " ok" : " FAILED") << std::endl;
    if (!detectOk)
    {
        std::cerr << "validation failed" << std::endl;
        scene = nullptr; app->destroy(); return 1;
    }

    /// ---- 4) 上传:压缩格式直接创建 GPU 纹理并整层上传块数据 ----
    TexPtr  texture =   ctx.device().createTexture();
    bool    texOk   =   false;
    if (texture == nullptr)
    {
        std::cerr << "device.createTexture() return nullptr" << std::endl;
    }
    else
    {
        FETexture::CreateInfo   texInfo =   {};
        texInfo._width      =   bw;
        texInfo._height     =   bh;
        texInfo._format     =   bci._format;
        texInfo._mips       =   bci._levels;
        texInfo._magFilter  =   FILTER_LINEAR;
        texInfo._minFilter  =   FILTER_LINEAR;
        texInfo._addressMode =  ADDRESS_REPEAT;
        texOk   =   texture->create(texInfo);
        std::cout << "[tex] create " << bw << "x" << bh
                  << " fmt=" << (int)bci._format
                  << " mips=" << bci._levels
                  << ": " << (texOk ? "ok" : "failed") << std::endl;
        if (texOk)
        {
            texOk   =   texture->update(blk->pixel(0,0), blk->bytesOfLayer());
            std::cout << "[tex] upload " << blk->bytesOfLayer() << " compressed bytes: "
                      << (texOk ? "ok" : "failed") << std::endl;
        }
    }

    /// ---- 5) 渲染:贴图盒子 ----
    bool    passed  =   texOk;
    if (passed)
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

        passed  =   (mesh->buffers().size() >= 2)
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
    std::cout << "compressed texture box ok, entering render loop..." << std::endl;

    /// 窗口关闭时抢在场景销毁 device 之前,释放 main 持有的压缩纹理
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        texture     =   nullptr;
        blk         =   nullptr;
    });
    app->run();

    scene   =   nullptr;
    app->destroy();
    return  0;
}

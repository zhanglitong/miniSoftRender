/// test.texture.box
/// 贴图立方体测试(Vulkan 纹理支持):
///   1. 创建引擎 + 渲染窗口
///   2. 程序生成 8x8 棋盘格 RGBA8 纹理,经 VKTexture 上传 GPU(staging + 布局切换)
///   3. 用 FEGeometryBox 按 pos/uv 生成立方体 mesh,校验 uv 属性
///   4. 经 FEMaterialTexture(combined image sampler -> SB_Material)分发渲染
/// 验证点:
///   a. 立方体各面显示棋盘格(说明 uv 采样正确)
///   b. 不旋转模型,方便观察各面贴图方向
/// 用法: test.texture.box.exe [--vulkan|--webgpu](当前纹理仅支持 vulkan)

#include    <iostream>
#include    <vector>
#include    <cstdint>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FETexture.h"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/material/FEMaterialTexture.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 程序生成 8x8 棋盘格纹理(RGBA8,行紧密排布无 padding)
    std::vector<uint8_t>    makeCheckerBoard(uint width,uint height,uint cells)
    {
        std::vector<uint8_t>    data(size_t(width) * height * 4);
        uint    cellW   =   width  / cells;
        uint    cellH   =   height / cells;
        for (uint y = 0; y < height; ++y)
        {
            for (uint x = 0; x < width; ++x)
            {
                bool    odd =   ((x / cellW) + (y / cellH)) & 1;
                /// 亮色: 橙白,暗色: 深蓝,对比清晰便于肉眼验证
                uint8_t r =   odd ? 0x1Au : 0xF2u;
                uint8_t g =   odd ? 0x2Eu : 0x8Cu;
                uint8_t b =   odd ? 0x7Eu : 0x2Eu;
                uint8_t a =   0xFFu;
                size_t  idx =   (size_t(y) * width + x) * 4;
                data[idx + 0]   =   r;
                data[idx + 1]   =   g;
                data[idx + 2]   =   b;
                data[idx + 3]   =   a;
            }
        }
        return  data;
    }
}

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan(纹理当前仅实现 Vulkan 版本)
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            std::cerr << "texture support is vulkan-only for now, please run without --webgpu" << std::endl;
            return  0;
        }
    }

    FEContext&  ctx =   FEContext::instance();

    /// 场景指针在引擎创建后赋值;notify 把窗口消息(更新/渲染/缩放/输入)
    /// 转发给场景,与 vkDemo 的 messageNotify 等价
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
    ///    FEScene 是引用计数对象,必须堆分配并用 SharedPtr 持有(与 vkDemo 一致)
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 创建纹理:程序生成棋盘格数据并上传 GPU
    constexpr   uint    kTexSize    =   512;
    bool    texOk   =   false;
    TexPtr  texture =   ctx.device().createTexture();
    {
        if (texture == nullptr)
        {
            std::cerr << "device.createTexture() return nullptr" << std::endl;
        }
        else
        {
            FETexture::CreateInfo   texInfo =   {};
            texInfo._width      =   kTexSize;
            texInfo._height     =   kTexSize;
            texInfo._format     =   FMT_R8G8B8A8_UNORM;
            texInfo._magFilter  =   FILTER_LINEAR;
            texInfo._minFilter  =   FILTER_LINEAR;
            texInfo._addressMode =  ADDRESS_REPEAT;
            texOk   =   texture->create(texInfo);
            std::cout << "texture create: " << (texOk ? "ok" : "failed") << std::endl;

            if (texOk)
            {
                auto    pixels  =   makeCheckerBoard(kTexSize, kTexSize, 8);
                texOk   =   texture->update(pixels.data(), pixels.size());
                std::cout << "texture upload " << pixels.size() << " bytes: "
                          << (texOk ? "ok" : "failed") << std::endl;
            }
        }
    }

    /// 4. 用 FEGeometryBox 生成立方体 mesh(pos + uv)
    bool    passed  =   false;
    {
        FEGeometryBox   geo(ctx);
        geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
        geo.param()._segs  =   uint3(1, 1, 1);

        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

        /// 校验几何:位置/uv 缓冲顶点数、属性缓冲数、绘制图元数
        uint    posCount    =   0;
        uint    uvCount     =   0;
        if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
            posCount    =   p->count();
        if (auto* uv = mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0)))
            uvCount     =   uv->count();
        std::cout << "box buffers=" << mesh->buffers().size()
                  << " pos=" << posCount
                  << " uv=" << uvCount
                  << " primitives=" << mesh->primitives().size() << std::endl;

        passed  =   texOk
                 && (mesh->buffers().size() >= 2)
                 && (posCount > 0)
                 && (uvCount == posCount)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            /// 纹理材质:棋盘格贴图
            FEMaterialTexture*  pMat    =   new FEMaterialTexture(ctx);
            pMat->setTexture(texture);
            /// 校验管线已加载(texture/TRIANGLES)
            if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
            {
                std::cerr << "pipeline texture/TRIANGLES not found" << std::endl;
                passed  =   false;
            }
            node->setMaterial(pMat);
            /// 不旋转模型,方便观察各面贴图方向(用户验证要求)
            node->makeDirty();
            node->update();
            /// 分发到渲染工厂并显示
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "texture box validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "texture box ok, entering render loop..." << std::endl;

    /// 5. 进入渲染循环显示带贴图的立方体
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

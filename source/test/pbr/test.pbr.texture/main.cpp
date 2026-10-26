/// test.pbr.texture
/// PBR 材质贴图支持测试(Vulkan):
///   1. 创建引擎 + 渲染窗口
///   2. 程序生成 5 张贴图(baseColor / metallicRoughness / normal / occlusion / emissive)
///   3. 用 FEGeometrySphere 按 pos/nor/uv 生成球体 mesh
///   4. 经 FEMaterialPBR 设置全套贴图,验证金属度-粗糙度 PBR 渲染
///   5. 环境图使用与 glTF-Sample-Viewer 参考查看器默认环境相同的
///      Cannon_Exterior.hdr(equirect,线性空间),运行时转换为 float cubemap
/// 验证点:
///   a. 球体表面呈现 baseColor 棋盘格(说明 uv 采样正确)
///   b. 高光区域显示金属色(金属工作流:F0=baseColor)
///   c. 法线贴图产生凹凸细节
///   d. AO 使边缘变暗
///   e. emissive 图案自发光
///   f. IBL:球体反射 Cannon 户外环境,背光面被环境照亮
/// 用法: test.pbr.texture.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <cstdint>
#include    <cstring>
#include    <cstdio>
#include    <cmath>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FETexture.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/component/FESkyComponent.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"

/// 公共天空参数调试面板(与 test.sky 共用)
#include    "../../common/ImGuiSkyPanel.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    constexpr uint kTexSize = 256;

    /// 生成 baseColor:彩色棋盘格
    std::vector<uint8_t> makeBaseColor()
    {
        std::vector<uint8_t> data(size_t(kTexSize) * kTexSize * 4);
        const uint cells = 8;
        uint cw = kTexSize / cells, ch = kTexSize / cells;
        for (uint y = 0; y < kTexSize; ++y)
            for (uint x = 0; x < kTexSize; ++x)
            {
                bool odd = ((x / cw) + (y / ch)) & 1;
                size_t i = (size_t(y) * kTexSize + x) * 4;
                if (odd) { data[i]=0xE2; data[i+1]=0x7A; data[i+2]=0x2A; }  /// 橙色
                else     { data[i]=0x2A; data[i+1]=0x9E; data[i+2]=0xE2; }  /// 青色
                data[i+3] = 0xFF;
            }
        return data;
    }

    /// 生成 metallicRoughness:R 未用,G=roughness(变化),B=metallic(全 0,介电体)
    std::vector<uint8_t> makeMetallicRoughness()
    {
        std::vector<uint8_t> data(size_t(kTexSize) * kTexSize * 4);
        for (uint y = 0; y < kTexSize; ++y)
            for (uint x = 0; x < kTexSize; ++x)
            {
                size_t i = (size_t(y) * kTexSize + x) * 4;
                float fx = (float)x / kTexSize;
                float fy = (float)y / kTexSize;
                float rough = 0.15f + 0.7f * (fx * 0.5f + fy * 0.5f);  /// 0.15~0.85
                data[i]   = 0;                                    /// R
                data[i+1] = (uint8)(clamp(rough,0.f,1.f) * 255.f + 0.5f);  /// G = roughness
                data[i+2] = 0;                                    /// B = metallic(介电)
                data[i+3] = 255;
            }
        return data;
    }

    /// 生成法线贴图:正弦波凹凸
    std::vector<uint8_t> makeNormal()
    {
        std::vector<uint8_t> data(size_t(kTexSize) * kTexSize * 4);
        for (uint y = 0; y < kTexSize; ++y)
            for (uint x = 0; x < kTexSize; ++x)
            {
                size_t i = (size_t(y) * kTexSize + x) * 4;
                float fx = (float)x / kTexSize * 6.2831853f * 6.0f;
                float fy = (float)y / kTexSize * 6.2831853f * 6.0f;
                /// 高度场 h = sin(fx)*sin(fy),求梯度得到切线空间法线
                float h  = sinf(fx) * sinf(fy);
                float dx = cosf(fx) * sinf(fy) * 0.15f;
                float dy = sinf(fx) * cosf(fy) * 0.15f;
                float nx = -dx, ny = -dy, nz = 1.0f;
                float len = sqrtf(nx*nx + ny*ny + nz*nz);
                nx /= len; ny /= len; nz /= len;
                data[i]   = (uint8)((nx * 0.5f + 0.5f) * 255.f + 0.5f);
                data[i+1] = (uint8)((ny * 0.5f + 0.5f) * 255.f + 0.5f);
                data[i+2] = (uint8)((nz * 0.5f + 0.5f) * 255.f + 0.5f);
                data[i+3] = 255;
                (void)h;
            }
        return data;
    }

    /// 生成 occlusion:径向,边缘暗
    std::vector<uint8_t> makeOcclusion()
    {
        std::vector<uint8_t> data(size_t(kTexSize) * kTexSize * 4);
        float cx = kTexSize * 0.5f, cy = kTexSize * 0.5f;
        for (uint y = 0; y < kTexSize; ++y)
            for (uint x = 0; x < kTexSize; ++x)
            {
                size_t i = (size_t(y) * kTexSize + x) * 4;
                float dx = ((float)x - cx) / cx;
                float dy = ((float)y - cy) / cy;
                float d  = sqrtf(dx*dx + dy*dy);
                float ao = clamp(1.0f - d * 0.8f, 0.2f, 1.0f);
                data[i] = data[i+1] = data[i+2] = (uint8)(ao * 255.f + 0.5f);
                data[i+3] = 255;
            }
        return data;
    }

    /// 生成 emissive:中心亮斑自发光
    std::vector<uint8_t> makeEmissive()
    {
        std::vector<uint8_t> data(size_t(kTexSize) * kTexSize * 4, 0);
        float cx = kTexSize * 0.5f, cy = kTexSize * 0.5f;
        for (uint y = 0; y < kTexSize; ++y)
            for (uint x = 0; x < kTexSize; ++x)
            {
                size_t i = (size_t(y) * kTexSize + x) * 4;
                float dx = ((float)x - cx) / cx;
                float dy = ((float)y - cy) / cy;
                float d  = sqrtf(dx*dx + dy*dy);
                float e  = clamp(1.0f - d * 2.5f, 0.0f, 1.0f);
                e = e * e;
                data[i]   = (uint8)(e * 255.f * 0.3f);          /// R
                data[i+1] = (uint8)(e * 255.f * 0.9f);          /// G(偏黄白)
                data[i+2] = (uint8)(e * 255.f * 1.0f);          /// B
                data[i+3] = 255;
            }
        return data;
    }

    /// 由程序化像素填入 FEImage
    ImagePtr makeImage(FEContext& ctx, const std::vector<uint8_t>& pixels)
    {
        ImagePtr image = new FEImage(ctx, kTexSize, kTexSize, FMT_R8G8B8A8_UNORM);
        auto plane = image->plane(0, 0);
        const size_t rowByte = size_t(kTexSize) * 4;
        for (uint y = 0; y < kTexSize; ++y)
            std::memcpy(plane.dataOffset<uint8>(y, 0), pixels.data() + size_t(y) * rowByte, rowByte);
        return image;
    }
    /// 由 FEImage 创建 GPU 纹理(FETextureHelper)
    TexPtr uploadTexture(FEContext& ctx, const std::vector<uint8_t>& pixels)
    {
        return FETextureHelper::create(ctx, *makeImage(ctx, pixels),
                                       FILTER_LINEAR, FILTER_LINEAR, ADDRESS_REPEAT);
    }
}

int main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    /// 天空参数面板启动显隐(命令行控制;运行时仍可用 setPanelVisible 切换)
    bool    skyPanel    =   true;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0 || _stricmp(arg,"/webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if (_stricmp(arg,"--no-sky-panel") == 0 || _stricmp(arg,"-no-sky-panel") == 0)
            skyPanel    =   false;
        else if (_stricmp(arg,"--sky-panel") == 0 || _stricmp(arg,"-sky-panel") == 0)
            skyPanel    =   true;
    }
    FEContext& ctx = FEContext::instance();
    FEScene* pScene = nullptr;
    /// 公共天空参数调试面板:仅 Vulkan 后端有 ImGui 渲染实现
    FEImGuiSkyPanel panel;

    FEApp::CreateInfo info{};
    info._width = 1280; info._height = 720;
    info._appInst = GetModuleHandle(nullptr);
    info._notify = [&](const FEMessage& msg)
    {
        /// 面板先把鼠标消息喂给 ImGui,并在其捕获鼠标时阻止场景响应
        const bool blockScene = panel.onMessage(msg);
        if (!blockScene && pScene)
            pScene->onMessage(msg);
    };
    App app = FEAppHelper::create(ctx, info);
    if (!app) { std::cerr << "create app failed\n"; return 1; }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene scene = new FEScene(ctx);
    pScene = scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed\n";
        scene = nullptr; app->destroy(); return 1;
    }

    /// ---- 天空参数调试面板(公共实现)----
    /// 命令行控制启动显隐;运行时仍可通过 FESkyComponent::setPanelVisible() 切换。
    /// 先同步天空的太阳光强度,使面板调整参数时不会把主方向光重置为默认亮度
    if (scene->sky() != nullptr)
    {
        scene->sky()->setSunLightIntensity(2.5f);
        scene->sky()->setPanelVisible(skyPanel);
    }

    /// 调亮默认平行光并调整方向,使 PBR 光照层次清晰可见
    {
        Light mainLight;
        if (ctx.device().lights().query("main", mainLight) && mainLight)
        {
            auto dir = mainLight->as<FELightDir>();
            if (dir)
            {
                dir->_color = float3(2.5f, 2.5f, 2.5f);
                dir->_dir   = float3(0.4f, 0.7f, 0.8f);  /// 斜上方照射
            }
        }
    }

    /// 生成并上传 5 张贴图;baseColor 额外走一遍
    /// "保存图片到文件 -> 从文件加载图像 -> 创建纹理" 的完整链路
    /// (验证 FEImageHelper::save/load + FETextureHelper::load;PNG 无损,渲染结果不变)
    TexPtr baseColor;
    {
        const char* file = "./pbr_basecolor_tmp.png";
        ImagePtr bcImg = makeImage(ctx, makeBaseColor());
        const bool saved = FEImageHelper::save(*bcImg, file);
        ImagePtr bcBack = FEImageHelper::load(ctx, file);
        /// 逐行比对:确认"保存->加载"后贴图内容与源图像完全一致(PNG 无损)
        bool identical = false;
        if (bcBack != nullptr)
        {
            identical = true;
            auto a = bcImg->plane(0, 0);
            auto b = bcBack->plane(0, 0);
            const size_t rowByte = size_t(kTexSize) * 4;
            for (uint y = 0; identical && y < kTexSize; ++y)
                identical = std::memcmp(a.dataOffset<uint8>(y, 0),
                                        b.dataOffset<uint8>(y, 0), rowByte) == 0;
        }
        baseColor = (bcBack != nullptr)
                    ? FETextureHelper::create(ctx, *bcBack, FILTER_LINEAR, FILTER_LINEAR, ADDRESS_REPEAT)
                    : nullptr;
        std::cout << "baseColor via file: save=" << (saved ? "ok" : "FAILED")
                  << " load=" << ((bcBack != nullptr) ? "ok" : "FAILED")
                  << " pixelsIdentical=" << (identical ? "yes" : "NO")
                  << " loadTexture=" << ((baseColor != nullptr) ? "ok" : "FAILED") << "\n";
        if (baseColor == nullptr)
        {
            std::cerr << "load baseColor texture failed\n";
            scene = nullptr; app->destroy(); return 1;
        }
        /// 环境贴图需要文件保留,baseColor 临时文件用后即删
        std::remove(file);
    }
    TexPtr mr        = uploadTexture(ctx, makeMetallicRoughness());
    TexPtr normal    = uploadTexture(ctx, makeNormal());
    TexPtr ao        = uploadTexture(ctx, makeOcclusion());
    TexPtr emissive  = uploadTexture(ctx, makeEmissive());

    /// 球体 mesh(pos/nor/uv)
    FEGeometrySphere geo(ctx);
    geo.param().radius = 1.0f;
    geo.param().widthSegments  = 48;
    geo.param().heightSegments = 32;
    Mesh mesh = geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    if (mesh->buffers().size() < 3 || mesh->primitives().empty())
    {
        std::cerr << "sphere build failed\n";
        scene = nullptr; app->destroy(); return 1;
    }

    Node node = new FENode(ctx);
    node->setMesh(mesh);

    /// PBR 材质:设置全套贴图
    auto* pMat = new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  = float4(1, 1, 1, 1);   /// baseColor 因子=白,完全由贴图决定
    pMat->data()._value._metallic  = 0.0f;   /// 介电体,baseColor 走漫反射
    pMat->data()._value._roughness = 1.0f;
    pMat->data()._value._emissive  = float4(1, 1, 1, 1);
    pMat->setBaseColorMap(baseColor);
    pMat->setMetallicRoughnessMap(mr);
    pMat->setNormalMap(normal);
    pMat->setOcclusionMap(ao);
    pMat->setEmissiveMap(emissive);
    pMat->setNormalScale(1.0f);
    pMat->setOcclusionStrength(1.0f);
    pMat->setEmissiveStrength(0.3f);
    pMat->data().update();

    node->setMaterial(pMat);
    node->makeDirty();
    node->update();
    scene->dispatchNodesToSystem({node});

    /// 环境图(IBL):与参考查看器默认环境相同的 Cannon_Exterior.hdr,
    /// 经 FEImageHelper::load 加载为 float equirect,再由 FETextureHelper::createCube
    /// 转成 float cubemap(漫反射 irradiance + 按粗糙度的镜面预滤波)
    const String hdrFile = ctx.resourcePath() + "/data/env/Cannon_Exterior.hdr";
    ImagePtr envImage = FEImageHelper::load(ctx, hdrFile.c_str());
    if (envImage == nullptr)
    {
        std::cerr << "load environment hdr failed: " << hdrFile << "\n";
        scene = nullptr; app->destroy(); return 1;
    }
    std::cout << "hdr equirect " << envImage->cInfo()._extent.x << "x" << envImage->cInfo()._extent.y
              << " format=" << FEFormatHelper::enumName(envImage->cInfo()._format) << "\n";
    auto envCube = FETextureHelper::createCube(ctx, *envImage, 256);
    if (envCube == nullptr)
    {
        std::cerr << "create environment cube failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    pMat->setEnvironmentMap(envCube);

    /// 资源就绪后再挂接调试面板:此前的失败分支不会遗留 ImGui 后端
    if (rendererId == RS_VULKAN)
        panel.attach(ctx, scene.get(), true);

    std::cout << "pbr texture test ready, entering render loop...\n";
    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的
    /// 场景外 GPU 资源——node→材质→纹理(与 main 局部 TexPtr 共同持有)、
    /// mesh、envCube;否则它们在 run() 返回后才析构,会访问已销毁的 device
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        /// ImGui 后端持有 device 句柄,必须抢在场景销毁 device 之前释放
        panel.detach();
        node        =   nullptr;
        mesh        =   nullptr;
        envCube     =   nullptr;
        baseColor   =   nullptr;
        mr          =   nullptr;
        normal      =   nullptr;
        ao          =   nullptr;
        emissive    =   nullptr;
    });
    app->run();

    panel.detach();
    scene = nullptr;
    app->destroy();
    return 0;
}

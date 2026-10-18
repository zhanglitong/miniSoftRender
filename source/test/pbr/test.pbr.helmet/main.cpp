/// test.pbr.helmet
/// 用 glTF 官方示例模型 DamagedHelmet 验证 FEMaterialPBR 的全套贴图链路:
///   1. 创建引擎 + 渲染窗口(Vulkan)
///   2. 经 FEFormatGLTFReader 加载 DamagedHelmet.gltf(网格 + 材质 + 贴图)
///   3. glTF 材质的 baseColor/metallicRoughness/normal/occlusion/emissive
///      由解析器自动创建纹理并设置到 FEMaterialPBR
///   4. 环境图使用与 glTF-Sample-Viewer 参考查看器默认环境相同的
///      Cannon_Exterior.hdr(equirect,线性空间),运行时转换为 float cubemap
/// 验证点:
///   a. 头盔呈现 albedo 贴图细节(说明纹理加载与 UV 采样正确)
///   b. 金属/粗糙度随贴图变化(高光强弱分布)
///   c. 法线贴图产生表面凹凸
///   d. AO 使凹槽/接缝处变暗
///   e. emissive 区域自发光
///   f. IBL:金属反射 Cannon 户外环境,背光面被环境照亮
/// 用法: test.pbr.helmet.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <cmath>
#include    <cstring>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 递归为节点树上的所有 FEMaterialPBR 设置环境贴图
    void    applyEnvMap(const Node& node,TexPtr env)
    {
        if (node == nullptr)
            return;
        auto    mat =   node->material();
        if (mat != nullptr)
        {
            auto    pbr =   mat->as<FEMaterialPBR>();
            if (pbr != nullptr)
                pbr->setEnvironmentMap(env);
        }
        for (auto& child : node->children())
            applyEnvMap(child, env);
    }
}

int main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0 || _stricmp(arg,"/webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
    }
    FEContext& ctx = FEContext::instance();
    FEScene* pScene = nullptr;

    FEApp::CreateInfo info{};
    info._width = 1280; info._height = 720;
    info._appInst = GetModuleHandle(nullptr);
    info._notify = [&](const FEMessage& msg) { if (pScene) pScene->onMessage(msg); };
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

    /// 加载 DamagedHelmet.gltf(网格/材质/贴图全部由解析器完成)
    const String gltfFile = R"(D:\study\glTF-Sample-Models\2.0\DamagedHelmet\glTF\DamagedHelmet.gltf)";
    FEFileFormat fmt(".gltf", "1.0.0.0", "GLTF text Format!");
    auto reader = FEFileFormatHelper::queryReader(ctx, fmt);
    if (!reader)
    {
        std::cerr << "query gltf reader failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    auto objects = reader->readFiles({gltfFile});
    Nodes nodes;
    for (auto var : objects)
    {
        Node node = var->cast<FENode>();
        if (node != nullptr)
            nodes.push_back(node);
    }
    if (nodes.empty())
    {
        std::cerr << "load gltf failed: " << gltfFile << "\n";
        scene = nullptr; app->destroy(); return 1;
    }

    scene->dispatchNodesToSystem(nodes);
    scene->addNodesToTree(nodes);

    /// 从参考查看器默认环境 HDR 生成 cubemap,挂到头盔所有 PBR 材质上(IBL):
    /// FEImageHelper::load 加载 HDR 为 float equirect -> FETextureHelper::createCube 转 cubemap
    const String hdrFile = ctx.resourcePath() + "/data/env/Cannon_Exterior.hdr";
    ImagePtr envImage = FEImageHelper::load(ctx, hdrFile.c_str());
    if (envImage == nullptr)
    {
        std::cerr << "load environment hdr failed: " << hdrFile << "\n";
        scene = nullptr; app->destroy(); return 1;
    }
    std::cout << "hdr equirect " << envImage->cInfo()._extent.x << "x" << envImage->cInfo()._extent.y
              << " format=" << FEFormatHelper::enumName(envImage->cInfo()._format) << "\n";
    auto    envCube =   FETextureHelper::createCube(ctx, *envImage, 256);
    if (envCube == nullptr)
    {
        std::cerr << "create environment cube failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    for (auto& node : nodes)
        applyEnvMap(node, envCube);

    std::cout << "pbr helmet test ready, entering render loop...\n";
    app->run();

    scene = nullptr;
    app->destroy();
    return 0;
}

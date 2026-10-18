/// test.pbr.carconcept
/// 用 glTF 官方示例模型 CarConcept 验证 FEMaterialPBR 的 KHR 材质扩展链路:
///   1. 创建引擎 + 渲染窗口(--vulkan / --webgpu 双后端)
///   2. 经 FEFormatGLTFReader 加载 CarConcept.gltf(网格 + 材质 + 贴图)
///   3. 验证扩展材质:
///      a. KHR_materials_clearcoat  车漆清漆层高光
///      b. KHR_materials_iridescence 虹彩(薄膜干涉)变色
///      c. KHR_materials_transmission 玻璃透射(alpha 混合近似)
///      d. doubleSided 双面渲染(pbr2s 管线)
///      e. occlusionTexture.texCoord==1 → AO 采样 TEXCOORD_1
///   4. KHR_materials_variants 车漆变体切换(本模型 3 种):
///      按 C 键循环变体,按数字键直接选择
///   5. 真实 HDR 环境贴图 IBL(equirect -> 预滤波 cubemap + 球谐 irradiance cubemap):
///      按 E 键循环 pisa / Cannon_Exterior / Colorful_Studio(与 glTF-Sample-Viewer
///      assets 同源),默认 Cannon_Exterior —— 与官方查看器默认环境一致
/// 用法: test.pbr.carconcept.exe [--vulkan|--webgpu]

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
#include    "../../../inc/fileFormat/gltf/FEFormatGLTFReader.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 可切换的 HDR 环境列表(bin/data/env 下,与 glTF-Sample-Viewer assets 同源)
    struct  EnvEntry
    {
        const char* name;
        const char* file;
    };
    const   EnvEntry    kEnvs[] =
    {
        { "pisa",            "/data/env/pisa.hdr"            },
        { "Cannon_Exterior", "/data/env/Cannon_Exterior.hdr" },
        { "Colorful_Studio", "/data/env/Colorful_Studio.hdr" },
    };

    /// 当前环境 IBL 资源缓存(applyEnvironment 生成/更新,
    /// reapplyEnvironment 在变体切换后复用挂载到新材质,不重建 cubemap)
    TexPtr  g_envCube, g_irrCube, g_brdfLUT;
    /// 场景指针(transmission 场景折射纹理由 FEScene 持有,变体切换后补挂新材质)
    FEScene*    g_scene =   nullptr;

    /// 递归为节点树上的所有 FEMaterialPBR 设置环境贴图(镜面预滤波 + 漫反射辐照度 + BRDF LUT)
    /// 同时补挂 transmission 场景折射纹理(变体切换后的新材质只有占位纹理)
    void    applyEnvMap(const Node& node,TexPtr env,TexPtr irradiance,TexPtr brdfLUT)
    {
        if (node == nullptr)
            return;
        auto    mat =   node->material();
        if (mat != nullptr)
        {
            auto    pbr =   mat->as<FEMaterialPBR>();
            if (pbr != nullptr)
            {
                pbr->setEnvironmentMap(env,irradiance);
                pbr->setBRDFLUT(brdfLUT);
                if (g_scene && g_scene->sceneTexture())
                    pbr->setSceneTexture(g_scene->sceneTexture());
            }
        }
        for (auto& child : node->children())
            applyEnvMap(child, env, irradiance, brdfLUT);
    }

    /// 加载 HDR 并生成 IBL 资源挂到全部材质上;失败返回 nullptr。
    /// 资源更新到 g_envCube/g_irrCube/g_brdfLUT 缓存,供 reapplyEnvironment 复用
    TexPtr  applyEnvironment(FEContext& ctx,Nodes& rootNodes,int envIdx)
    {
        const String    hdrFile =   ctx.resourcePath() + kEnvs[envIdx].file;
        ImagePtr        envImage    =   FEImageHelper::load(ctx,hdrFile.c_str());
        if (envImage == nullptr)
        {
            std::cerr << "load environment hdr failed: " << hdrFile << "\n";
            return  nullptr;
        }
        /// 镜面预滤波 cubemap(带 mip 链,512 保证低粗糙度反射清晰)
        g_envCube   =   FETextureHelper::createCube(ctx,*envImage,512);
        if (g_envCube == nullptr)
        {
            std::cerr << "create environment cube failed\n";
            return  nullptr;
        }
        /// 漫反射辐照度 cubemap(CPU 2 阶球谐 + 余弦卷积解析式,平滑无噪声)
        g_irrCube   =   FETextureHelper::createIrradianceCube(ctx,*envImage,32);
        if (g_irrCube == nullptr)
        {
            std::cerr << "create irradiance cube failed\n";
            return  nullptr;
        }
        /// split-sum BRDF 积分 LUT(与环境无关,全局一份,首次调用时生成)
        if (g_brdfLUT == nullptr)
        {
            g_brdfLUT   =   FETextureHelper::createBRDFLUT(ctx,512);
            if (g_brdfLUT == nullptr)
            {
                std::cerr << "create brdf lut failed\n";
                return  nullptr;
            }
        }
        for (auto& node : rootNodes)
            applyEnvMap(node, g_envCube, g_irrCube, g_brdfLUT);
        std::cout << "environment: " << kEnvs[envIdx].name << " ("
                  << envImage->cInfo()._extent.x << "x" << envImage->cInfo()._extent.y << ")\n";
        return  g_envCube;
    }

    /// 变体切换后把当前环境资源挂到新换上的材质(复用缓存,不重建 cubemap);
    /// 环境未初始化时(启动流程尚未调 applyEnvironment)不做任何事
    void    reapplyEnvironment(Nodes& rootNodes)
    {
        if (g_envCube == nullptr)
            return;
        for (auto& node : rootNodes)
            applyEnvMap(node, g_envCube, g_irrCube, g_brdfLUT);
    }

    /// 打印变体切换信息
    void    printVariant(FEFormatGLTFReader* pReader,int idx)
    {
        auto&   names   =   pReader->variantNames();
        if (idx >= 0 && idx < (int)names.size())
            std::cout << "variant[" << idx << "] = " << names[idx] << "\n";
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

    /// 变体切换状态(reader 必须存活,变体映射存储在其中)
    FEFormatGLTFReader* pGltfReader =   nullptr;
    Nodes               rootNodes;
    int                 curVariant  =   0;
    int                 curEnv      =   1;    /// 默认 Cannon_Exterior(与官方 glTF-Sample-Viewer 默认环境一致)
    constexpr int       kEnvCount   =   int(sizeof(kEnvs) / sizeof(kEnvs[0]));

    FEApp::CreateInfo info{};
    info._width = 1280; info._height = 720;
    info._appInst = GetModuleHandle(nullptr);   /// WebGPU surface 创建需要正确的 HINSTANCE
    info._notify = [&](const FEMessage& msg)
    {
        if (pScene) pScene->onMessage(msg);
        if (msg.msgId() == MSG_KEYDOWN && pGltfReader != nullptr)
        {
            auto&   keyMsg  =   static_cast<const MsgKeyDown&>(msg);
            auto&   names   =   pGltfReader->variantNames();
            int     count   =   (int)names.size();
            if (keyMsg._info._key == 'C' && count > 0)
            {
                curVariant  =   (curVariant + 1) % count;
                pGltfReader->applyVariant(rootNodes,curVariant);
                /// 新换上的材质没有环境贴图,补挂当前 IBL 资源(否则金属漆全黑)
                reapplyEnvironment(rootNodes);
                printVariant(pGltfReader,curVariant);
            }
            else if (keyMsg._info._key >= '0' && keyMsg._info._key < (uint32)('0' + count))
            {
                curVariant  =   keyMsg._info._key - '0';
                pGltfReader->applyVariant(rootNodes,curVariant);
                reapplyEnvironment(rootNodes);
                printVariant(pGltfReader,curVariant);
            }
            else if (keyMsg._info._key == 'E' && !rootNodes.empty())
            {
                /// 循环切换 HDR 环境(IBL 全套重建后挂回所有材质)
                curEnv  =   (curEnv + 1) % kEnvCount;
                if (applyEnvironment(ctx,rootNodes,curEnv) == nullptr)
                    curEnv  =   (curEnv - 1 + kEnvCount) % kEnvCount;
            }
        }
    };
    App app = FEAppHelper::create(ctx, info);
    if (!app) { std::cerr << "create app failed\n"; return 1; }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene scene = new FEScene(ctx);
    pScene = scene.get();
    g_scene = scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed\n";
        scene = nullptr; app->destroy(); return 1;
    }

    /// 调亮默认平行光并调整方向,呈现车漆层次
    {
        Light mainLight;
        if (ctx.device().lights().query("main", mainLight) && mainLight)
        {
            auto dir = mainLight->as<FELightDir>();
            if (dir)
            {
                dir->_color = float3(2.0f, 2.0f, 2.0f);
                dir->_dir   = float3(0.4f, 0.7f, 0.8f);
            }
        }
    }

    /// 加载 CarConcept.gltf(网格/材质/贴图/扩展全部由解析器完成)
    const String gltfFile = R"(D:\study\glTF-Sample-Assets\Models\CarConcept\glTF\CarConcept.gltf)";
    FEFileFormat fmt(".gltf", "1.0.0.0", "GLTF text Format!");
    auto reader = FEFileFormatHelper::queryReader(ctx, fmt);
    if (!reader)
    {
        std::cerr << "query gltf reader failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    auto objects = reader->readFiles({gltfFile});
    for (auto var : objects)
    {
        Node node = var->cast<FENode>();
        if (node != nullptr)
            rootNodes.push_back(node);
    }
    if (rootNodes.empty())
    {
        std::cerr << "load gltf failed: " << gltfFile << "\n";
        scene = nullptr; app->destroy(); return 1;
    }
    pGltfReader = reader->as<FEFormatGLTFReader>();
    std::cout << "variants: ";
    for (auto& name : pGltfReader->variantNames())
        std::cout << name << " ";
    std::cout << "\n";

    scene->dispatchNodesToSystem(rootNodes);
    scene->addNodesToTree(rootNodes);

    /// 干净背景(隐藏编辑网格 + 浅灰清屏,对齐参考截图的影棚观感)
    scene->showGrid(false);
    scene->setClearColor(float4(0.72f, 0.72f, 0.72f, 1.0f));

    /// 3/4 视角相机(CarConcept 车长约 4.6m,原点附近;需 updatePerspective 保持透视投影)
    {
        auto    cam     =   scene->camera();
        if (cam)
        {
            cam->setEye (real3(3.6f, 1.55f, 4.3f));
            cam->setTarget(real3(0.0f, 0.55f, 0.1f));
            cam->setUp  (real3(0,1,0));
            cam->setNear(0.1f);
            cam->setFar (100.0f);
            cam->updatePerspective();
        }
    }

    /// 真实 HDR 环境贴图挂到所有 PBR 材质上(IBL):
    /// 镜面 = equirect -> 512 预滤波 cubemap(mip 链按 roughness 采样)× 512 BRDF LUT
    /// 漫反射 = equirect -> 32 球谐预卷积 irradiance cubemap
    /// 默认 Cannon_Exterior(官方查看器默认环境),按 E 键循环切换
    if (applyEnvironment(ctx,rootNodes,curEnv) == nullptr)
    {
        scene = nullptr; app->destroy(); return 1;
    }

    std::cout << "pbr carconcept test ready, entering render loop...\n";
    {
        auto&   names   =   pGltfReader->variantNames();
        std::cout << "press C to cycle paint variants, 0~" << (names.size() - 1)
                  << " to select directly, E to cycle environments\n";
    }
    app->run();

    scene = nullptr;
    app->destroy();
    return 0;
}

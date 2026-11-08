/// test.skin.fepj.scene
/// 验证「蒙皮场景保存成 fepj 后能重新加载并正确渲染」:
///   1. 用 glTF 读取器加载 Soldier.glb(含 skins),挂到离屏场景渲染一帧作为参照
///   2. FEScene::save 把整棵节点树(含 FESkin / FESkinData)写入 .fepj
///   3. FEScene::clear 清空场景,再 FEScene::open 从 .fepj 重新装载
///   4. 重新渲染并回读像素,校验:
///        a. 关节引用已由 open 内的 resolvePending 回填(无 pending)
///        b. worldRotation 等骨骼数据随场景往返
///        c. 渲染结果与保存前一致(非背景像素数量接近,说明网格确实被画出来且蒙皮姿态相同)
/// 用法: test.skin.fepj.scene.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <algorithm>
#include    <cstdlib>
#include    <filesystem>
#include    <functional>
#include    <Windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/node/FESkin.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    int     g_pass  =   0;
    int     g_fail  =   0;

    void    check(bool ok,const char* label)
    {
        if (ok)     {   ++  g_pass;     printf("[PASS] %s\n",label);    }
        else        {   ++  g_fail;     printf("[FAIL] %s\n",label);    }
        fflush(stdout);
    }
    bool    matNear(const mat4r& a,const mat4r& b,real eps = 1e-4)
    {
        for (int col = 0;col < 4;++col)
        {
            for (int row = 0;row < 4;++row)
            {
                if (std::abs(a[col][row] - b[col][row]) > eps)
                    return  false;
            }
        }
        return  true;
    }
    String  exePath()
    {
        char    szTemp[2048]    =   {};
        GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
        std::filesystem::path   filePath(szTemp);
        return  filePath.parent_path().string();
    }
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
    /// 递归收集节点树上的蒙皮组件
    void    collectSkins(const Node& node,std::vector<FESkin*>& skins)
    {
        if (node == nullptr)
            return;
        for (FESkin* skin : node->objects<FESkin>())
        {
            if (skin != nullptr)
                skins.push_back(skin);
        }
        for (auto& child : node->children())
            collectSkins(child,skins);
    }
    /// 统计与背景色差异明显的像素数量(背景为纯黑清屏色)
    size_t  visiblePixels(const uint8s& pixels)
    {
        size_t  count   =   0;
        for (size_t i = 0;i + 3 < pixels.size();i += 4)
        {
            const   uint    maxValue    =   (std::max)({pixels[i],pixels[i+1],pixels[i+2]});
            if (maxValue > 12)
                ++count;
        }
        return  count;
    }
}

int     main(int argc,char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1;i < argc;++i)
    {
        const   char*   arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
    }

    setvbuf(stdout,nullptr,_IONBF,0);
    printf("=== test.skin.fepj.scene: save / load / render skinned scene ===\n");

    FEContext&  ctx     =   FEContext::instance();
    const String    path    =   exePath();

    OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);
    FEOffscreenRenderer::CreateInfo renderInfo  =   {};
    renderInfo._width       =   256;
    renderInfo._height      =   256;
    renderInfo._rendererId  =   rendererId;
    renderInfo._workPath    =   path;
    renderInfo._resourcePath=   path + "/../";
    if (!renderer->create(renderInfo))
    {
        std::cerr << "FEOffscreenRenderer::create failed" << std::endl;
        renderer    =   nullptr;
        return  1;
    }
    Scene   scene   =   renderer->scene();
    scene->showSky(false);
    scene->setClearColor(float4(0,0,0,1));
    /// 调亮默认平行光,避免模型在纯黑背景下过暗
    {
        Light   mainLight;
        if (ctx.device().lights().query("main",mainLight) && mainLight)
        {
            auto    dir =   mainLight->as<FELightDir>();
            if (dir)
            {
                dir->_color =   float3(3.0f,3.0f,3.0f);
                dir->_dir   =   float3(0.4f,0.7f,0.8f);
            }
        }
    }

    /// ---- 1. 加载 glTF 蒙皮模型并挂到场景 ----
    const   String  glbFile =   path + "/../data/model/Soldier.glb";
    FEFileFormat    fmt(".glb","1.0.0.0","GLTF binary Format!");
    auto    gltfReader  =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(gltfReader != nullptr,"gltf: query reader");
    if (gltfReader == nullptr)
        return  1;

    auto    objects =   gltfReader->readFiles({glbFile});
    Nodes   nodes;
    for (auto var : objects)
    {
        Node    node    =   var->cast<FENode>();
        if (node != nullptr)
            nodes.push_back(node);
    }
    check(!nodes.empty(),"gltf: load Soldier.glb");
    if (nodes.empty())
        return  1;

    /// 环境光照(IBL):保存前后的材质贴图走 fepj,环境贴图属于运行时资源,加载后重新挂一次
    const   String  hdrFile =   path + "/../data/env/Cannon_Exterior.hdr";
    ImagePtr    envImage    =   FEImageHelper::load(ctx,hdrFile.c_str());
    TexPtr      envCube;
    if (envImage != nullptr)
        envCube =   FETextureHelper::createCube(ctx,*envImage,256);
    check(envCube != nullptr,"env: create cube");
    if (envCube != nullptr)
    {
        for (auto& node : nodes)
            applyEnvMap(node,envCube);
    }
    /// glTF 为 Y-up,引擎为 Z-up:蒙皮结果需左乘坐标轴修正,该矩阵存在 FESkinData 上
    const   mat4r   yUpToZUp    =   mat4r(glm::rotate(mat4r(1.0),real(glm::radians(90.0)),real3(1.0,0.0,0.0)));
    {
        std::function<void(Node)>   fixRot  =   [&](Node node)
        {
            if (node == nullptr)
                return;
            Skin    skin    =   skinOf(node.get());
            if (skin != nullptr)
                skin->setWorldRotation(yUpToZUp);
            for (auto& child : node->children())
                fixRot(child);
        };
        for (auto& node : nodes)
            fixRot(node);
    }

    std::vector<FESkin*>    sourceSkins;
    for (auto& node : nodes)
        collectSkins(node,sourceSkins);
    check(!sourceSkins.empty(),"gltf: model has skin");
    if (sourceSkins.empty())
        return  1;
    printf("       gltf skin components = %zu\n",sourceSkins.size());
    check(sourceSkins.size() == 2,"gltf: exactly 2 skin components");
    const   size_t  sourceJoints    =   sourceSkins.front()->data()->jointCount();

    scene->dispatchNodesToSystem(nodes);
    scene->addNodesToTree(nodes);

    /// 每个 glTF 动画(Idle/Run/Walk/TPose)应各自对应一个 FEPlayback
    auto&   playbacks   =   scene->animationSystem()->playbacks();
    printf("       playbacks = %zu\n",playbacks.size());
    for (auto& var : playbacks)
        printf("         playback '%s' -> %zu animations\n",var.first.c_str(),var.second->objects().size());
    check(playbacks.size() == 4,"anim: four playbacks for four clips");
    check(playbacks.find("Soldier_Idle")  != playbacks.end(),"anim: playback Soldier_Idle exists");
    check(playbacks.find("Soldier_Run")   != playbacks.end(),"anim: playback Soldier_Run exists");
    check(playbacks.find("Soldier_Walk")  != playbacks.end(),"anim: playback Soldier_Walk exists");
    check(playbacks.find("Soldier_TPose") != playbacks.end(),"anim: playback Soldier_TPose exists");
    for (auto& var : playbacks)
        check(var.second->objects().size() == 52,"anim: playback holds 52 node clips");
    scene->camera()->setEye(real3(0.0,2.9,1.05));
    scene->camera()->setTarget(real3(0.0,0.0,0.95));
    scene->camera()->setUp(real3(0,0,1));
    scene->camera()->setNearFar(real2(0.05,100.0));
    scene->camera()->updatePerspective();

    /// ---- 2. 渲染参照帧 ----
    check(renderer->render(2),"render: before save");
    const   size_t  pixelsBefore    =   visiblePixels(renderer->pixels());
    printf("       visible pixels before save = %zu\n",pixelsBefore);
    check(pixelsBefore > 256,"render: model visible before save");

    /// ---- 3. 保存整个场景到 .fepj ----
    const   std::filesystem::path   fepjPath    =   std::filesystem::temp_directory_path() / "fe_test_skin_scene.fepj";
    std::filesystem::remove(fepjPath);
    check(scene->save(fepjPath.string().c_str()),"fepj: scene save");
    check(std::filesystem::exists(fepjPath),"fepj: file exists");
    if (!std::filesystem::exists(fepjPath))
        return  1;
    const   uintmax_t   fileSize    =   std::filesystem::file_size(fepjPath);
    printf("       fepj size = %llu bytes\n",(unsigned long long)fileSize);
    check(fileSize > 0,"fepj: file not empty");

    /// ---- 4. 清空场景后从 fepj 重新加载 ----
    scene->clear();
    check(scene->nodeTree().topLevelNodes().empty(),"scene: cleared");
    check(scene->open(fepjPath.string().c_str()),"fepj: scene open");

    const   Nodes&  loadedRoots =   scene->nodeTree().topLevelNodes();
    check(!loadedRoots.empty(),"fepj: nodes loaded");
    if (loadedRoots.empty())
        return  1;

    std::vector<FESkin*>    loadedSkins;
    for (auto& node : loadedRoots)
        collectSkins(node,loadedSkins);
    check(!loadedSkins.empty(),"fepj: skins loaded");
    if (loadedSkins.empty())
        return  1;
    printf("       fepj skin components = %zu\n",loadedSkins.size());
    check(loadedSkins.size() == 2,"fepj: exactly 2 skin components");

    /// 播放器分组随工程往返:仍是 4 个,每个 52 个节点 clip
    auto&   loadedPlaybacks =   scene->animationSystem()->playbacks();
    printf("       fepj playbacks = %zu\n",loadedPlaybacks.size());
    for (auto& var : loadedPlaybacks)
        printf("         playback '%s' -> %zu animations\n",var.first.c_str(),var.second->objects().size());
    check(loadedPlaybacks.size() == 4,"fepj: four playbacks restored");
    for (auto& var : loadedPlaybacks)
        check(var.second->objects().size() == 52,"fepj: playback holds 52 node clips");

    size_t  pendingTotal    =   0;
    bool    worldRotOk      =   true;
    bool    jointsBound     =   true;
    for (FESkin* skin : loadedSkins)
    {
        if (skin->data() == nullptr)
        {
            jointsBound =   false;
            continue;
        }
        pendingTotal    +=  skin->data()->pendingJointCount();
        if (!matNear(skin->data()->worldRotation(),yUpToZUp))
            worldRotOk  =   false;
        for (auto& bone : skin->data()->bones())
        {
            if (bone.node == nullptr)
            {
                jointsBound =   false;
                break;
            }
        }
    }
    /// open() 内部已调用 FESkinData::resolvePending,引用必须全部回填
    check(pendingTotal == 0,"fepj: no pending joints after open");
    check(jointsBound,"fepj: all joint nodes bound");
    check(worldRotOk,"fepj: world rotation restored");
    check(loadedSkins.front()->data()->jointCount() == sourceJoints,"fepj: joint count restored");

    /// 环境贴图是运行时资源,重新挂到加载出来的材质上
    if (envCube != nullptr)
    {
        for (auto& node : loadedRoots)
            applyEnvMap(node,envCube);
    }

    /// ---- 5. 重新渲染并与参照帧比较 ----
    check(renderer->render(2),"render: after open");
    const   size_t  pixelsAfter =   visiblePixels(renderer->pixels());
    printf("       visible pixels after open = %zu\n",pixelsAfter);
    check(pixelsAfter > 256,"render: model visible after open");

    const   double  ratio   =   (pixelsBefore == 0) ? 0.0 : double(pixelsAfter) / double(pixelsBefore);
    printf("       pixel ratio = %.3f\n",ratio);
    check(ratio > 0.6 && ratio < 1.6,"render: silhouette matches before save");

    std::filesystem::remove(fepjPath);

    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    /// 已知问题:带贴图/PBR 材质的场景在 device 销毁时仍有少量 VkBuffer 未回收
    /// (与本用例的 fepj 存取无关,直接渲染 glTF 模型同样会触发),
    /// 全局析构阶段访问已销毁设备会导致 0xC0000005。为避免掩盖用例结果,这里跳过全局析构。
    std::quick_exit(g_fail == 0 ? 0 : 1);
}

/// test.anim.perf
/// 1000 个 Soldier 实例同时播放动画,统计:
///   1. 克隆/分发耗时
///   2. CPU 动画采样耗时(每帧 FEAnimationSys::update)
///   3. 可选:整帧渲染耗时(W 小分辨率,便于观察 CPU 占比)
/// 用法: test.anim.perf.exe [--count=1000] [--frames=120] [--render] [--render-frames=5] [--webgpu]

#include    <iostream>
#include    <algorithm>
#include    <chrono>
#include    <cstdlib>
#include    <filesystem>
#include    <functional>
#include    <unordered_map>
#include    <vector>
#include    <Windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/node/FESkin.hpp"

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
    String  exePath()
    {
        char    szTemp[2048]    =   {};
        GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
        std::filesystem::path   filePath(szTemp);
        return  filePath.parent_path().string();
    }
    double  nowMs()
    {
        using   Clock   =   std::chrono::high_resolution_clock;
        return  std::chrono::duration<double,std::milli>(Clock::now().time_since_epoch()).count();
    }
    struct  Timing
    {
        double  _avg    =   0.0;
        double  _min    =   0.0;
        double  _max    =   0.0;
    };
    Timing  summarize(const std::vector<double>& samples)
    {
        Timing  timing;
        if (samples.empty())
            return  timing;
        double  total   =   0.0;
        timing._min =   timing._max  =   samples.front();
        for (double value : samples)
        {
            total       +=  value;
            timing._min =   (std::min)(timing._min,value);
            timing._max =   (std::max)(timing._max,value);
        }
        timing._avg =   total / double(samples.size());
        return  timing;
    }
    using   NodeMap =   std::unordered_map<const FENode*,Node>;

    /// 建立 "源节点 -> 克隆节点" 的映射(克隆树的子节点顺序与源一致)
    void    mapTree(const Node& src,const Node& dst,NodeMap& map)
    {
        map[src.get()]  =   dst;
        const   auto&   srcChildren =   src->children();
        const   auto&   dstChildren =   dst->children();
        const   size_t  count       =   (std::min)(srcChildren.size(),dstChildren.size());
        for (size_t i = 0;i < count;++i)
            mapTree(srcChildren[i],dstChildren[i],map);
    }
    /// 拷贝构造只克隆子节点列表,不会回填 parent 指针;
    /// 清空后重新 addChild,保证渲染工厂遍历时 parent 有效
    void    reparent(Node node)
    {
        Nodes   children    =   node->children();
        node->children().clear();
        for (auto& child : children)
        {
            node->addChild(child);
            reparent(child);
        }
    }
    /// 克隆出来的对象没有 id(FEObject 拷贝构造不复制 id),这里重新生成
    void    renewIds(Node node)
    {
        node->setObjectId(FEUuid::create());
        for (Component com : node->components())
        {
            if (com != nullptr)
                com->setObjectId(FEUuid::create());
        }
        for (auto& child : node->children())
            renewIds(child);
    }
    /// 克隆的 FESkin 与源共享 FESkinData,骨骼仍指向源骨架;
    /// 按映射重建一份,让每个实例驱动自己的节点树
    void    rebuildSkins(Node node,const NodeMap& map,FEContext& ctx)
    {
        for (FESkin* skin : node->objects<FESkin>())
        {
            if (skin == nullptr || skin->data() == nullptr)
                continue;
            SkinData    old     =   skin->data();
            SkinData    data    =   new FESkinData(ctx);
            data->setWorldRotation(old->worldRotation());
            Bones   bones;
            bones.reserve(old->bones().size());
            for (const FEBone& src : old->bones())
            {
                FEBone  bone;
                bone.inverseBind    =   src.inverseBind;
                auto    itr         =   map.find(src.node.get());
                bone.node           =   (itr != map.end()) ? itr->second : src.node;
                bones.push_back(bone);
            }
            data->setBones(bones);
            skin->setData(data);
        }
        for (auto& child : node->children())
            rebuildSkins(child,map,ctx);
    }
}

int     main(int argc,char** argv)
{
    size_t  instanceCount   =   1000;
    int     frameCount      =   120;
    int     renderFrames    =   5;
    bool    doRender        =   false;
    FEUuid  rendererId      =   RS_VULKAN;

    for (int i = 1;i < argc;++i)
    {
        const   char*   arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if (_stricmp(arg,"--render") == 0)
            doRender    =   true;
        else if (_strnicmp(arg,"--count=",8) == 0)
            instanceCount   =   size_t(_atoi64(arg + 8));
        else if (_strnicmp(arg,"--frames=",9) == 0)
            frameCount      =   _atoi64(arg + 9);
        else if (_strnicmp(arg,"--render-frames=",16) == 0)
            renderFrames    =   _atoi64(arg + 16);
    }
    instanceCount   =   (std::max)(size_t(1),instanceCount);
    frameCount      =   (std::max)(1,frameCount);
    renderFrames    =   (std::max)(1,renderFrames);

    setvbuf(stdout,nullptr,_IONBF,0);
    printf("=== test.anim.perf: %zu instances x Soldier.glb ===\n",instanceCount);

    FEContext&  ctx     =   FEContext::instance();
    const   String  path    =   exePath();

    OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);
    FEOffscreenRenderer::CreateInfo renderInfo  =   {};
    /// 小分辨率:光栅化开销尽量小,让整帧耗时主要由 CPU 动画+蒙皮提交决定
    renderInfo._width       =   128;
    renderInfo._height      =   128;
    renderInfo._rendererId  =   rendererId;
    renderInfo._workPath    =   path;
    renderInfo._resourcePath=   path + "/../";
    if (!renderer->create(renderInfo))
    {
        std::cerr << "FEOffscreenRenderer::create failed" << std::endl;
        return  1;
    }
    Scene   scene   =   renderer->scene();
    scene->showSky(false);
    scene->setClearColor(float4(0,0,0,1));
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

    /// ---- 1. 加载一次模型 ----
    const   String  glbFile =   path + "/../data/model/Soldier.glb";
    FEFileFormat    fmt(".glb","1.0.0.0","GLTF binary Format!");
    auto    gltfReader  =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(gltfReader != nullptr,"gltf: query reader");
    if (gltfReader == nullptr)
        return  1;

    auto    objects =   gltfReader->readFiles({glbFile});
    Nodes   sources;
    for (auto var : objects)
    {
        Node    node    =   var->cast<FENode>();
        if (node != nullptr)
            sources.push_back(node);
    }
    check(!sources.empty(),"gltf: load Soldier.glb");
    if (sources.empty())
        return  1;

    /// glTF 为 Y-up,引擎为 Z-up
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
        for (auto& node : sources)
            fixRot(node);
    }

    /// ---- 2. 克隆实例 ----
    const   Node&   sourceRoot  =   sources.front();
    const   double  cloneBegin  =   nowMs();
    Nodes   roots;
    roots.reserve(instanceCount);
    roots.push_back(sourceRoot);
    for (size_t i = 1;i < instanceCount;++i)
    {
        Node    copy    =   new FENode(*sourceRoot);
        NodeMap map;
        mapTree(sourceRoot,copy,map);
        reparent(copy);
        renewIds(copy);
        rebuildSkins(copy,map,ctx);
        roots.push_back(copy);
    }
    const   double  cloneMs =   nowMs() - cloneBegin;
    printf("       clone: %zu instances in %.1f ms (%.2f ms/instance)\n",
           instanceCount,cloneMs,cloneMs / double(instanceCount));

    /// 布局成网格,避免实例完全重叠
    for (size_t i = 0;i < roots.size();++i)
    {
        const   real    col =   real(i % 32);
        const   real    row =   real(i / 32);
        roots[i]->setLocalTranslation(real3(col * 1.6f,row * 1.6f,0.0f));
    }

    /// ---- 3. 分发到系统并挂到场景 ----
    const   double  dispatchBegin   =   nowMs();
    scene->dispatchNodesToSystem(roots);
    scene->addNodesToTree(roots);
    const   double  dispatchMs  =   nowMs() - dispatchBegin;
    printf("       dispatch + add tree: %.1f ms\n",dispatchMs);

    /// ---- 4. 统计 ----
    size_t  nodeCount   =   0;
    size_t  skinCount   =   0;
    size_t  animCount   =   0;
    {
        std::function<void(const Node&)>    walk    =   [&](const Node& node)
        {
            if (node == nullptr)
                return;
            ++nodeCount;
            skinCount   +=  node->objects<FESkin>().size();
            animCount   +=  node->objects<FEAnimation>().size();
            for (auto& child : node->children())
                walk(child);
        };
        for (auto& root : roots)
            walk(root);
    }
    printf("       nodes = %zu, skins = %zu, animations = %zu\n",nodeCount,skinCount,animCount);
    check(nodeCount == 68 * instanceCount,"perf: node count");
    check(skinCount == 2 * instanceCount,"perf: skin count");
    check(animCount == 208 * instanceCount,"perf: animation count");

    auto&   playbacks   =   scene->animationSystem()->playbacks();
    printf("       playbacks = %zu\n",playbacks.size());
    check(playbacks.size() == 4,"perf: four playbacks");
    for (auto& var : playbacks)
    {
        printf("         playback '%s' -> %zu animations\n",var.first.c_str(),var.second->objects().size());
        check(var.second->objects().size() == 52 * instanceCount,"perf: playback holds all instances");
    }

    /// 俯视整个网格
    scene->camera()->setEye(real3(24.0,24.0,60.0));
    scene->camera()->setTarget(real3(24.0,24.0,0.0));
    scene->camera()->setUp(real3(0.0,0.0,1.0));
    scene->camera()->setNearFar(real2(0.1,500.0));
    scene->camera()->updatePerspective();

    /// ---- 5. CPU 动画采样耗时 ----
    auto    idle    =   playbacks.find("Soldier_Idle");
    check(idle != playbacks.end(),"perf: Soldier_Idle exists");
    if (idle == playbacks.end())
        return  1;
    idle->second->play();

    const   real    dt  =   real(1.0 / 60.0);
    for (int i = 0;i < 10;++i)
        scene->animationSystem()->update(dt);

    std::vector<double> samples;
    samples.reserve(size_t(frameCount));
    for (int i = 0;i < frameCount;++i)
    {
        const   double  begin   =   nowMs();
        scene->animationSystem()->update(dt);
        samples.push_back(nowMs() - begin);
    }
    const   Timing  cpuTiming   =   summarize(samples);
    const   double  perInstance =   cpuTiming._avg / double(instanceCount);
    const   double  clipsPerMs  =   double(animCount / 4) / (cpuTiming._avg > 0.0 ? cpuTiming._avg : 1.0);
    printf("       [cpu] animation update: avg %.3f ms, min %.3f ms, max %.3f ms  (%.0f clips/ms)\n",
           cpuTiming._avg,cpuTiming._min,cpuTiming._max,clipsPerMs);
    printf("       [cpu] per instance: %.4f ms, %.1f instances fit in a 16.67 ms frame\n",
           perInstance,perInstance > 0.0 ? 16.67 / perInstance : 0.0);
    check(idle->second->status() == FEPlayback::PS_Running,"perf: playback running");

    /// ---- 6. 整帧渲染耗时(可选) ----
    if (doRender)
    {
        std::vector<double> frameSamples;
        frameSamples.reserve(size_t(renderFrames));
        for (int i = 0;i < renderFrames;++i)
        {
            const   double  begin   =   nowMs();
            const   bool    ok      =   renderer->render(1);
            frameSamples.push_back(nowMs() - begin);
            if (!ok)
            {
                check(false,"render: frame failed");
                break;
            }
        }
        const   Timing  frameTiming =   summarize(frameSamples);
        printf("       [gpu] full frame: avg %.3f ms, min %.3f ms, max %.3f ms @128x128\n",
               frameTiming._avg,frameTiming._min,frameTiming._max);
        check(!frameSamples.empty(),"render: frames rendered");
    }
    else
    {
        printf("       (skip render; pass --render to measure full frame time)\n");
    }

    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    /// 已知问题:带贴图/PBR 材质的场景在 device 销毁时仍有少量 VkBuffer 未回收,
    /// 全局析构阶段访问已销毁设备会导致 0xC0000005。为避免掩盖用例结果,这里跳过全局析构。
    std::quick_exit(g_fail == 0 ? 0 : 1);
}

/// test.pbr.skin.cpu
/// 用 three.js 示例模型 Soldier.glb 验证纯 CPU 端骨骼蒙皮:
///   1. 通过 FEFormatGLTFReader 加载 .glb(含 skins / JOINTS_0 / WEIGHTS_0)
///   2. 把加载器分配的 pbrSkin 材质切回普通 pbr 管线(不做 GPU 蒙皮),
///      并对 skin 调用 setCpuSkin(true)
///   3. 骨骼动画系统(FESkinSys)每帧调用 FESkin::updateMatrices() + FESkin::skinMesh(),
///      在 CPU 上完成 LBS(linear blend skinning)并写回 mesh 顶点缓冲区,
///      再通知渲染工厂重建顶点缓冲,结果与 GPU 蒙皮(test.pbr.skin)一致
/// 验证点:
///   a. 角色按动画摆动骨骼,姿态与 test.pbr.skin(GPU 蒙皮)一致
///   b. 切换不同动画(数字键 1~4 / 自动循环)角色动作随之改变
///   c. 蒙皮后的法线正确(受光随姿态变化)
///   d. 自动校验:skinMesh 写回结果与单点 skinPoint 一致(误差 < 1e-4);
///      第 300 帧顶点相对绑定姿态确有位移,并保存一张 cpu_skin.png
/// 用法: test.pbr.skin.cpu.exe [--vulkan|--webgpu] [--gpu]
///      数字键 1~N 切换动画;--gpu 保持 pbrSkin 管线(GPU 蒙皮)用于对照

#include    <iostream>
#include    <vector>
#include    <algorithm>
#include    <cmath>
#include    <cstring>
#include    <functional>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/node/FESkin.hpp"
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
    bool    gpuSkin     =   false;      ///< 调试对照:--gpu 保持 pbrSkin 管线(不做 CPU 蒙皮)
    for (int i = 1; i < argc; ++i)
    {
        const char* arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if (_stricmp(arg,"--gpu") == 0)
            gpuSkin     =   true;
    }

    FEContext&  ctx     =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

    FEApp::CreateInfo info{};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx,info);
    if (!app)
    {
        std::cerr << "create app failed\n";
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene scene =   new FEScene(ctx);
    pScene      =   scene.get();
    if (!scene->setup(app,rendererId))
    {
        std::cerr << "scene setup failed\n";
        scene = nullptr;
        app->destroy();
        return  1;
    }

    /// 调亮默认平行光,便于观察蒙皮后的明暗变化
    {
        Light mainLight;
        if (ctx.device().lights().query("main",mainLight) && mainLight)
        {
            auto dir = mainLight->as<FELightDir>();
            if (dir)
            {
                dir->_color = float3(2.5f,2.5f,2.5f);
                dir->_dir   = float3(0.4f,0.7f,0.8f);
            }
        }
    }

    /// 加载 three.js 示例模型(models/gltf/Soldier.glb)
    const String glbFile =   ctx.resourcePath() + "/data/model/Soldier.glb";
    FEFileFormat fmt(".glb","1.0.0.0","GLTF binary Format!");
    auto reader =   FEFileFormatHelper::queryReader(ctx,fmt);
    if (!reader)
    {
        std::cerr << "query gltf reader failed\n";
        scene = nullptr;
        app->destroy();
        return  1;
    }
    auto objects =   reader->readFiles({glbFile});
    Nodes nodes;
    for (auto var : objects)
    {
        Node node = var->cast<FENode>();
        if (node != nullptr)
            nodes.push_back(node);
    }
    if (nodes.empty())
    {
        std::cerr << "load glb failed: " << glbFile << "\n";
        scene = nullptr;
        app->destroy();
        return  1;
    }
    /// 环境光照(IBL)
    const String hdrFile =   ctx.resourcePath() + "/data/env/Cannon_Exterior.hdr";
    ImagePtr envImage    =   FEImageHelper::load(ctx,hdrFile.c_str());
    TexPtr   envCube;
    if (envImage != nullptr)
    {
        envCube =   FETextureHelper::createCube(ctx,*envImage,256);
        if (envCube != nullptr)
        {
            for (auto& node : nodes)
                applyEnvMap(node,envCube);
        }
    }
    /// glTF 为 Y-up,引擎为 Z-up。蒙皮结果的朝向由关节世界矩阵决定,
    /// 因此坐标轴转换通过 FESkin::setWorldRotation 左乘到蒙皮结果上
    const   mat4r   yUpToZUp    =   mat4r(glm::rotate(mat4r(1.0),real(glm::radians(90.0)),real3(1.0,0.0,0.0)));
    /// CPU 蒙皮的网格:加载器给它们分配了 pbrSkin(顶点着色器做 GPU 蒙皮),
    /// 这里切回普通 pbr 管线,并打开 skin 的 CPU 蒙皮开关,
    /// 由骨骼动画系统在每帧场景更新阶段完成 LBS 与顶点缓冲重建
    std::vector<Node>   cpuSkinNodes;
    {
        std::function<void(Node)>   prepare =   [&](Node n)
        {
            if (n == nullptr)
                return;
            Skin    skin    =   skinOf(n);
            if (skin != nullptr)
            {
                skin->setWorldRotation(yUpToZUp);
                Material    mat =   n->material();
                if (mat != nullptr && !gpuSkin)
                {
                    auto    pbr =   mat->cast<FEMaterialPBR>();
                    if (pbr != nullptr)
                        pbr->setPipelinePrefix("pbr");
                }
                if (n->mesh() != nullptr && !gpuSkin)
                    cpuSkinNodes.push_back(n);
                if (!gpuSkin)
                    skin->setCpuSkin(true);
            }
            for (auto& c : n->children())
                prepare(c);
        };
        for (auto& n : nodes)
            prepare(n);
    }
    if (cpuSkinNodes.empty() && !gpuSkin)
        std::cout << "no skinned mesh found\n";

    /// 绑定姿态(蒙皮前)顶点:用于第 300 帧校验动画确实驱动了顶点
    float3s bindPositions;
    FEMesh* bindMesh    =   nullptr;
    if (!cpuSkinNodes.empty())
    {
        Mesh    mesh    =   cpuSkinNodes.front()->mesh();
        if (mesh != nullptr)
        {
            FEAttribyteBuffer*  posBuf  =   mesh->get(IS_VERTEX_POS);
            if (posBuf != nullptr && posBuf->count() > 0)
            {
                const   float3* bp  =   reinterpret_cast<const float3*>(posBuf->data());
                bindPositions.assign(bp,bp + posBuf->count());
                bindMesh    =   mesh.get();
            }
        }
    }

    scene->dispatchNodesToSystem(nodes);
    scene->addNodesToTree(nodes);
    /// 相机:角色世界高度约 1.83(根节点带 0.01 缩放),正面平视全身
    scene->camera()->setEye(real3(0.0,2.9,1.05));
    scene->camera()->setTarget(real3(0.0,0.0,0.95));
    scene->camera()->setUp(real3(0,0,1));
    /// 默认相机远裁剪面只有 1 米,需要放宽
    scene->camera()->setNearFar(real2(0.05,100.0));
    scene->camera()->updatePerspective();

    /// ---- 动画:加载器按 glTF 动画名拆分成独立 clip,这里按名字登记到播放器 ----
    /// 加载时所有动画默认进了 "default" 播放器,先按动画名重新分组
    std::vector<String> animNames;
    FEAnimationSys*     animSys     =   scene->animationSystem();
    if (animSys != nullptr)
    {
        Playback    defaultPb   =   animSys->getOrCreate("default");
        if (defaultPb != nullptr)
        {
            /// objects() 返回内部列表的引用,登记到新播放器会修改它 => 先拷贝
            auto        anims   =   defaultPb->objects();
            for (auto& anim : anims)
            {
                if (anim == nullptr)
                    continue;
                const String&   name    =   anim->name();
                if (std::find(animNames.begin(),animNames.end(),name) == animNames.end())
                    animNames.push_back(name);
                animSys->addObject(anim->as<FEComponent>(),name);
            }
            defaultPb->stop();
        }
    }
    std::cout << "animations:";
    for (auto& name : animNames)
        std::cout << " " << name;
    std::cout << "\n";

    /// 自动循环播放(参考 three.js skinning_blending 示例的动画切换)
    int     animIndex   =   0;
    auto    playAnim    =   [&](int index)
    {
        if (animSys == nullptr || animNames.empty())
            return;
        animIndex   =   (index % int(animNames.size()) + int(animNames.size())) % int(animNames.size());
        for (auto& var : animSys->playbacks())
        {
            if (var.second != nullptr)
                var.second->stop();
        }
        Playback    pb  =   animSys->getOrCreate(animNames[animIndex]);
        if (pb != nullptr)
        {
            pb->setMode(FEPlayback::PT_Loop);
            pb->play();
        }
        std::cout << "play animation: " << animNames[animIndex] << "\n";
    };
    playAnim(0);

    /// 键盘:数字键切换动画
    app->messageBus().subscribe(MSG_KEYDOWN,[&](const FEMessage& msg)
    {
        const   auto&   keyInfo =   static_cast<const MsgKeyDown&>(msg)._info;
        if (keyInfo._key >= '1' && keyInfo._key <= '9')
            playAnim(int(keyInfo._key - '1'));
    });

    /// 蒙皮由骨骼动画系统(FESkinSys)在每帧场景更新阶段完成,
    /// 这里只负责动画切换、自检与抓图
    int     frameCounter  =   0;
    uint    frameNo       =   0;
    bool    selfChecked   =   false;
    bool    capturePending=   false;
    app->messageBus().subscribe(MSG_RENDER,[&](const FEMessage&)
    {
        ++frameNo;
        /// 每 240 帧(约 4 秒)自动切换到下一个动画,方便观察不同动作
        if (++frameCounter >= 240 && animNames.size() >= 2)
        {
            frameCounter = 0;
            playAnim(animIndex + 1);
        }
        /// 首次自检:手动调一次 skinMesh(幂等,始终基于内部缓存的绑定姿态重算),
        /// 校验批量写回结果与单点 skinPoint 完全一致
        if (!selfChecked && !cpuSkinNodes.empty())
        {
            Node    node    =   cpuSkinNodes.front();
            Skin    skin    =   skinOf(node);
            Mesh    mesh    =   node->mesh();
            if (skin != nullptr && mesh != nullptr)
            {
                FEAttribyteBuffer*  posBuf  =   mesh->get(IS_VERTEX_POS);
                FEAttribyteBuffer*  idxBuf  =   mesh->get(IS_VERTEX_BONE_INDEX);
                FEAttribyteBuffer*  wgtBuf  =   mesh->get(IS_VERTEX_BONE_WEIGHT);
                if (posBuf != nullptr && idxBuf != nullptr && wgtBuf != nullptr
                    &&  bindPositions.size() == posBuf->count())
                {
                    /// 校验基准必须用加载时缓存的绑定姿态:
                    /// 引擎已经先在工厂里蒙皮过,当前顶点缓冲已不是绑定姿态
                    const   uint    count   =   posBuf->count();
                    const   float3* bp      =   bindPositions.data();
                    const   ushort4* joints =   reinterpret_cast<const ushort4*>(idxBuf->data());
                    const   float4*  weights=   reinterpret_cast<const float4*>(wgtBuf->data());
                    skin->updateMatrices();
                    uint    sample      =   0;
                    real3   expected(0.0);
                    bool    hasSample   =   false;
                    for (uint i = 0;i < count;++i)
                    {
                        const   float   wsum    =   weights[i].x + weights[i].y + weights[i].z + weights[i].w;
                        if (wsum < 0.999f)
                            continue;
                        sample      =   i;
                        expected    =   skin->skinPoint(real3(bp[i]),joints[i],weights[i]);
                        hasSample   =   true;
                        break;
                    }
                    if (hasSample && skin->skinMesh(*mesh))
                    {
                        const   float3* sp  =   reinterpret_cast<const float3*>(posBuf->data());
                        const   float3  exp =   float3(float(expected.x),float(expected.y),float(expected.z));
                        const   float   err =   glm::length(sp[sample] - exp);
                        selfChecked =   true;
                        std::cout << "[check] cpu skin write-back error = " << err
                                  << (err < 1e-4f ? " PASS" : " FAIL") << "\n";
                    }
                }
            }
        }
        /// 第 300 帧(已切到第二个动画):校验顶点确实被蒙皮改变,并抓一张图
        if (frameNo == 300)
        {
            if (bindMesh != nullptr)
            {
                FEAttribyteBuffer*  posBuf  =   bindMesh->get(IS_VERTEX_POS);
                if (posBuf != nullptr && posBuf->count() == bindPositions.size())
                {
                    const   float3* cur =   reinterpret_cast<const float3*>(posBuf->data());
                    float           maxDisp =   0.0f;
                    for (size_t i = 0;i < bindPositions.size();++i)
                        maxDisp =   (std::max)(maxDisp,glm::length(cur[i] - bindPositions[i]));
                    std::cout << "[check] max vertex displacement = " << maxDisp
                              << (maxDisp > 1.0f ? " PASS" : " FAIL") << "\n";
                }
            }
            scene->requestCapture();
            capturePending  =   true;
        }
        if (capturePending)
        {
            std::vector<unsigned char>  pixels;
            if (scene->takeCapture(pixels))
            {
                capturePending  =   false;
                const   uint2   size    =   scene->captureSize();
                const   size_t  need    =   size_t(size.x) * size_t(size.y) * 4;
                if (size.x != 0 && size.y != 0 && pixels.size() >= need)
                {
                    FEImage image(ctx,size.x,size.y,FMT_R8G8B8A8_UNORM);
                    auto    plane   =   image.plane(0,0);
                    if (plane.data() != nullptr)
                    {
                        for (uint y = 0;y < size.y;++y)
                            std::memcpy(plane.rowAt(y),pixels.data() + size_t(y) * size.x * 4,size_t(size.x) * 4);
                        const   String  out =   app->path() + (gpuSkin ? "/gpu_skin.png" : "/cpu_skin.png");
                        if (FEImageHelper::save(image,out.c_str()))
                            std::cout << "[check] screenshot saved: " << out << "\n";
                    }
                }
            }
        }
    });

    std::cout << "pbr skin cpu test ready, entering render loop...\n";
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        cpuSkinNodes.clear();
        reader      =   nullptr;
        nodes.clear();
        objects.clear();
        envCube     =   nullptr;
        pScene      =   nullptr;
    });
    app->run();

    animSys =   nullptr;
    scene   =   nullptr;
    app->destroy();
    return  0;
}

/// test.pbr.skin
/// 用 three.js 示例模型 Soldier.glb 验证 glTF 骨骼蒙皮(skin)动画:
///   1. 通过 FEFormatGLTFReader 加载 .glb(二进制 glTF,含 skins / JOINTS_0 / WEIGHTS_0)
///   2. FEAnimationSys 按 glTF 动画名(Idle / Run / TPose / Walk)分别播放
///   3. 渲染工厂每帧用关节世界矩阵重算蒙皮矩阵,上传 storage buffer,
///      由 pbrSkin 顶点着色器完成 LBS(linear blend skinning)
/// 验证点:
///   a. 角色按动画循环摆动骨骼(网格随关节变形,不再是绑定姿态)
///   b. 切换不同动画(数字键 1~4 / 自动循环)角色动作随之改变
///   c. 蒙皮网格法线正确(受光随姿态变化)
/// 用法: test.pbr.skin.exe [--vulkan|--webgpu]
///      数字键 1~N 切换动画

#include    <iostream>
#include    <vector>
#include    <algorithm>
#include    <cmath>
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
    for (int i = 1; i < argc; ++i)
    {
        const char* arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
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
    /// glTF 为 Y-up,引擎为 Z-up。蒙皮网格的最终朝向由 joint 的世界矩阵决定
    /// (着色器里 mesh 节点的世界变换会被约掉),而骨骼的世界矩阵由动画系统直接驱动,
    /// 因此坐标轴转换通过 FESkin::setWorldRotation 左乘到蒙皮结果上
    const   mat4r   yUpToZUp    =   mat4r(glm::rotate(mat4r(1.0),real(glm::radians(90.0)),real3(1.0,0.0,0.0)));
    {
        std::function<void(Node)>   fixRot  =   [&](Node n)
        {
            if (n == nullptr) return;
            Skin    skin    =   skinOf(n);
            if (skin != nullptr)
                skin->setWorldRotation(yUpToZUp);
            for (auto& c : n->children())
                fixRot(c);
        };
        for (auto& n : nodes)
            fixRot(n);
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

    /// 每 240 帧(约 4 秒)自动切换到下一个动画,方便观察不同动作
    int     frameCounter  =   0;
    app->messageBus().subscribe(MSG_UPDATE,[&](const FEMessage&)
    {
        if (++frameCounter < 240 || animNames.size() < 2)
            return;
        frameCounter = 0;
        playAnim(animIndex + 1);
    });

    std::cout << "pbr skin test ready, entering render loop...\n";
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
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

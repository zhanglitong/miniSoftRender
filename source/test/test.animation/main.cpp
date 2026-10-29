/// test.animation
/// 动画系统覆盖式应用测试:
///   1. 节点自身本地变换: 位置(3,1,0)、缩放(2,2,2)、绕 Z 轴 30 度
///   2. 动画仅含位置轨道: 0s -> (-0.4,0,0),2s -> (0.4,0,0)
///   3. 断言采样位置覆盖节点本地位置;未打帧的缩放/旋转保持节点本地值
///   4. 断言动画停用后节点回落到自身本地变换
/// 用法: test.animation.exe [--vulkan|--webgpu] [--pingpong]

#include    <iostream>
#include    <cmath>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan; --pingpong 切换播乒乓模式
    FEUuid  rendererId  =   RS_VULKAN;
    FEPlayback::PlayMode    playMode    =   FEPlayback::PT_PingPong;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId = RS_WEBGPU;
        }
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
        {
            rendererId = RS_VULKAN;
        }
        else if (_stricmp(arg, "--pingpong") == 0)
        {
            playMode    =   FEPlayback::PT_PingPong;
        }
    }

    FEContext&  ctx =   FEContext::instance();

    /// 场景指针在引擎创建后赋值;notify 把窗口消息转发给场景
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
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 生成立方体节点,并设置非默认的本地变换(位置/缩放/旋转)
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setMesh(mesh);

    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
    pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
    pMat->data().update();
    node->setMaterial(pMat);

    node->setLocalTranslation(real3(3.0,1.0,0.0));
    node->setLocalScaling(real3(0.5,0.5,0.5));
    node->setLocalRotation(quatr(glm::radians(real3(0.0,0.0,30.0))));
    node->makeDirty();
    node->update();

    /// 4. 创建只含位置轨道的动画:0s -> (-0.4,0,0),2s -> (0.4,0,0)
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(ctx, NP_TRANSFORM_XYZ);
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0, real3(-0.4,0.0,0.0));
    clip->addKeyFrame(2.0, real3(0.4,0.0,0.0));
    node->addComponent(Component(anim.get()));
    /// 必须在挂载组件之后分发,动画系统才会收集到该节点的动画
    scene->dispatchNodesToSystem({node});

    /// 5. 断言:位置被动画覆盖,缩放/旋转保持节点本地值
    bool    passed      =   true;
    auto    checkReal   =   [&](const char* name,real actual,real expect)
    {
        bool    ok  =   std::abs(actual - expect) <= 1e-3;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name
                  << " actual=" << actual << " expect=" << expect << std::endl;
        passed  =   passed && ok;
    };

    anim->update(1.0);
    node->update();
    real3   gpos;
    real3   gscl;
    quatr   grot;
    FE::decompose<real>(node->globalTransform(),gpos,gscl,grot);
    std::cout << "t=1.0 pos=[" << gpos.x << "," << gpos.y << "," << gpos.z
              << "] scale=[" << gscl.x << "," << gscl.y << "," << gscl.z << "]" << std::endl;
    checkReal("覆盖:位置 X",gpos.x,0.0);
    checkReal("覆盖:位置 Y",gpos.y,0.0);
    checkReal("保留:缩放 X",gscl.x,0.5);
    checkReal("保留:缩放 Y",gscl.y,0.5);
    quatr   qz30    =   quatr(glm::radians(real3(0.0,0.0,30.0)));
    checkReal("保留:绕 Z 旋转",std::abs((real)glm::dot(grot,qz30)),1.0);

    /// 6. 断言:动画停用后节点回落到自身本地变换
    anim->setEnable(false);
    anim->update(1.0);
    node->update();
    FE::decompose<real>(node->globalTransform(),gpos,gscl,grot);
    std::cout << "disabled pos=[" << gpos.x << "," << gpos.y << "," << gpos.z << "]" << std::endl;
    checkReal("回落:位置 X",gpos.x,3.0);
    checkReal("回落:位置 Y",gpos.y,1.0);

    /// 6.5 编辑轴回归:节点有动画时(时间线停在首帧,等于范围下界),
    ///     通过动画系统写属性必须同步写入关键帧,采样不得把编辑结果覆盖回旧关键帧
    {
        /// 补齐默认动画的三条轨道(位置轨道前面已建,这里补缩放/旋转)
        FEAnimationHelper::addDefaultTracks(clip,ctx,NodeProperyBits(NP_SCALE_XYZ|NP_ROTATE_XYZ));
        anim->setEnable(true);
        anim->update(0.0);
        node->update();
        auto    sys =   scene->animationSystem();
        if (sys == nullptr)
        {
            std::cout << "[FAIL] animationSystem 为空" << std::endl;
            passed  =   false;
        }
        else
        {
            /// 移动抓手:写 PROP_TRANSFORM_XYZ
            real3   editPos =   real3(1.5,0.25,-0.75);
            sys->setNodeProperty(Object(node.get()),PROP_TRANSFORM_XYZ,editPos);
            anim->update(0.0);
            node->update();
            real3   lpos    =   node->localTranslation();
            checkReal("编辑:位置 X",lpos.x,editPos.x);
            checkReal("编辑:位置 Y",lpos.y,editPos.y);
            checkReal("编辑:位置 Z",lpos.z,editPos.z);

            /// 旋转抓手:写 PROP_QUAT,默认轨道是欧拉角,必须换算成匹配的值类型才能写帧
            const   quatr   rotQ    =   normalize(quatr(glm::radians(real3(0.0,0.0,45.0))));
            const   quatf   editRot =   quatf((float)rotQ.w,(float)rotQ.x,(float)rotQ.y,(float)rotQ.z);
            const   real3   euler   =   quatToEuler(rotQ);
            std::cout << "quatToEuler(45deg Z)=[" << euler.x << "," << euler.y << "," << euler.z << "]" << std::endl;
            sys->setNodeProperty(Object(node.get()),PROP_QUAT,editRot);
            anim->update(0.0);
            node->update();
            checkReal("编辑:旋转 dot",(real)std::abs(glm::dot(node->localRotation(),editRot)),1.0);

            /// 缩放抓手:写 PROP_SCALE_XYZ
            const   real3   editScl =   real3(1.25,1.25,1.25);
            sys->setNodeProperty(Object(node.get()),PROP_SCALE_XYZ,editScl);
            anim->update(0.0);
            node->update();
            real3   lscl    =   node->localScaling();
            checkReal("编辑:缩放 X",lscl.x,editScl.x);
            checkReal("编辑:缩放 Y",lscl.y,editScl.y);
            checkReal("编辑:缩放 Z",lscl.z,editScl.z);
        }
    }

    /// 7. 恢复动画并交给播放器循环播放,进入渲染循环
    anim->setEnable(true);
    anim->update(0.0);
    node->update();
    if (auto  sys =   scene->animationSystem())
    {
        auto    action  =   sys->getOrCreate();
        action->setMode(playMode);
        action->play();
    }
    std::cout << "playMode=" << (playMode == FEPlayback::PT_PingPong ? "pingpong" : "loop") << std::endl;

    std::cout << "test.animation " << (passed ? "PASSED" : "FAILED") << std::endl;
    if (!passed)
    {
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 8. 进入渲染循环:立方体在 0~1.2 之间循环平移
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

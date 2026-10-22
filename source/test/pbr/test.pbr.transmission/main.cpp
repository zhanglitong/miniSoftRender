/// test.pbr.transmission
/// transmission 架构验证测试(PBR 材质自动自取场景折射纹理,方案 A):
///   1. 创建引擎 + 渲染窗口
///   2. 场景:背景红色立方体(opaque PBR,z=-3),前景白色球体(transmission=1,z=+1)
///      —— 渲染时透过球体应看到折射后的红色立方体
///   3. FEScene 在 swapchain/resize 时创建场景折射纹理并注册到 device cache
///      (FEConstUuid::SceneTexId);PBR 材质渲染时经 refreshSceneTexture 自动
///      检测并重绑,无需场景遍历挂接(旧架构已移除)
/// 验证点(后台线程 5 秒后判定并写 verify_transmission.txt):
///   a. device cache 中能查询到 FEConstUuid::SceneTexId(FEScene 已注册)
///   b. FEMaterialPBR::anyTransmissionMaterial() == true
///      (玻璃材质已自动绑到真场景纹理,且 transmission>0)
///   c. 玻璃材质的 sceneTexture() 与 cache 中纹理为同一对象
/// 不加载 IBL/HDR(绕开 carconcept 环境生成的独立问题),纯因子着色即可。
/// 用法: test.pbr.transmission.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <thread>
#include    <chrono>
#include    <cstdio>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FEConstUuid.h"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 创建 PBR 球体/立方体 mesh(pos/nor/uv)
    Mesh    buildSphere(FEContext& ctx,float radius)
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   radius;
        geo.param().widthSegments   =   48;
        geo.param().heightSegments  =   32;
        return  geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
    }
    Mesh    buildBox(FEContext& ctx)
    {
        FEGeometryBox   geo(ctx);
        return  geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
    }
}

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
    }

    FEContext&  ctx =   FEContext::instance();
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

    /// 3. 相机:透视,正前方观察原点
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0f,0.0f,10.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->setNear(0.1f);
        cam->setFar(100.0f);
        cam->updatePerspective();
    }

    /// 4. 背景:红色立方体(opaque PBR)——折射画面中的"景物"
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(buildBox(ctx));
        node->setLocalScaling(real3(2.0,2.0,2.0));
        node->setLocalTranslation(real3(0.0,0.0,-3.0));
        auto*   mat =   new FEMaterialPBR(ctx);
        mat->data()._value._diffuse   =   float4(1.0f,0.15f,0.15f,1.0f);
        mat->data()._value._metallic  =   0.0f;
        mat->data()._value._roughness =   0.8f;
        mat->data().update();
        node->setMaterial(mat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }

    /// 5. 前景:白色球体(transmission=1,薄壁玻璃)
    FEMaterialPBR*  glassMat    =   nullptr;
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(buildSphere(ctx,1.2f));
        node->setLocalTranslation(real3(0.0,0.0,1.0));
        glassMat    =   new FEMaterialPBR(ctx);
        glassMat->data()._value._diffuse      =   float4(1.0f,1.0f,1.0f,1.0f);
        glassMat->data()._value._metallic     =   0.0f;
        /// 磨砂玻璃(roughness>0):shader 按 roughness*maxLod 采样 sceneTex
        /// mip 链,覆盖对标 1 的模糊折射路径(Vulkan 满链;占位/单 mip 退化为清晰)
        glassMat->data()._value._roughness    =   0.5f;
        glassMat->data().update();
        glassMat->setTransmission(1.0f);
        /// 对标 2(KHR_materials_volume):厚度 2.0(球径 2.4)产生折射偏移,
        /// 吸收色 (1,0.35,0.35) + 吸收距离 1.5 → Beer-Lambert 红玻璃效果
        glassMat->setVolume(2.0f,float3(1.0f,0.35f,0.35f),1.5f);
        glassMat->setAlphaBlend(true);
        node->setMaterial(glassMat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }
    std::cout << "transmission test ready, entering render loop..." << std::endl;

    /// 6. 后台线程:5 秒后判定架构自检点并写结果文件
    std::thread([&ctx,glassMat,rendererId]
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        /// a. FEScene 已把场景折射纹理注册进 device cache
        auto    cached  =   ctx.device().queryCache(FEConstUuid::SceneTexId);
        const   bool    cacheOk =   (cached != nullptr);
        /// b. 玻璃材质已自动挂到真场景纹理(refreshSceneTexture 生效)
        const   bool    anyTrans    =   FEMaterialPBR::anyTransmissionMaterial();
        /// c. 材质持有的纹理正是 cache 中那份(同一对象,非白图占位)
        bool    sameTex =   false;
        if (cacheOk && glassMat != nullptr)
            sameTex =   (glassMat->sceneTexture().get() == cached.get());
        /// d. 对标 1:sceneTex mip 链——Vulkan 满链(>1),WebGPU 暂保持单 mip
        bool    mipOk   =   false;
        uint    mipCnt  =   0;
        if (sameTex)
        {
            mipCnt  =   glassMat->sceneTexture()->mips();
            mipOk   =   (rendererId == RS_VULKAN) ? (mipCnt > 1) : (mipCnt == 1);
        }
        /// e. 对标 2:volume 参数已写入 UBO 值块(thickness=2.0, attDist=1.5)
        bool    volOk   =   false;
        if (glassMat != nullptr)
        {
            const float2    vol =   glassMat->volume();
            volOk   =   (fabsf(vol.x - 2.0f) < 1e-5f) && (fabsf(vol.y - 1.5f) < 1e-5f);
        }
        const   bool    pass    =   cacheOk && anyTrans && sameTex && mipOk && volOk;
        if (auto* fp = fopen("verify_transmission.txt","w"))
        {
            fprintf(fp,"cacheOk=%d anyTransmission=%d sameTex=%d mips=%u mipOk=%d volOk=%d => %s\n",
                    cacheOk ? 1 : 0, anyTrans ? 1 : 0, sameTex ? 1 : 0,
                    mipCnt, mipOk ? 1 : 0, volOk ? 1 : 0,
                    pass ? "PASS" : "FAIL");
            fclose(fp);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        exit(pass ? 0 : 2);
    }).detach();

    /// 7. 渲染循环(5 秒后由后台线程带结果退出)
    app->run();

    scene   =   nullptr;
    app->destroy();
    return  0;
}

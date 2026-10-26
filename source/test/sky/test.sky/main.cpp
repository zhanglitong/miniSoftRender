/// test.sky
/// 程序化天空盒组件测试:
///   FESkyComponent 实时计算天空(移植 three.js webgl_shaders_sky 的 Preetham 大气散射模型),
///   不加载任何图片,天空颜色完全由着色器按大气参数与太阳位置实时求值。
/// 流程:
///   1. 引擎初始化(与 test.texture.* 一致)
///   2. 创建天空盒组件(内部组装:立方体壳 + FEMaterialSky),把节点分发到渲染工厂
///   3. 设置大气参数(turbidity/rayleigh/mie/luminance)与太阳方位
///   4. 可选:开启自动日升日落(每帧经 MSG_UPDATE 推进太阳)
///   5. 进入渲染循环
/// 用法:
///   test.sky.exe [--vulkan|--webgpu] [--autorun] [--headless] [--elevation <deg>]
///                [--sky-panel|--no-sky-panel]
///     默认静态太阳(高度角 65°,与 three.js 参考示例一致)
///     --autorun    : 开启太阳自动推进(日升日落演示)
///     --headless   : 无头离屏渲染,校验太阳->主方向光同步与昼夜亮度,输出 PNG/TXT
///     --elevation  : 静态太阳高度角(角度制,默认 65;负值即夜晚)
///     --no-sky-panel : 启动时隐藏天空参数面板(仍可通过 setPanelVisible(true) 重新打开)
///     --sky-panel    : 启动时显示天空参数面板(默认)
///   太阳会同步为场景主方向光:方向取太阳方向,亮度按仰角平滑衰减,
///   地平线以下完全熄灭,实现"天黑模型也黑"。

#include    <iostream>
#include    <string>
#include    <cstdlib>
#include    <filesystem>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/component/FESkyComponent.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"

/// 公共天空参数调试面板(与 test.pbr.* 共用):仅 Vulkan 后端启用
#include    "../../common/ImGuiSkyPanel.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    int     g_passed    =   0;
    int     g_failed    =   0;

    void    check(bool condition,const char* message)
    {
        if (condition)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
            std::cerr << "[FAILED] " << message << std::endl;
        }
    }
    String  exePath()
    {
        char    szTemp[2048]    =   {};
        GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
        std::filesystem::path   filePath(szTemp);
        return  filePath.parent_path().string();
    }
    /// <summary>
    /// 在指定位置创建一个白色 PBR 球体(无贴图/无自发光),用于观察昼夜光照
    /// </summary>
    FEMaterialPBR*  addDemoSphere(FEContext& ctx,Scene scene,float radius,const real3& pos)
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   radius;
        geo.param().widthSegments   =   48;
        geo.param().heightSegments  =   32;
        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        node->setLocalTranslation(pos);
        auto*   mat     =   new FEMaterialPBR(ctx);
        mat->data()._value._diffuse   =   float4(1.0f,1.0f,1.0f,1.0f);
        mat->data()._value._metallic  =   0.0f;
        mat->data()._value._roughness =   0.5f;
        /// 自发光必须清零,否则夜间模型仍会残留底色
        mat->data()._value._emissive  =   float4(0.0f,0.0f,0.0f,1.0f);
        mat->data().update();
        node->setMaterial(mat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
        return  mat;
    }
    /// <summary>
    /// 中心区域平均亮度(0~255,Rec.709 加权)
    /// </summary>
    float   centerLuminance(const uint8s& pixels,uint width,uint height)
    {
        const uint  x0  =   width  * 2 / 5;
        const uint  x1  =   width  * 3 / 5;
        const uint  y0  =   height * 2 / 5;
        const uint  y1  =   height * 3 / 5;
        double  sum =   0.0;
        uint64  cnt =   0;
        for (uint y = y0; y < y1; ++y)
        {
            for (uint x = x0; x < x1; ++x)
            {
                const uint8*    p   =   &pixels[(y * width + x) * 4];
                sum +=  0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2];
                ++cnt;
            }
        }
        return  (cnt == 0) ? 0.0f : float(sum / double(cnt));
    }
    /// <summary>
    /// 无头离屏:同一场景下分别以白天/夜晚太阳高度角渲染,
    /// 校验主方向光与太阳同步、且模型亮度随昼夜变化(白天亮/夜晚黑)
    /// </summary>
    int     runHeadless(FEContext& ctx,const FEUuid& rendererId,float dayElevation)
    {
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
        if (scene == nullptr)
        {
            std::cerr << "renderer->scene() == nullptr" << std::endl;
            renderer    =   nullptr;
            return  1;
        }
        scene->setClearColor(float4(0,0,0,1));
        scene->showGrid(false);

        /// 相机放在 -Y 侧看向原点(up=+Z);headless 用方位 180° 让太阳位于相机一侧,
        /// 保证正对相机的半球被照亮,像素判定稳定
        {
            Camera  cam =   scene->camera();
            cam->setEye(real3(0.0f,-5.0f,0.0f));
            cam->setTarget(real3(0.0f,0.0f,0.0f));
            cam->setUp(real3(0.0f,0.0f,1.0f));
            cam->setNear(0.05f);
            cam->setFar(256.0f);
            cam->updatePerspective();
        }

        /// 天空盒由引擎默认创建并加入渲染系统(scene->sky()),此处只调整参数
        auto*   pSky    =   scene->sky();
        if  (pSky == nullptr)
        {
            std::cerr << "scene default sky missing" << std::endl;
            scene   =   nullptr;
            renderer=   nullptr;
            return  1;
        }
        pSky->setTurbidity(10.0f);
        pSky->setRayleigh(3.0f);
        pSky->setMieCoefficient(0.005f);
        pSky->setMieDirectionalG(0.7f);
        pSky->setLuminance(0.05f);
        /// 关云:避免云影影响中心区域亮度判定
        pSky->setCloudCoverage(0.0f);
        pSky->setSunElevationAzimuth(dayElevation,180.0f);

        /// 半径 1 的球体位于原点:投影半径约 24% 画面宽度,中心 20% 区域完全落在球体上
        addDemoSphere(ctx,scene,1.0f,real3(0.0f,0.0f,0.0f));

        auto    mainLight   =   [&]() -> FELightDir*
        {
            auto*   light   =   ctx.device().lights().isExist("main");
            if  (light == nullptr || *light == nullptr)
                return  nullptr;
            return  (*light)->as<FELightDir>();
        };
        auto    sunDirection    =   [&]()
        {
            return  normalize(float3(pSky->sky()._sunDirection.x,
                                     pSky->sky()._sunDirection.y,
                                     pSky->sky()._sunDirection.z));
        };

        /// ---- 白天 ----
        check(renderer->render(4),"render(day) failed");
        const String    dayPng  =   path + "/verify_sky_day.png";
        std::filesystem::remove(dayPng);
        check(renderer->save(dayPng.c_str()),"save verify_sky_day.png failed");
        const float dayLum  =   centerLuminance(renderer->pixels(),256,256);
        FELightDir* dayLight    =   mainLight();
        check(dayLight != nullptr,"main directional light missing");
        float   dayDirDot   =   0.0f;
        float3  dayColor    =   float3(0.0f,0.0f,0.0f);
        if  (dayLight != nullptr)
        {
            dayDirDot   =   dot(normalize(dayLight->_dir),sunDirection());
            dayColor    =   dayLight->_color;
        }
        std::cout << "  day  : elev=" << dayElevation << " lum=" << dayLum
                  << " dirDot=" << dayDirDot
                  << " light=(" << dayColor.x << "," << dayColor.y << "," << dayColor.z << ")"
                  << std::endl;

        /// ---- 夜晚 ----
        const float nightElev   =   -10.0f;
        pSky->setSunElevationAzimuth(nightElev,180.0f);
        check(renderer->render(4),"render(night) failed");
        const String    nightPng    =   path + "/verify_sky_night.png";
        std::filesystem::remove(nightPng);
        check(renderer->save(nightPng.c_str()),"save verify_sky_night.png failed");
        const float nightLum    =   centerLuminance(renderer->pixels(),256,256);
        FELightDir* nightLight  =   mainLight();
        float   nightDirDot =   0.0f;
        float3  nightColor  =   float3(0.0f,0.0f,0.0f);
        if  (nightLight != nullptr)
        {
            nightDirDot =   dot(normalize(nightLight->_dir),sunDirection());
            nightColor  =   nightLight->_color;
        }
        std::cout << "  night: elev=" << nightElev << " lum=" << nightLum
                  << " dirDot=" << nightDirDot
                  << " light=(" << nightColor.x << "," << nightColor.y << "," << nightColor.z << ")"
                  << std::endl;

        /// ---- 校验 ----
        check(dayDirDot > 0.999f,"day light direction not synced with sun");
        check(nightDirDot > 0.999f,"night light direction not synced with sun");
        check(dayColor.r > 0.95f,"day light color not at full intensity");
        check(nightColor.r < 0.01f,"night light is not extinguished");
        check(dayLum > 40.0f,"model not bright during day");
        check(nightLum < 12.0f,"model not black at night");
        check(dayLum > nightLum * 3.0f + 20.0f,"day/night contrast too low");

        const String    txtFile =   path + "/verify_sky.txt";
        {
            if (auto* fp = fopen(txtFile.c_str(),"w"))
            {
                fprintf(fp,"renderer=%s\n",(rendererId == RS_VULKAN) ? "vulkan" : "webgpu");
                fprintf(fp,"day  : elev=%.2f lum=%.2f dirDot=%.5f light=(%.4f,%.4f,%.4f)\n",
                        dayElevation,dayLum,dayDirDot,dayColor.x,dayColor.y,dayColor.z);
                fprintf(fp,"night: elev=%.2f lum=%.2f dirDot=%.5f light=(%.4f,%.4f,%.4f)\n",
                        nightElev,nightLum,nightDirDot,nightColor.x,nightColor.y,nightColor.z);
                fprintf(fp,"png_day=%s\n",dayPng.c_str());
                fprintf(fp,"png_night=%s\n",nightPng.c_str());
                fprintf(fp,"passed=%d failed=%d => %s\n",
                        g_passed,g_failed,(g_failed == 0) ? "PASS" : "FAIL");
                fclose(fp);
            }
        }

        /// 天空由场景默认持有;FEScene::destroy 会先解绑组件(断开节点与组件的强引用环)
        /// 再销毁设备,资源释放顺序安全
        scene   =   nullptr;
        renderer=   nullptr;

        std::cout << "=== " << g_passed << " passed, " << g_failed << " failed ===" << std::endl;
        return  (g_failed == 0) ? 0 : 2;
    }
}

int     main(int argc,char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    /// 默认静态太阳:参考示例是固定的 65° 高度角,自动推进会让太阳随时间变化,
    /// 画面整幅变暗偏蓝,无法与参考对照
    bool    autoSun     =   false;
    bool    headless    =   false;
    float   elevation   =   65.0f;
    /// 天空参数面板启动显隐(命令行控制;运行时仍可用 setPanelVisible 切换)
    bool    skyPanel    =   true;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if (_stricmp(arg, "--autorun") == 0 || _stricmp(arg, "-autorun") == 0)
            autoSun =   true;
        else if (_stricmp(arg, "--headless") == 0 || _stricmp(arg, "-headless") == 0)
            headless    =   true;
        else if (_stricmp(arg, "--elevation") == 0 || _stricmp(arg, "-elevation") == 0)
        {
            if (i + 1 < argc)
                elevation   =   float(std::atof(argv[++i]));
        }
        else if (_stricmp(arg, "--no-sky-panel") == 0 || _stricmp(arg, "-no-sky-panel") == 0)
            skyPanel    =   false;
        else if (_stricmp(arg, "--sky-panel") == 0 || _stricmp(arg, "-sky-panel") == 0)
            skyPanel    =   true;
    }

    FEContext&  ctx =   FEContext::instance();

    /// 无头离屏:校验太阳同步与昼夜亮度(白天/夜晚各渲一帧并保存 PNG)
    if (headless)
        return  runHeadless(ctx, rendererId, elevation);

    FEScene*    pScene  =   nullptr;
    /// 天空组件:必须持有引用,否则节点上的组件会被提前释放
    Component   sky;
    /// 公共天空参数面板:仅在 Vulkan 后端挂接(--webgpu 时无 ImGui 渲染后端)
    FEImGuiSkyPanel     panel;
    bool                wantImGui   =   (rendererId == RS_VULKAN);
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        const   auto    msgId   =   msg.msgId();
        /// 面板先把鼠标消息喂给 ImGui,并在其捕获鼠标时阻止场景响应
        const   bool    blockScene  =   panel.onMessage(msg);
        /// 先交给场景处理(MSG_UPDATE 会在其中刷新 deltaTime 并驱动各系统)
        if (!blockScene && pScene)
            pScene->onMessage(msg);
        /// 再推进天空:自动日升日落(写入 SkyUBO,渲染帧即刻生效)
        if (sky != nullptr && msgId == MSG_UPDATE)
            sky->update(ctx.deltaTime());
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

    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前释放 ImGui 后端
    /// (后端持有 VKDevice 引用,device 销毁后再 destroy 会访问悬空句柄)
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        panel.detach();
    });

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 隐藏引擎默认的编辑网格(天空盒渲染在远平面深度上,网格会透出来)
    scene->showGrid(false);

    /// 引擎默认 Z 朝上:把示例相机调整为水平朝前(沿 +Y、up=+Z),与天空默认 up(=+Z)一致,
    /// 使天顶位于画面上方、地平线位于画面中央。
    /// (场景默认相机为 eye=(0,0,2.5)/up=+Y 沿 -Z 观察,在 Z-up 世界中属于俯视天底方向)
    scene->camera()->setEye(real3(0.0f, -2.5f, 0.0f));
    scene->camera()->setTarget(real3(0.0f, 0.0f, 0.0f));
    scene->camera()->setUp(real3(0.0f, 0.0f, 1.0f));
    scene->camera()->update();

    /// ---- 天空盒由引擎默认创建并加入渲染系统(setup 时自动启动)----
    /// 这里只取组件引用用于调整参数与自动日升日落;SkyUBO 在 setup 完成后可用
    sky =   scene->sky();
    auto*   pSky    =   scene->sky();
    if  (pSky == nullptr)
    {
        std::cerr << "scene default sky missing" << std::endl;
        scene   =   nullptr; app->destroy(); return  1;
    }

    /// 大气参数(与 three.js webgpu_sky 示例面板值一致)
    pSky->setTurbidity(10.0f);
    pSky->setRayleigh(3.0f);
    pSky->setMieCoefficient(0.005f);
    pSky->setMieDirectionalG(0.7f);
    /// 曝光:three.js 示例 GUI 默认 exposure=0.05(覆盖了 init 里的 0.5),必须一致否则严重过曝
    pSky->setLuminance(0.05f);
    /// 云参数(移植 three.js SkyMesh;默认取 webgpu_sky 示例面板值)
    /// 云量 0 即关闭云;云速驱动云随时间流动与翻涌
    pSky->setCloudCoverage(0.4f);
    pSky->setCloudDensity(0.4f);
    pSky->setCloudElevation(0.5f);
    pSky->setCloudSpeed(0.00002f);
    /// 太阳:高度角决定昼夜(>0 白天,<0 黄昏/夜;天顶为 +Z)。
    /// 静态演示默认取 three.js webgpu_sky 示例的 65° 高角、方位 0,与该示例构图一致;
    /// --elevation 可覆盖高度角(负值即夜晚,模型随主方向光一起变黑);
    /// 自动模式(setAutoSun)会从低角开始扫过完整日升日落。
    pSky->setSunElevationAzimuth(elevation, 0.0f);
    if (autoSun)
        pSky->setAutoSun(true, 5.0f, 2.0f);   // 5 度起,2 度/秒推进

    /// 校验:默认天空材质成功取到 sky 管线
    {
        auto*   skyNode =   (sky != nullptr) ? sky->owner()->as<FENode>() : nullptr;
        Material    mat =   (skyNode != nullptr) ? skyNode->material() : nullptr;
        if (mat == nullptr || mat->pipeline(PRI_TRIANGLES) == nullptr)
        {
            std::cerr << "sky pipeline (sky/TRIANGLES) not found" << std::endl;
            scene   =   nullptr; app->destroy(); return  1;
        }
    }

    /// 演示模型:白色 PBR 球体,用于观察太阳同步后的昼夜光照
    /// (相机在 -Y 看向原点,球体偏右前方,白天受光、夜晚随主方向光变黑)
    addDemoSphere(ctx, scene, 0.35f, real3(0.7f, 0.0f, -0.2f));

    /// ---- 天空参数调试面板(公共实现)----
    /// 命令行只控制启动显隐;运行时仍可通过 FESkyComponent::setPanelVisible()
    /// 或公共组件挂接接口切换。attach 内部完成 ImGui 上下文/后端/字体/Overlay 工厂。
    pSky->setPanelVisible(skyPanel);
    panel._autoSun  =   autoSun;
    panel._autoSpeed=   2.0f;
    if (wantImGui)
        panel.attach(ctx,scene.get(),true);

    std::cout << "sky component ready (programmatic, no textures), entering render loop..."
              << (autoSun ? " [auto-sun]" : " [static-sun]")
              << (skyPanel ? " [sky-panel]" : " [no-sky-panel]") << std::endl;

    app->run();

    /// <summary>
    /// 正常关闭时后端已在 MSG_CLOSE 中先行释放(抢在场景销毁 device 之前);
    /// 此处兜底:若循环因其它原因退出而未走 MSG_CLOSE,后端仍需在
    /// 场景析构(销毁 device)之前释放
    /// </summary>
    panel.detach();
    /// 天空由场景默认持有;FEScene::destroy 会先解绑组件再销毁设备
    scene   =   nullptr;
    sky     =   nullptr;
    app->destroy();
    return  0;
}

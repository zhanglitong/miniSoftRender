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
///   test.sky.exe [--vulkan|--webgpu] [--autorun]
///     默认静态太阳(高度角 65°,与 three.js 参考示例一致)
///     --autorun : 开启太阳自动推进(日升日落演示)

#include    <iostream>
#include    <string>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/component/FESkyComponent.hpp"

/// 引擎内调试 GUI(Dear ImGui):仅 Vulkan 后端启用
#include    "ImGuiDebugUI.hpp"
#include    <imgui.h>

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    /// 默认静态太阳:参考示例是固定的 65° 高度角,自动推进会让太阳随时间变化,
    /// 画面整幅变暗偏蓝,无法与参考对照
    bool    autoSun     =   false;
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
    }

    FEContext&  ctx =   FEContext::instance();

    FEScene*    pScene  =   nullptr;
    /// 天空组件:必须持有引用,否则节点上的组件会被提前释放
    Component   sky;
    /// 调试 GUI:仅 Vulkan 后端启用(--webgpu 时 createImGuiBackend 返回 nullptr)
    bool        wantImGui   =   (rendererId == RS_VULKAN);
    bool        imguiReady  =   false;
    /// ImGui 渲染后端:内部持有 VKDevice 引用,必须在 device 销毁前释放
    FEImGuiBackend*     imguiBackend    =   nullptr;
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        const   auto    msgId   =   msg.msgId();
        bool            blockScene  =   false;
        if (wantImGui && imguiReady)
        {
            /// 把鼠标事件喂给 ImGui(NewFrame 时消费)
            ImGuiIO&    io  =   ImGui::GetIO();
            switch (msgId)
            {
            case MSG_MOUSE_MOVE:
                {
                    const auto& m = static_cast<const MsgMouseMove&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                }
                break;
            case MSG_LBUTTON_DOWN:
                {
                    const auto& m = static_cast<const MsgLButtonDown&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[0] =   true;
                }
                break;
            case MSG_LBUTTON_UP:
                {
                    const auto& m = static_cast<const MsgLButtonUp&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[0] =   false;
                }
                break;
            case MSG_RBUTTON_DOWN:
                {
                    const auto& m = static_cast<const MsgRButtonDown&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[1] =   true;
                }
                break;
            case MSG_RBUTTON_UP:
                {
                    const auto& m = static_cast<const MsgRButtonUp&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[1] =   false;
                }
                break;
            case MSG_MBUTTON_DOWN:
                {
                    const auto& m = static_cast<const MsgMButtonDown&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[2] =   true;
                }
                break;
            case MSG_MBUTTON_UP:
                {
                    const auto& m = static_cast<const MsgMButtonUp&>(msg)._info;
                    io.MousePos =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                    io.MouseDown[2] =   false;
                }
                break;
            case MSG_MOUSE_WHEEL:
                {
                    const auto& w = static_cast<const MsgMouseWheel&>(msg)._info;
                    io.MousePos  =  ImVec2(float(w._mouse.x),float(w._mouse.y));
                    io.MouseWheel=  float(w._zDelta) / 120.0f;
                }
                break;
            default:
                break;
            }
            /// GUI 捕获鼠标时不把鼠标消息交给场景,避免拖动滑条同时旋转相机
            if (io.WantCaptureMouse)
            {
                switch (msgId)
                {
                case MSG_MOUSE_MOVE:
                case MSG_LBUTTON_DOWN:   case MSG_LBUTTON_UP:
                case MSG_RBUTTON_DOWN:   case MSG_RBUTTON_UP:
                case MSG_MBUTTON_DOWN:   case MSG_MBUTTON_UP:
                case MSG_MOUSE_WHEEL:
                    blockScene  =   true;
                    break;
                default:
                    break;
                }
            }
        }
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
        if (imguiBackend != nullptr)
        {
            imguiBackend->destroy();
            delete  imguiBackend;
            imguiBackend    =   nullptr;
            imguiReady  =   false;
        }
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

    /// ---- 创建天空盒组件(场景 setup 后缓存的 SkyUBO 才可用)----
    sky =   new FESkyComponent(ctx);
    auto*   pSky    =   (FESkyComponent*)sky.get();

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
    /// 静态演示取 three.js webgpu_sky 示例的 65° 高角、方位 0,与该示例构图一致;
    /// 自动模式(setAutoSun)会从低角开始扫过完整日升日落。
    pSky->setSunElevationAzimuth(65.0f, 0.0f);
    if (autoSun)
        pSky->setAutoSun(true, 5.0f, 2.0f);   // 5 度起,2 度/秒推进

    Node    node    =   pSky->createNode();
    /// 校验:天空材质成功取到 sky 管线
    auto*   pMat    =   (FEMaterial*)node->material();
    if (pMat == nullptr || pMat->pipeline(PRI_TRIANGLES) == nullptr)
    {
        std::cerr << "sky pipeline (sky/TRIANGLES) not found" << std::endl;
        scene   =   nullptr; app->destroy(); return  1;
    }
    node->addComponent(sky);
    scene->dispatchNodesToSystem({node});

    /// ---- 引擎内调试 GUI(Dear ImGui)----
    /// 只做三件事:创建 ImGui 上下文、让渲染后端(VKImGuiBackend)就绪、
    /// 把 FEImGuiFactory 加到工厂管理器(其绘制属性为 USAGE_Overlay → 最后绘制)。
    FEImGuiSkyPanel    panel;
    panel._autoSun  =   autoSun;
    panel._autoSpeed=   2.0f;
    if (wantImGui)
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO&    io  =   ImGui::GetIO();
        /// 不落盘 imgui.ini,避免测试产物污染工作目录
        io.IniFilename  =   nullptr;

        imguiBackend    =   scene->device()->createImGuiBackend();
        if (imguiBackend == nullptr || !imguiBackend->init(scene->swapchainColorFormat()))
        {
            std::cerr << "create ImGui backend failed (debug GUI disabled)" << std::endl;
            if (imguiBackend != nullptr)
            {
                imguiBackend->destroy();
                delete  imguiBackend;
                imguiBackend    =   nullptr;
            }
            ImGui::DestroyContext();
            wantImGui   =   false;
        }
        else
        {
            /// 上传字体图集(RGBA8);TexID 仅作占位,后端始终绑定该纹理
            /// 先加载宋体(simsun.ttc)作为默认字体,使面板支持中文显示;
            /// 若加载失败(文件不存在),io.FontDefault 保持 ImGui 内置字体,中文会显示为方块
            ImFont* cnFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simsun.ttc", 16.0f, nullptr,
                                                          io.Fonts->GetGlyphRangesChineseFull());
            if (cnFont)
                io.FontDefault = cnFont;

            unsigned char*  pixels  =   nullptr;
            int             texW    =   0;
            int             texH    =   0;
            io.Fonts->GetTexDataAsRGBA32(&pixels,&texW,&texH);
            FEImGuiFontData font;
            font._pixels    =   pixels;
            font._width     =   uint32(texW);
            font._height    =   uint32(texH);
            imguiBackend->uploadFont(font);
            io.Fonts->TexID =   (void*)1;

            auto*   pGuiFactory =   new FEImGuiFactory(ctx,imguiBackend);
            pGuiFactory->_buildUI   =   [&panel,pSky]() {   panel.draw(pSky);   };
            scene->factoryMgr().addObject(pGuiFactory);
            imguiReady  =   true;
        }
    }

    std::cout << "sky component ready (programmatic, no textures), entering render loop..."
              << (autoSun ? " [auto-sun]" : " [static-sun]") << std::endl;

    app->run();

    /// <summary>
    /// 正常关闭时后端已在 MSG_CLOSE 中先行释放(抢在场景销毁 device 之前);
    /// 此处兜底:若循环因其它原因退出而未走 MSG_CLOSE,后端仍需在
    /// 场景析构(销毁 device)之前释放
    /// </summary>
    if (imguiBackend != nullptr)
    {
        imguiBackend->destroy();
        delete  imguiBackend;
        imguiBackend    =   nullptr;
    }
    scene   =   nullptr;
    sky     =   nullptr;
    if (wantImGui)
        ImGui::DestroyContext();
    app->destroy();
    return  0;
}

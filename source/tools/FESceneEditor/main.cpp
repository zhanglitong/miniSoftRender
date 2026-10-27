/// <summary>
/// FESceneEditor
/// miniSoftRender 场景编辑器(工具):Vulkan 窗口 + ImGui Ribbon 功能区。
/// 流程:
///   1. 创建引擎窗口与场景(默认含天空盒、网格、相机)
///   2. 挂接 ImGui 主界面(上下文/渲染后端/字体/Overlay 工厂)
///   3. 进入渲染循环;Ribbon 标签:文件(退出)/开始(网格、重置视图)/视图(网格)/帮助(关于)/界面(模型树、UI 预览)
/// 用法:
///   FESceneEditor.exe
/// </summary>

#include    <iostream>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    "FEEditorUi.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main()
{
    FEContext&  ctx         =   FEContext::instance();
    const   FEUuid  rendererId  =   RS_VULKAN;

    FEScene*    pScene  =   nullptr;
    FEEditorUi  ui;

    FEApp::CreateInfo   info    =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        /// <summary>
        /// 界面先把鼠标消息喂给 ImGui,并在其捕获鼠标时阻止场景响应
        /// </summary>
        const   bool    blockScene  =   ui.onMessage(msg);
        if (!blockScene && pScene != nullptr)
            pScene->onMessage(msg);
    };

    App     app     =   FEAppHelper::create(ctx,info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    /// <summary>
    /// 窗口关闭时抢在场景销毁 device 之前释放 ImGui 后端
    /// (后端持有 VKDevice 引用,device 销毁后再 destroy 会访问悬空句柄)
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        ui.detach();
    });

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app,rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// <summary>
    /// 引擎默认 Z 朝上:相机水平朝前(沿 +Y、up=+Z),与默认天空的 up 一致
    /// </summary>
    scene->camera()->setEye(real3(0.0f,-2.5f,0.0f));
    scene->camera()->setTarget(real3(0.0f,0.0f,0.0f));
    scene->camera()->setUp(real3(0.0f,0.0f,1.0f));
    scene->camera()->update();
    scene->showGrid(true);

    /// <summary>
    /// 功能区"退出":直接向引擎窗口投递关闭消息
    /// (不能用 GetActiveWindow,编辑器带控制台窗口时可能取到控制台)
    /// </summary>
    const   HWND    hwnd    =   (HWND)app->cInfo()._window;
    ui._onQuit  =   [hwnd]()
    {
        PostMessageA(hwnd,WM_CLOSE,0,0);
    };

    ui.attach(ctx,scene.get());

    std::cout << "FESceneEditor ready, entering render loop..." << std::endl;
    app->run();

    /// <summary>
    /// 正常关闭时后端已在 MSG_CLOSE 中先行释放;此处兜底
    /// </summary>
    ui.detach();
    scene   =   nullptr;
    pScene  =   nullptr;
    app->destroy();
    return  0;
}

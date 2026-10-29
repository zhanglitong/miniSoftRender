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

#include    <cstring>
#include    <iostream>
#include    <string>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    "FEEditorUi.hpp"
#include    "FEShortcutConfig.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// <summary>
    /// 渲染窗口句柄与"清理完成"事件,供控制台事件处理线程使用
    /// </summary>
    HWND    g_renderWnd =   nullptr;
    HANDLE  g_cleaned   =   nullptr;

    /// <summary>
    /// 关闭控制台窗口 / Ctrl+C / Ctrl+Break 时先把关闭消息转发给渲染窗口,
    /// 走正常流程在 device 销毁前归还 GPU 资源。控制台事件处理函数返回后
    /// 系统会直接结束进程,故这里要等清理完成,否则 Vulkan 对象来不及销毁
    /// (驱动与验证层会报 "Object has not been destroyed")
    /// </summary>
    BOOL    WINAPI    onConsoleCtrl(DWORD type)
    {
        if (type != CTRL_CLOSE_EVENT && type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT)
            return  FALSE;
        if (g_renderWnd != nullptr)
            PostMessageA(g_renderWnd,WM_CLOSE,0,0);
        if (g_cleaned != nullptr)
            WaitForSingleObject(g_cleaned,4500);
        return  TRUE;
    }
}

int     main(int argc,char* argv[])
{
    /// 命令行参数:--script &lt;文件&gt; 启动脚本自动化,其余参数暂不处理
    std::string     scriptPath;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i],"--script") == 0 && i + 1 < argc)
            scriptPath =   argv[++i];
    }

    FEContext&  ctx         =   FEContext::instance();
    /// <summary>
    /// 配置是所有功能的启动基础,在创建窗口/场景之前第一时间加载(exe 同目录,
    /// exe 路径由平台层 FE::exePath() 提供)
    /// </summary>
    ctx.setting().load(FEShortcutConfig::exeXmlPath().c_str());

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

    /// <summary>
    /// 控制台窗口被关闭时也走正常关闭流程(见 onConsoleCtrl)
    /// </summary>
    g_renderWnd =   hwnd;
    g_cleaned   =   CreateEventA(nullptr,TRUE,FALSE,nullptr);
    SetConsoleCtrlHandler(onConsoleCtrl,TRUE);

    ui._onQuit  =   [hwnd]()
    {
        PostMessageA(hwnd,WM_CLOSE,0,0);
    };

    ui.attach(ctx,scene.get());
    /// 脚本自动化:挂接成功后加载脚本(路径相对当前工作目录)
    if (!scriptPath.empty() && !ui.loadScript(scriptPath))
        std::cerr << "load script failed: " << scriptPath << std::endl;

    std::cout << "FESceneEditor ready, entering render loop..." << std::endl;
    app->run();

    /// <summary>
    /// 正常关闭时场景已在 MSG_CLOSE 中销毁(ImGui 后端先于 device 释放);
    /// 若循环因其它原因退出而没走 MSG_CLOSE,这里补一次 destroy(幂等),
    /// 保证 GPU 资源在 app->destroy() 与进程退出前全部归还
    /// </summary>
    /// 脚本结果要在 detach 之前取:退出码 1 表示脚本断言失败,供 CI 判定
    const   bool    scriptFailed    =   ui.scriptFailed();
    ui.detach();
    if (scene != nullptr)
        scene->destroy();
    scene   =   nullptr;
    pScene  =   nullptr;
    app->destroy();

    /// <summary>
    /// 通知可能仍在等待的控制台事件处理线程:清理已完成
    /// </summary>
    if (g_cleaned != nullptr)
    {
        SetEvent(g_cleaned);
        CloseHandle(g_cleaned);
        g_cleaned   =   nullptr;
    }
    return  scriptFailed ? 1 : 0;
}

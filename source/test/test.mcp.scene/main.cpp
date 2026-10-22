/// test.mcp.scene
/// 带真实渲染场景的 MCP Server 测试进程:
///   1. 创建引擎 + 渲染窗口 + 场景(完整 setup,注册动画系统)
///   2. 启动 FEMcpServer,外部 AI Agent 通过 JSON-RPC 调 tools 创建场景内容
///   3. 进入渲染循环:动画系统每帧 update,Agent 创建的动画真实播放
/// 与 test.mcp 的区别:test.mcp 是数据场景(无渲染,动画只挂组件不播放);
///   本进程是真实场景,动画系统存在,anim_translate 返回 playing 并真实驱动节点
/// 用法: test.mcp.scene.exe [port] [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryGrid.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../core/mcp/FEMcpServer.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    uint16  port        =   28100;
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg =   argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
        {
            rendererId  =   RS_VULKAN;
        }
        else
        {
            port    =   (uint16)atoi(arg);
        }
    }

    FEContext&  ctx =   FEContext::instance();
    FEScene*    pScene  =   nullptr;
    FEMcpServer*    pMcp    =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (!pScene) return;
        /// 每帧在主线程处理 MCP 排队请求(工具 handler 在此线程运行,
        /// 保证引擎/GPU 操作与渲染循环同线程,避免 Vulkan 线程亲和性崩溃)
        if (msg.msgId() == MSG_UPDATE && pMcp)
        {
            pMcp->update();
        }
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

    /// 2. 创建场景并完整 setup(注册动画系统)
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 放一个网格地面作为视觉参照(不影响 Agent 创建的内容)
    {
        FEGeometryGrid  geo(ctx);
        geo.param()._size   =   20;
        geo.param()._divs   =   20;
        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_COLOR0,    FMT_R8G8B8A8_UNORM} });
        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        node->setMaterial(new FEMaterialPBR(ctx));
        scene->addNodesToTree({node});
        scene->dispatchNodesToSystem({node});
    }

    /// 4. 启动 MCP Server
    FEMcpServer mcp(ctx, *scene);
    pMcp        =   &mcp;
    mcp.addEngineTools();
    if (!mcp.start(port, "127.0.0.1"))
    {
        std::cerr << "start MCP server on port " << port << " failed (port occupied?)" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    std::cout   << "miniSoftRender MCP server (real scene)\n"
                << "  endpoint : http://127.0.0.1:" << port << "/mcp (JSON-RPC 2.0)\n"
                << "  tools    : " << mcp.toolCount() << " registered\n"
                << "  scene    : setup done, animation system active\n"
                << "AI Agent 可通过 MCP tools 创建节点/几何/动画,动画将真实播放" << std::endl;

    /// 窗口关闭时先停 MCP
    app->messageBus().subscribe(MSG_CLOSE, [&](const FEMessage&)
    {
        mcp.stop();
    });

    /// 5. 进入渲染循环(动画系统在此每帧 update)
    app->run();

    mcp.stop();
    scene   =   nullptr;
    app->destroy();
    return  0;
}

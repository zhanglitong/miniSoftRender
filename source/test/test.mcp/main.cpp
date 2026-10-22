/// test.mcp
/// 引擎 MCP Server 控制台测试:
///   1. 创建 FEContext + 数据场景(无渲染窗口,不进渲染循环)
///   2. FEMcpServer 注册引擎内置工具(scene/node/geo/mesh/anim)
///   3. 监听 127.0.0.1:28100,外部客户端(如 Claude/Cursor/任意 MCP client)以
///      JSON-RPC 2.0 POST /mcp 调用 tools
/// 验证方式(另一终端):
///   curl -X POST http://127.0.0.1:28100/mcp -H "Content-Type: application/json"
///        -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}"
/// 用法: test.mcp.exe [port] [holdSeconds]

#include    <iostream>
#include    <thread>
#include    <chrono>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../core/mcp/FEMcpServer.hpp"

using   namespace   FE;

/// 用法: test.mcp.exe [port] [holdSeconds]
/// holdSeconds > 0:服务保持指定秒后自动退出(无人值守测试,stdin 非 tty 时 cin.get 会立即返回);
/// 否则等待 Enter 退出
int     main(int argc, char** argv)
{
    uint16  port    =   28100;
    uint32  hold    =   0;
    if (argc    >=  2)
        port    =   (uint16)atoi(argv[1]);
    if (argc    >=  3)
        hold    =   (uint32)atoi(argv[2]);

    FEContext&  ctx =   FEContext::instance();

    /// 数据场景:不 setup 渲染,节点树/几何/动画数据面全部可用
    Scene       scene   =   new FEScene(ctx);
    FEMcpServer mcp(ctx, *scene);
    mcp.addEngineTools();

    if (!mcp.start(port, "127.0.0.1"))
    {
        std::cerr << "start MCP server on port " << port << " failed (port occupied?)" << std::endl;
        scene   =   nullptr;
        return  1;
    }

    std::cout   << "miniSoftRender MCP server\n"
                << "  endpoint : http://127.0.0.1:" << port << "/mcp (JSON-RPC 2.0)\n"
                << "  tools    : " << mcp.toolCount() << " registered\n"
                << "  protocol : initialize / ping / tools/list / tools/call\n"
                << "press Enter to quit..." << std::endl;

    /// 数据场景无渲染循环:后台线程持续调用 update() 处理排队的 MCP 请求
    /// (HTTP 线程只入队,工具 handler 在此线程执行;数据场景无 GPU,线程安全)
    std::thread updater([&]
    {
        while (mcp.isRunning())
        {
            mcp.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    if (hold    >   0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(hold));
    }
    else
    {
        std::cin.get();
    }

    mcp.stop();
    if (updater.joinable())
        updater.join();
    scene   =   nullptr;
    return  0;
}

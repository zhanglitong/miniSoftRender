/// FEMcpServer
/// 引擎 MCP(Model Context Protocol)Server:把 FEScene/FENode/FEMesh/FEGeometry/动画
/// 能力暴露为 MCP tools,供 LLM/Agent 等外部客户端以 JSON-RPC 2.0 调用
///
/// 传输层复用 FEHttpServer:单端点 POST /mcp,请求/响应均为 JSON 文本
/// 协议实现:initialize / ping / tools/list / tools/call;notifications/* 收到即忽略(HTTP 202)
/// JSON 使用引擎自带 nlohmann/json(source/inc/fileFormat/gltf/json.hpp)
/// 工具执行串行化(内部 mutex):同一时刻只执行一个工具,引擎对象无需考虑并发访问
///
/// 与脚本绑定无关系:工具 handler 直接调用引擎 C++ API

#pragma     once

#include    <functional>
#include    <memory>
#include    <mutex>
#include    <queue>
#include    <condition_variable>
#include    <vector>
#include    <atomic>

#include    "../../inc/FEDefine.h"
#include    "../../inc/network/FEHttpServer.hpp"
#include    "../../inc/fileFormat/gltf/json.hpp"

namespace   FE
{
    class   FEContext;
    class   FEScene;

    /// <summary>
    /// MCP 工具定义:tools/list 暴露元信息,tools/call 按 arguments 执行 handler
    /// handler 返回 json(结构化结果,序列化为 text content);
    /// 参数错误/执行失败直接 throw std::runtime_error,框架转为 isError=true 响应
    /// </summary>
    struct  FE_API  FEMcpTool
    {
        using   Handler =   std::function<nlohmann::json(const nlohmann::json& arguments)>;

        String          name;           /// 工具名(工具的唯一索引)
        String          description;    /// 给 LLM 看的功能描述
        nlohmann::json  inputSchema;    /// 参数 JSON Schema(类型 object)
        Handler         handler;        /// 执行体
    };

    /// <summary>
    /// 引擎 MCP Server
    /// 用法:构造(传入 context 与数据场景) -> addEngineTools()/addTool() -> start(port)
    /// HTTP 线程只收请求并入队;工具 handler 在主线程通过 update() 执行
    /// (引擎渲染/GPU 操作必须在主线程,否则 Vulkan 会因线程亲和性崩溃)
    /// </summary>
    class   FE_API  FEMcpServer
    {
    public:
        FEMcpServer(FEContext& ctx, FEScene& scene);
        ~FEMcpServer();
    public:
        /// 注册引擎内置工具(scene/node/geo/mesh/anim);重复调用安全(同名跳过)
        void    addEngineTools();
        /// 注册/覆盖自定义工具(须在 start 之前调用)
        void    addTool(FEMcpTool tool);
        /// 已注册工具数量
        size_t  toolCount() const;
    public:
        /// 非阻塞启动:绑定并监听后由内部线程服务;端口被占用返回 false
        bool    start(uint16 port, const String& host = "127.0.0.1");
        /// 停止并回收内部线程(未启动时安全调用)
        void    stop();
        bool    isRunning() const;
        uint16  port() const;
        /// 主线程每帧调用:取出排队的 MCP 请求并执行(handler 在此线程运行)
        void    update();
    private:
        /// 处理一条 JSON-RPC 文本;通知类方法返回空串(HTTP 202)
        String  handleRpc(const String& body);
        /// 按名查找工具,未命中返回 nullptr
        const   FEMcpTool*  findTool(const String& name) const;
        /// 注册 FEScene/FENode/FEGeometry/FEMesh/动画相关的内置工具
        void    registerEngineTools();
    private:
        /// mesh 注册表:geo_create 产生的 mesh 按 id 存放,供 node_set_mesh/mesh_info 引用
        struct  MeshPool;
        /// 排队的 RPC 请求:HTTP 线程填充 body,主线程执行后写 response 并置 done
        struct  PendingRequest
        {
            String                  body;
            String                  response;
            std::atomic<bool>       done{false};
        };
    private:
        FEContext*                  _ctx    =   nullptr;
        FEScene*                    _scene  =   nullptr;
        FEHttpServer                _http;
        std::vector<FEMcpTool>      _tools;
        std::unique_ptr<MeshPool>   _meshes;
        mutable std::mutex          _mutex;     /// 队列与工具执行串行化
        std::queue<std::shared_ptr<PendingRequest>> _pending;
        std::condition_variable     _cv;
        uint16                      _port   =   0;

        FEMcpServer(const FEMcpServer&)              =   delete;
        FEMcpServer& operator = (const FEMcpServer&) =   delete;
    };
}

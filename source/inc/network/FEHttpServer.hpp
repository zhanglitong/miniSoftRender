#pragma     once

#include    <functional>
#include    <map>
#include    <memory>

#include    "FEDefine.h"

namespace   FE
{
    /// HTTP 请求(纯数据封装,不含任何第三方库类型)
    struct  FE_API  FEHttpRequest
    {
        String                      method;     /// 请求方法:GET/POST/PUT/DELETE...
        String                      path;       /// 请求路径,如 /hello
        String                      body;       /// 请求体
        std::map<String, String>    params;     /// URL 查询参数
        std::map<String, String>    headers;    /// 请求头(键统一小写,同名多值取首个)
    };

    /// HTTP 响应(纯数据封装,handler 直接填写)
    struct  FE_API  FEHttpResponse
    {
        uint32      status      =   200;
        String      contentType =   "text/plain";
        String      body;
    };

    /// 请求处理回调:读 request,填 response
    using FEHttpHandler =   std::function<void(const FEHttpRequest&, FEHttpResponse&)>;

    /// <summary>消息处理回调:与路由 handler 相同签名,每种消息注册一个专门处理函数</summary>
    using FEMessageHandler  =   FEHttpHandler;

    /// <summary>流式接收代理:请求体每到达一块被调用一次;返回 false 中止接收(连接随即断开)</summary>
    using FEStreamSink      =   std::function<bool(const char* data, size_t size)>;

    /// <summary>流式数据源:传入 sink 启动接收,阻塞至请求体全部推送完(或中止)才返回</summary>
    using FEStreamSource    =   std::function<void(FEStreamSink)>;

    /// <summary>
    /// 流式消息处理回调:请求体不整体进内存,经 source 逐块(约 4KB)推给 sink,内存占用与单块大小恒定;
    /// request.body 恒为空;source 返回后请求体已接收完毕,可直接填写 response(如打开已落盘文件)
    /// </summary>
    using FEStreamHandler   =   std::function<void(const FEHttpRequest&, FEHttpResponse&, FEStreamSource)>;

    /// 内嵌 HTTP 服务器(pimpl 隔离实现,不对外暴露任何第三方库类型)
    /// 用法:先 route/get/post/onMessage/onStreamMessage 注册 handler,再 start(非阻塞,内部工作线程);
    /// handler 注册须在 start 之前;stop 后可重新 start;析构自动 stop
    class   FE_API  FEHttpServer
    {
    public:
        FEHttpServer();
        virtual ~FEHttpServer();
    public:
        /// 注册路由(须在 start 前调用);method 大小写不敏感
        void    route(const String& method, const String& pattern, FEHttpHandler handler);
        void    get(const String& pattern, FEHttpHandler handler);
        void    post(const String& pattern, FEHttpHandler handler);

        /// <summary>
        /// 注册消息处理函数(须在 start 前调用);消息名大小写敏感,同名消息后注册覆盖
        /// </summary>
        /// <param name="name">消息名;触发入口为任意 HTTP 方法访问 /message?name=&lt;消息名&gt;</param>
        /// <param name="handler">该消息的专门处理函数</param>
        /// <remarks>未注册消息回 404,缺 name 参数回 400</remarks>
        void    onMessage(const String& name, FEMessageHandler handler);

        /// <summary>
        /// 注册流式消息处理函数(须在 start 前调用);消息名大小写敏感,同名后注册覆盖
        /// </summary>
        /// <param name="name">消息名;触发入口为 POST/PUT/PATCH 访问 /stream?name=&lt;消息名&gt;</param>
        /// <param name="handler">该消息的流式处理函数</param>
        /// <remarks>与 onMessage 的 /message 入口互相独立;请求体大小上限 100MB(httplib 默认),超限回 413</remarks>
        void    onStreamMessage(const String& name, FEStreamHandler handler);

        /// 非阻塞启动:绑定并监听后由内部线程服务;端口被占用返回 false
        bool    start(uint16 port, const String& host = "0.0.0.0");
        /// 停止并回收内部线程(未启动时安全调用)
        void    stop();

        bool    isRunning() const;
        uint16  port() const;
    private:
        struct  Impl;
        std::unique_ptr<Impl>   _impl;

        FEHttpServer(const FEHttpServer&)              =   delete;
        FEHttpServer& operator = (const FEHttpServer&) =   delete;
    };
}

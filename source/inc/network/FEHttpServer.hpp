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

    /// 内嵌 HTTP 服务器(pimpl 隔离实现,不对外暴露任何第三方库类型)
    /// 用法:先 route/get/post 注册 handler,再 start(非阻塞,内部工作线程);
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

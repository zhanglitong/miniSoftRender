#include    "../../inc/network/FEHttpServer.hpp"

#include    <httplib.h>

#include    <atomic>
#include    <thread>

namespace   FE
{
    /// <summary>httplib 请求 -> FE 纯数据封装(请求头键统一小写,同名多值取首个)</summary>
    static  FEHttpRequest   toRequest(const httplib::Request& req)
    {
        FEHttpRequest   request;
        request.method  =   req.method;
        request.path    =   req.path;
        request.body    =   req.body;
        for (const auto& [key, value] : req.params)     request.params.emplace(key, value);
        for (const auto& [key, value] : req.headers)
        {
            String  lowerKey   =   key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(),
                           [](unsigned char c) { return  (char)std::tolower(c); });
            request.headers.emplace(lowerKey, value);
        }
        return  request;
    }

    /// <summary>FE 响应 -> httplib 响应</summary>
    static  void    applyResponse(httplib::Response& res, const FEHttpResponse& response)
    {
        res.status  =   (int)response.status;
        res.body    =   response.body;
        res.set_header("Content-Type", response.contentType);
    }

    struct  FEHttpServer::Impl
    {
        httplib::Server     server;
        std::thread         thread;
        std::atomic<bool>   running { false };
        uint16              port    =   0;
        /// <summary>消息名 -> 专门处理函数(须在 start 前注册,与路由同一约束)</summary>
        std::map<String, FEMessageHandler>  messages;
    };

    FEHttpServer::FEHttpServer()
        :_impl(new Impl)
    {
        // 统一消息入口:/message?name=<消息名>,按消息名分发到注册的处理函数
        auto    entry   =   [this](const httplib::Request& req, httplib::Response& res)
        {
            auto    nameIt  =   req.params.find("name");
            if (nameIt == req.params.end())
            {
                res.status  =   400;
                res.body    =   "missing message name";
                return;
            }
            auto    msgIt   =   _impl->messages.find(nameIt->second);
            if (msgIt == _impl->messages.end())
            {
                res.status  =   404;
                res.body    =   "unknown message: " + nameIt->second;
                return;
            }

            FEHttpResponse  response;
            msgIt->second(toRequest(req), response);
            applyResponse(res, response);
        };

        _impl->server.Get("/message", entry);
        _impl->server.Post("/message", entry);
        _impl->server.Put("/message", entry);
        _impl->server.Delete("/message", entry);
        _impl->server.Patch("/message", entry);
        _impl->server.Options("/message", entry);
    }

    FEHttpServer::~FEHttpServer()
    {
        stop();
    }

    void    FEHttpServer::route(const String& method, const String& pattern, FEHttpHandler handler)
    {
        // httplib 回调 -> FE 纯数据封装;handler 异常由 httplib 捕获并回 500
        auto    wrap    =   [handler = std::move(handler)](const httplib::Request& req, httplib::Response& res)
        {
            FEHttpResponse  response;
            handler(toRequest(req), response);
            applyResponse(res, response);
        };

        String  upper   =   method;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return  (char)std::toupper(c); });

        if      (upper == "GET")        _impl->server.Get(pattern, wrap);
        else if (upper == "POST")       _impl->server.Post(pattern, wrap);
        else if (upper == "PUT")        _impl->server.Put(pattern, wrap);
        else if (upper == "DELETE")     _impl->server.Delete(pattern, wrap);
        else if (upper == "PATCH")      _impl->server.Patch(pattern, wrap);
        else if (upper == "OPTIONS")    _impl->server.Options(pattern, wrap);
    }

    void    FEHttpServer::get(const String& pattern, FEHttpHandler handler)
    {
        route("GET", pattern, std::move(handler));
    }

    void    FEHttpServer::post(const String& pattern, FEHttpHandler handler)
    {
        route("POST", pattern, std::move(handler));
    }

    void    FEHttpServer::onMessage(const String& name, FEMessageHandler handler)
    {
        _impl->messages[name]   =   std::move(handler);
    }

    bool    FEHttpServer::start(uint16 port, const String& host)
    {
        if (_impl->running)
            return  true;
        // bind_to_port 内部完成 socket+bind+listen,返回后端口已可连接,
        // accept 循环由 listen_after_bind 在工作线程中运行
        if (!_impl->server.bind_to_port(host, (int)port))
            return  false;
        _impl->port     =   port;
        _impl->running  =   true;
        _impl->thread   =   std::thread([this]
        {
            _impl->server.listen_after_bind();
            _impl->running  =   false;
        });
        return  true;
    }

    void    FEHttpServer::stop()
    {
        _impl->server.stop();
        if (_impl->thread.joinable())
            _impl->thread.join();
        _impl->running  =   false;
    }

    bool    FEHttpServer::isRunning() const
    {
        return  _impl->running;
    }

    uint16  FEHttpServer::port() const
    {
        return  _impl->port;
    }
}

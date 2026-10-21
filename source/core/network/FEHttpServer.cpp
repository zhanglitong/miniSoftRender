#include    "../../inc/network/FEHttpServer.hpp"

#include    <httplib.h>

#include    <atomic>
#include    <thread>

namespace   FE
{
    struct  FEHttpServer::Impl
    {
        httplib::Server     server;
        std::thread         thread;
        std::atomic<bool>   running { false };
        uint16              port    =   0;
    };

    FEHttpServer::FEHttpServer()
        :_impl(new Impl)
    {
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

            FEHttpResponse  response;
            handler(request, response);

            res.status  =   (int)response.status;
            res.body    =   response.body;
            res.set_header("Content-Type", response.contentType);
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


#include    <iostream>
#include    <assert.h>

#include    "../../inc/network/FEHttpServer.hpp"

// 测试专用:httplib 仅作为 HTTP 客户端与 FEHttpServer 通信,
// 验证 FEHttpServer 未在接口层暴露任何第三方库类型
#include    <httplib.h>

using   namespace   FE;

int     main(int , char**)
{
    const uint16    port    =   28899;

    FEHttpServer    server;

    // 1. 基础 GET:方法/路径/响应体
    server.get("/hello", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        assert(req.method   ==  "GET");
        assert(req.path     ==  "/hello");
        res.body    =   "hello world";
    });

    // 2. 查询参数解析
    server.get("/sum", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        assert(req.params.count("a") == 1);
        assert(req.params.count("b") == 1);
        int     a   =   atoi(req.params.at("a").c_str());
        int     b   =   atoi(req.params.at("b").c_str());
        res.body    =   std::to_string(a + b);
    });

    // 3. POST:请求体回显 + 请求头(键统一小写)
    server.post("/echo", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        assert(req.method   ==  "POST");
        res.body            =   req.body;
        res.contentType     =   req.headers.at("content-type");
    });

    // 4. 自定义响应 Content-Type(JSON)
    server.get("/json", [](const FEHttpRequest&, FEHttpResponse& res)
    {
        res.contentType     =   "application/json";
        res.body            =   "{\"ok\":true}";
    });

    // 5. 自定义请求头读取
    server.get("/header", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.body    =   req.headers.at("x-fe-test");
    });

    // 6. route 通用注册(方法名大小写不敏感)
    server.route("get", "/routeGet", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        assert(req.method == "GET");
        res.body    =   "route ok";
    });

    // 启动:非阻塞,端口立即可连接
    assert(server.start(port));
    assert(server.isRunning());
    assert(server.port() == port);

    httplib::Client cli("localhost", port);

    // 1. 基础 GET
    {
        auto    res =   cli.Get("/hello");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "hello world");
    }

    // 2. 查询参数
    {
        auto    res =   cli.Get("/sum?a=3&b=4");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "7");
    }

    // 3. POST 回显 + Content-Type 透传
    {
        auto    res =   cli.Post("/echo", "fe-http-body", "text/echo-custom");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "fe-http-body");
        assert(res->get_header_value("Content-Type") == "text/echo-custom");
    }

    // 4. JSON Content-Type
    {
        auto    res =   cli.Get("/json");
        assert(res);
        assert(res->status  ==  200);
        assert(res->get_header_value("Content-Type") == "application/json");
        assert(res->body    ==  "{\"ok\":true}");
    }

    // 5. 自定义请求头
    {
        httplib::Headers    headers;
        headers.emplace("X-FE-Test", "fe-value");
        auto    res =   cli.Get("/header", headers);
        assert(res);
        assert(res->body    ==  "fe-value");
    }

    // 6. route 通用注册
    {
        auto    res =   cli.Get("/routeGet");
        assert(res);
        assert(res->body    ==  "route ok");
    }

    // 7. 未注册路由 -> 404
    {
        auto    res =   cli.Get("/notfound");
        assert(res);
        assert(res->status  ==  404);
    }

    // 8. 停止后请求失败,同端口可重启
    server.stop();
    assert(!server.isRunning());
    {
        auto    res =   cli.Get("/hello");
        assert(!res);
    }
    assert(server.start(port));
    {
        httplib::Client cli2("localhost", port);
        auto    res =   cli2.Get("/hello");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "hello world");
    }

    server.stop();
    std::cout   <<  "[test.http] all passed"   <<  std::endl;
    return  0;
}

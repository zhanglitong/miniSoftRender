
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

    // 7. 消息处理:每种消息注册一个专门处理函数(GET 带查询参数)
    server.onMessage("add", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        assert(req.path == "/message");
        int     a   =   atoi(req.params.at("a").c_str());
        int     b   =   atoi(req.params.at("b").c_str());
        res.body    =   std::to_string(a + b);
    });

    // 8. 消息处理:POST 请求体回显
    server.onMessage("echo", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.body    =   req.body;
    });

    // 9. 同名消息重复注册 -> 后注册者生效
    server.onMessage("dup", [](const FEHttpRequest&, FEHttpResponse& res)
    {
        res.body    =   "first";
    });
    server.onMessage("dup", [](const FEHttpRequest&, FEHttpResponse& res)
    {
        res.body    =   "second";
    });

    // 9.5 流式消息:请求体分块到达,拼装块数与内容
    server.onStreamMessage("sstream", [](const FEHttpRequest& req, FEHttpResponse& res, FEStreamSource source)
    {
        assert(req.body.empty());                   /// 流式消息 body 恒为空
        String  body;
        int     chunks  =   0;
        source([&](const char* data, size_t size)
        {
            body.append(data, size);
            chunks++;
            return  true;
        });
        res.body    =   std::to_string(chunks) + ":" + body;
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

    // 7. 消息:GET 带查询参数
    {
        auto    res =   cli.Get("/message?name=add&a=3&b=4");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "7");
    }

    // 8. 消息:POST 请求体
    {
        auto    res =   cli.Post("/message?name=echo", "msg-body", "text/plain");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "msg-body");
    }

    // 9. 消息:同名重复注册,后注册者生效
    {
        auto    res =   cli.Get("/message?name=dup");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "second");
    }

    // 10. 消息:未注册消息名 -> 404
    {
        auto    res =   cli.Get("/message?name=nothing");
        assert(res);
        assert(res->status  ==  404);
        assert(res->body    ==  "unknown message: nothing");
    }

    // 11. 消息:缺 name 参数 -> 400
    {
        auto    res =   cli.Get("/message");
        assert(res);
        assert(res->status  ==  400);
    }

    // 11.5 流式消息:100KB 请求体必分多块,拼装后与原文一致
    {
        String  big;
        for (int i = 0; i < 10000; ++i)
            big +=  "0123456789";
        auto    res =   cli.Post("/stream?name=sstream", big, "application/octet-stream");
        assert(res);
        assert(res->status  ==  200);
        auto    pos     =   res->body.find(':');
        assert(pos != String::npos);
        assert(atoi(res->body.substr(0, pos).c_str()) >  1);    /// 确认实际分了多块
        assert(res->body.substr(pos + 1) == big);               /// 内容逐字节一致
    }

    // 11.6 流式消息:未注册消息名 -> 404
    {
        auto    res =   cli.Post("/stream?name=nothing", "x", "application/octet-stream");
        assert(res);
        assert(res->status  ==  404);
    }

    // 12. 未注册路由 -> 404
    {
        auto    res =   cli.Get("/notfound");
        assert(res);
        assert(res->status  ==  404);
    }

    // 13. 停止后请求失败,同端口可重启,消息注册保留
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

        // 重启后消息仍可用
        res =   cli2.Get("/message?name=dup");
        assert(res);
        assert(res->status  ==  200);
        assert(res->body    ==  "second");
    }

    server.stop();
    std::cout   <<  "[test.http] all passed"   <<  std::endl;
    return  0;
}


#include    <iostream>
#include    <assert.h>

#include    <chrono>
#include    <thread>

#include    <algorithm>
#include    <cctype>
#include    <ctime>
#include    <filesystem>
#include    <fstream>
#include    <memory>

#ifdef  _WIN32
#include    <windows.h>
#include    <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif

#include    "../../inc/network/FEHttpServer.hpp"

using   namespace   FE;

/// 测试页面:由服务器自身在 "/" 提供,浏览器打开 http://localhost:28900 即可点按钮发消息
static  const   char*   PAGE    =
R"HTML(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>FEHttpServer message test</title>
<style>
    body{font-family:Consolas,monospace;margin:2em;background:#1e1e1e;color:#ddd}
    fieldset{margin-bottom:1em;border:1px solid #555}
    button{cursor:pointer}
    #out{white-space:pre-wrap;border:1px solid #555;padding:1em;min-height:8em;background:#111}
</style>
</head>
<body>
<h3>FEHttpServer 消息测试 (/message?name=)</h3>
<fieldset><legend>add (GET, 查询参数)</legend>
    a <input id="a" value="3" size="4"> b <input id="b" value="4" size="4">
    <button onclick="msg('add','a='+a.value+'&b='+b.value)">GET add</button>
</fieldset>
<fieldset><legend>echo (POST, 请求体)</legend>
    <input id="body" value="hello fe" size="30">
    <button onclick="msgPost('echo',document.getElementById('body').value)">POST echo</button>
</fieldset>
<fieldset><legend>上传文件 → 服务端缓存 → 默认程序打开</legend>
    <input type="file" id="file">
    <button onclick="uploadFile('openfile')">整传 openfile</button>
    <button onclick="uploadFile('openstream')">流式 openstream</button>
</fieldset>
<fieldset><legend>异常路径</legend>
    <button onclick="msg('notexist','')">未注册消息 notexist (404)</button>
    <button onclick="raw('/message')">缺 name 参数 (400)</button>
</fieldset>
<fieldset><legend>更多消息</legend>
    <input id="rbody" value="hello" size="20">
    <button onclick="msgPost('reverse',document.getElementById('rbody').value)">POST reverse</button>
    <button onclick="msgPost('upper',document.getElementById('rbody').value)">POST upper</button>
    <button onclick="msg('time','')">GET time</button>
    <button onclick="msg('json','a=1&b=2')">GET json</button>
</fieldset>
<div id="out"></div>
<script>
const out = document.getElementById('out');
function log(s){ out.textContent = s + '\n' + out.textContent; }
async function request(method,url,body){
    try{
        const r = await fetch(url,{method:method,body:body});
        const t = await r.text();
        log(method + ' ' + url + '\n-> ' + r.status + ' (' + r.headers.get('Content-Type') + ')\n' + t + '\n');
    }catch(e){ log(method + ' ' + url + '\n-> ' + e + '\n'); }
}
function msg(n,q){ request('GET','/message?name='+n+(q?'&'+q:'')); }
function msgPost(n,b){ request('POST','/message?name='+n,b); }
function raw(u){ request('GET',u); }
function uploadFile(n){
    const f = document.getElementById('file').files[0];
    if(!f){ log('-> 请先选择文件\n'); return; }
    const url = (n == 'openstream' ? '/stream' : '/message') + '?name=' + n
              + '&filename=' + encodeURIComponent(f.name);
    request('POST', url, f);
}
</script>
</body>
</html>)HTML";

/// 上传缓存路径:upload_cache/<时间戳>_<文件名>;只取末段防路径穿越,按 UTF-8 解释防中文乱码
static  std::filesystem::path    cachePathFor(const String& filename)
{
    namespace   fs  =   std::filesystem;
    time_t      now     =   time(nullptr);
    struct  tm  lt      =   *localtime(&now);
    char        stamp[24]   =   {};
    strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &lt);
    fs::path    path    =   fs::u8path("upload_cache");
    path            /=  fs::u8path(stamp);
    path            +=  L"_";
    path            +=  fs::u8path(filename).filename();
    return  path;
}

int     main(int , char**)
{
    const uint16    port    =   28900;

    FEHttpServer    server;

    // 消息注册:每种消息一个专门处理函数
    server.onMessage("add", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        int     a   =   atoi(req.params.at("a").c_str());
        int     b   =   atoi(req.params.at("b").c_str());
        res.body    =   std::to_string(a + b);
    });
    server.onMessage("echo", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.body    =   req.body;
    });

    // 请求体反转:hello -> olleh
    server.onMessage("reverse", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.body.assign(req.body.rbegin(), req.body.rend());
    });

    // 请求体转大写:hello -> HELLO
    server.onMessage("upper", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.body    =   req.body;
        std::transform(res.body.begin(), res.body.end(), res.body.begin(),
                       [](unsigned char c) { return  (char)std::toupper(c); });
    });

    // 服务器当前时间
    server.onMessage("time", [](const FEHttpRequest&, FEHttpResponse& res)
    {
        time_t      now     =   time(nullptr);
        struct  tm  local   =   *localtime(&now);
        char        buf[32] =   {};
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local);
        res.body    =   buf;
    });

    // JSON 回显:请求体 + 查询参数
    server.onMessage("json", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        res.contentType =   "application/json";
        res.body        =   "{\"body\":\"" + req.body + "\"";
        for (const auto& [key, value] : req.params)
        {
            if (key != "name")
                res.body    +=   ",\"" + key + "\":\"" + value + "\"";
        }
        res.body        +=   "}";
    });

    // 上传文件:原始字节经请求体上传,服务端缓存到 upload_cache/ 后用系统默认程序打开
    server.onMessage("openfile", [](const FEHttpRequest& req, FEHttpResponse& res)
    {
        auto    nameIt  =   req.params.find("filename");
        if (nameIt == req.params.end() || nameIt->second.empty() || req.body.empty())
        {
            res.status  =   400;
            res.body    =   "need filename param and file body";
            return;
        }

        std::filesystem::path   path    =   cachePathFor(nameIt->second);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream   file(path, std::ios::binary);
        file.write(req.body.data(), (std::streamsize)req.body.size());
        bool    saved   =   file.good();

        bool    opened  =   false;
        if (saved)
        {
#ifdef  _WIN32
            // 用系统默认关联程序打开(异步返回,不阻塞服务)
            HINSTANCE   inst    =   ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            opened      =   (uintptr_t)inst >  32;
#else
            String  cmd =   "xdg-open '" + path.string() + "'";
            opened      =   system(cmd.c_str()) == 0;
#endif
        }

        res.contentType =   "application/json";
        res.body        =   String("{\"saved\":")    +   (saved  ?   "true"  :   "false")
                        +   ",\"path\":\""          +   path.generic_u8string()
                        +   "\",\"opened\":"        +   (opened ?   "true"  :   "false")   +   "}";
    });

    // 流式上传:请求体分块直写磁盘,内存占用与单块大小恒定(适合大文件);收完同样打开
    server.onStreamMessage("openstream", [](const FEHttpRequest& req, FEHttpResponse& res, FEStreamSource source)
    {
        auto    nameIt  =   req.params.find("filename");
        auto    lenIt   =   req.headers.find("content-length");
        if (nameIt == req.params.end() || nameIt->second.empty()
                || lenIt == req.headers.end() || lenIt->second == "0")
        {
            res.status  =   400;
            res.body    =   "need filename param and file body";
            return;
        }

        std::filesystem::path   path    =   cachePathFor(nameIt->second);
        std::filesystem::create_directories(path.parent_path());
        auto    file    =   std::make_shared<std::ofstream>(path, std::ios::binary);
        source([file](const char* data, size_t size) -> bool
        {
            file->write(data, (std::streamsize)size);
            return  file->good();
        });

        bool    saved   =   file->good();
        bool    opened  =   false;
        if (saved)
        {
#ifdef  _WIN32
            // 用系统默认关联程序打开(异步返回,不阻塞服务)
            HINSTANCE   inst    =   ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            opened      =   (uintptr_t)inst >  32;
#else
            String  cmd =   "xdg-open '" + path.string() + "'";
            opened      =   system(cmd.c_str()) == 0;
#endif
        }

        res.contentType =   "application/json";
        res.body        =   String("{\"saved\":")    +   (saved  ?   "true"  :   "false")
                        +   ",\"path\":\""          +   path.generic_u8string()
                        +   "\",\"opened\":"        +   (opened ?   "true"  :   "false")   +   "}";
    });

    // 测试页面
    server.get("/", [](const FEHttpRequest&, FEHttpResponse& res)
    {
        res.contentType     =   "text/html; charset=utf-8";
        res.body            =   PAGE;
    });

    assert(server.start(port));
    std::cout   <<  "listening on port " <<  port
                <<  ", open http://localhost:"  <<  port <<  "/ in browser"
                <<  std::endl   <<  "press Enter to stop..."   <<  std::endl;
    char    enter   =   0;
    if (!std::cin.get(enter))   /// istream& 重载:EOF 时为假
    {
        // stdin 已关闭(后台/管道启动):EOF 立即返回,转为常驻直到进程被终止
        for (;;) std::this_thread::sleep_for(std::chrono::hours(1));
    }
    server.stop();
    return  0;
}

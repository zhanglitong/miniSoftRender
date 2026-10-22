#include    "FEMcpServer.hpp"

#include    <stdexcept>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"

#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../inc/geometry/FEGeometryCone.hpp"
#include    "../../inc/geometry/FEGeometryPyramid.hpp"
#include    "../../inc/geometry/FEGeometryTorus.hpp"
#include    "../../inc/geometry/FEGeometryTorusKnot.hpp"
#include    "../../inc/geometry/FEGeometryCircle.hpp"
#include    "../../inc/geometry/FEGeometryRing.hpp"
#include    "../../inc/geometry/FEGeometryPlane.hpp"
#include    "../../inc/geometry/FEGeometryGrid.hpp"
#include    "../../inc/geometry/FEGeometryEllipsoidEGM.hpp"
#include    "../../inc/geometry/FEGeometryTube.hpp"

#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"
#include    "../../inc/animation/FEPlayback.hpp"

namespace   FE
{
    using   json    =   nlohmann::json;

    /// mesh 注册表:geo_create 产生 mesh 时登记 id,node_set_mesh/mesh_info 按 id 引用
    struct  FEMcpServer::MeshPool
    {
        std::map<String,Mesh>   items;
        uint32                  next    =   1;
    };

    namespace
    {
        /// <summary>geo 工厂默认属性流(与脚本绑定 defaultStreams 一致)</summary>
        static  Attrs   defaultStreams()
        {
            return  Attrs({IS_VERTEX_POS,IS_VERTEX_NOR,IS_VERTEX_TEXCOORD0});
        }
        /// <summary>读取必填 {x,y,z}</summary>
        static  real3   readV3(const json& v,const char* what)
        {
            if (!v.is_object()  ||  !v.contains("x")    ||  !v.contains("y")    ||  !v.contains("z"))
                throw   std::runtime_error(std::string("expect {x,y,z} for ") + what);
            return  real3((real)v["x"].get<double>(),
                          (real)v["y"].get<double>(),
                          (real)v["z"].get<double>());
        }
        /// <summary>读取必填 {x,y,z,w};成员赋值保证 (x,y,z,w) 分量顺序</summary>
        static  quatr   readQuat(const json& v,const char* what)
        {
            if (!v.is_object()  ||  !v.contains("x")    ||  !v.contains("y")    ||
                !v.contains("z")    ||  !v.contains("w"))
                throw   std::runtime_error(std::string("expect {x,y,z,w} for ") + what);
            quatr   q;
            q.x =   (real)v["x"].get<double>();
            q.y =   (real)v["y"].get<double>();
            q.z =   (real)v["z"].get<double>();
            q.w =   (real)v["w"].get<double>();
            return  q;
        }
        static  json    v3Json(const real3& v)
        {
            return  json{{"x",(double)v.x},{"y",(double)v.y},{"z",(double)v.z}};
        }
        static  json    quatJson(const quatr& q)
        {
            return  json{{"x",(double)q.x},{"y",(double)q.y},{"z",(double)q.z},{"w",(double)q.w}};
        }
        /// <summary>递归查找第一个同名节点(含顶层与全部孩子),与 fe.findNode 行为一致</summary>
        static  FENode* findByName(const Nodes& list,const String& name)
        {
            for (auto&  n   :   list)
            {
                if (n   ==  nullptr)
                    continue;
                Node    tmp =   n;
                if (tmp->name() ==  name)
                    return  tmp.get();
                if (FENode* sub =   findByName(tmp->children(),name))
                    return  sub;
            }
            return  nullptr;
        }
        /// <summary>按 args[key] 的节点名查找节点,缺失/不存在时抛错</summary>
        static  FENode* requireNode(FEScene& scene,const json& args,const char* key,const char* what)
        {
            if (!args.contains(key) ||  !args[key].is_string())
                throw   std::runtime_error(std::string("expect string '")   +   key + "' (node name) for "  +   what);
            String  name    =   args[key].get<String>();
            FENode* node    =   findByName(scene.nodeTree().topLevelNodes(),name);
            if (node    ==  nullptr)
                throw   std::runtime_error("node not found: "   +   name);
            return  node;
        }
        /// <summary>节点简况:名字/孩子/网格概要</summary>
        static  json    meshInfo(FEMesh* mesh,const String& id)
        {
            json    out     =   json::object();
            if (!id.empty())
                out["meshId"]   =   id;
            auto*   buf     =   mesh->get(IS_VERTEX_POS);
            out["vertexCount"]  =   buf ?   (double)buf->count()    :   0.0;
            json    attrs   =   json::array();
            for (auto&  attr :  mesh->inputs())
            {
                uint32  slot    =   (uint32)attr.slot();
                if (slot    &   IS_VERTEX_POS)              attrs.push_back("POSITION");
                else if (slot   &   IS_VERTEX_NOR)          attrs.push_back("NORMAL");
                else if (slot   &   IS_VERTEX_TEXCOORD0)    attrs.push_back("TEXCOORD0");
                else if (slot   &   IS_VERTEX_COLOR0)       attrs.push_back("COLOR0");
                else                                        attrs.push_back("slot-" +   std::to_string(slot));
            }
            out["attributes"]   =   attrs;
            const   aabb3f& box  =   mesh->aabb();
            const   auto&   mn  =   box.getMinimum();
            const   auto&   mx  =   box.getMaximum();
            out["aabb"]         =   json{{"min",v3Json(real3(mn.x,mn.y,mn.z))},
                                      {"max",v3Json(real3(mx.x,mx.y,mx.z))}};
            return  out;
        }
        static  json    nodeInfo(FENode* node)
        {
            json    out             =   json::object();
            out["name"]             =   node->name();
            json    kids            =   json::array();
            for (auto&  c   :   node->children())
                kids.push_back(c    ?   json(c->name())    :   json(nullptr));
            out["children"]         =   kids;
            out["childCount"]       =   (double)node->children().size();
            Mesh    mesh            =   node->mesh();
            if (mesh)
                out["mesh"]         =   meshInfo(mesh.get(),String());
            return  out;
        }
        /// <summary>schema 辅助:字段定义</summary>
        static  json    prop(const char* type,const char* desc)
        {
            return  json{{"type",type},{"description",desc}};
        }
        static  json    objSchema(json props,json required = json::array())
        {
            return  json{{"type","object"},{"properties",std::move(props)},{"required",std::move(required)}};
        }
        /// <summary>JSON-RPC 错误响应</summary>
        static  String  rpcError(const json& id,int code,const String& msg)
        {
            json    resp    =   json{
                {"jsonrpc","2.0"},
                {"id",id},
                {"error",json{{"code",code},{"message",msg}}}
            };
            return  resp.dump();
        }
        /// <summary>tools/call 结果响应:引擎结果序列化为 text content</summary>
        static  String  callResult(const json& id,const String& text,bool isError)
        {
            json    resp    =   json{
                {"jsonrpc","2.0"},
                {"id",id},
                {"result",json{
                    {"content",json::array({json{{"type","text"},{"text",text}}})},
                    {"isError",isError}
                }}
            };
            return  resp.dump();
        }
    }

    // ==================== 生命周期 ====================

    FEMcpServer::FEMcpServer(FEContext& ctx, FEScene& scene)
        :_ctx(&ctx)
        ,_scene(&scene)
        ,_meshes(new MeshPool())
    {
        /// 路由在 start 前注册(FEHttpServer 契约);stop 后重新 start 无需重复注册
        _http.post("/mcp",[this](const FEHttpRequest& req,FEHttpResponse& res)
        {
            /// HTTP 线程不直接执行工具(引擎/GPU 操作必须在主线程),
            /// 而是把请求入队,由主线程 update() 执行后回填 response
            auto    pending =   std::make_shared<PendingRequest>();
            pending->body   =   req.body;
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _pending.push(pending);
            }
            /// 等待主线程处理完成
            std::unique_lock<std::mutex> lk(_mutex);
            _cv.wait(lk,[&]{ return pending->done.load(); });
            String  out =   pending->response;
            if (out.empty())
            {
                /// 通知类请求:无响应体
                res.status      =   202;
                res.contentType =   "application/json";
            }
            else
            {
                res.status      =   200;
                res.contentType =   "application/json";
                res.body        =   out;
            }
        });
        _http.get("/",[this](const FEHttpRequest& req,FEHttpResponse& res)
        {
            (void)req;
            res.body    =   "miniSoftRender MCP server. POST /mcp (JSON-RPC 2.0), tools: "
                            +   std::to_string(toolCount());
        });
    }

    FEMcpServer::~FEMcpServer()
    {
        stop();
    }

    bool    FEMcpServer::start(uint16 port,const String& host)
    {
        _port   =   port;
        if (!_http.start(port,host))
        {
            _port   =   0;
            return  false;
        }
        return  true;
    }

    void    FEMcpServer::stop()
    {
        _http.stop();
        _port   =   0;
        /// 唤醒所有在 _cv 上等待的 HTTP handler,避免 stop 后死等
        _cv.notify_all();
    }

    void    FEMcpServer::update()
    {
        /// 主线程每帧调用:取出排队请求并在此线程执行 handleRpc
        /// (引擎/GPU 操作必须在主线程,否则 Vulkan 因线程亲和性崩溃)
        std::shared_ptr<PendingRequest>   pending;
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if (_pending.empty())
                return;
            pending =   _pending.front();
            _pending.pop();
        }
        /// 执行时不持锁,允许新请求继续入队;工具串行由主线程单帧保证
        pending->response  =   handleRpc(pending->body);
        pending->done      =   true;
        _cv.notify_all();
    }

    bool    FEMcpServer::isRunning() const
    {
        return  _http.isRunning();
    }

    uint16  FEMcpServer::port() const
    {
        return  _port;
    }

    // ==================== 工具表 ====================

    void    FEMcpServer::addTool(FEMcpTool tool)
    {
        for (auto&  t   :   _tools)
        {
            if (t.name  ==  tool.name)
            {
                t   =   std::move(tool);
                return;
            }
        }
        _tools.push_back(std::move(tool));
    }

    size_t  FEMcpServer::toolCount() const
    {
        return  _tools.size();
    }

    const   FEMcpTool*  FEMcpServer::findTool(const String& name) const
    {
        for (auto&  t   :   _tools)
        {
            if (t.name  ==  name)
                return  &t;
        }
        return  nullptr;
    }

    // ==================== JSON-RPC 协议 ====================

    String  FEMcpServer::handleRpc(const String& body)
    {
        json    req     =   json::parse(body,nullptr,false);
        if (req.is_discarded())
            return  rpcError(json(),-32700,"parse error");

        /// 通知类请求(无 id):initialized 等忽略即可
        if (!req.contains("id"))
            return  String();

        const   json    id      =   req.value("id",json());
        String  method          =   req.value("method",String());
        json    params          =   req.contains("params")  ?   req["params"]   :   json::object();

        json    result;
        try
        {
            if (method  ==  "initialize")
            {
                result  =   json{
                    {"protocolVersion","2024-11-05"},
                    {"capabilities",json{{"tools",json{{"listChanged",false}}}}},
                    {"serverInfo",json{{"name","miniSoftRender-mcp"},{"version","1.0.0"}}}
                };
            }
            else if (method ==  "ping")
            {
                result  =   json::object();
            }
            else if (method ==  "tools/list")
            {
                json    list    =   json::array();
                for (auto&  t   :   _tools)
                    list.push_back(json{{"name",t.name},{"description",t.description},{"inputSchema",t.inputSchema}});
                result  =   json{{"tools",list}};
            }
            else if (method ==  "tools/call")
            {
                String  name    =   params.value("name",String());
                const   FEMcpTool*  tool    =   findTool(name);
                if (tool    ==  nullptr)
                    return  callResult(id,"unknown tool: "  +   name,true);
                json    arguments   =   params.contains("arguments")    ?   params["arguments"] :   json::object();
                if (!arguments.is_object())
                    arguments   =   json::object();
                try
                {
                    json    out =   tool->handler(arguments);
                    return  callResult(id,out.is_string()   ?   out.get<String>()   :   out.dump(),false);
                }
                catch (const std::exception& e)
                {
                    return  callResult(id,e.what(),true);
                }
            }
            else
            {
                return  rpcError(id,-32601,"method not found: " +   method);
            }
        }
        catch (const std::exception& e)
        {
            return  rpcError(id,-32603,std::string("internal error: ")    +   e.what());
        }

        json    resp    =   json{{"jsonrpc","2.0"},{"id",id},{"result",result}};
        return  resp.dump();
    }

    // ==================== 引擎内置工具 ====================

    void    FEMcpServer::addEngineTools()
    {
        registerEngineTools();
    }

    void    FEMcpServer::registerEngineTools()
    {
        static  const   char*   kGEO_TYPES  =
            "box|sphere|cylinder|cone|pyramid|torus|torusknot|circle|ring|plane|grid|ellipsoid|tube";

        // ---------- scene ----------
        addTool({
            "scene_info",
            "列出场景全部节点(名字/孩子/网格概要)",
            objSchema(json::object()),
            [this](const json&)
            {
                json    nodes   =   json::array();
                std::function<void(const Nodes&)>   walk    =   [&](const Nodes& list)
                {
                    for (auto&  n   :   list)
                    {
                        if (n   ==  nullptr)
                            continue;
                        Node    tmp =   n;
                        nodes.push_back(nodeInfo(tmp.get()));
                        walk(tmp->children());
                    }
                };
                walk(_scene->nodeTree().topLevelNodes());
                return  json{{"nodes",nodes},{"count",(double)nodes.size()}};
            }
        });

        addTool({
            "node_create",
            "创建顶层节点",
            objSchema(json{{"name",prop("string","节点名,缺省为 node")}}),
            [this](const json& args)
            {
                String  name    =   args.value("name",String("node"));
                Node    node    =   new FENode(*_ctx);
                node->setName(name);
                _scene->nodeTree().addToplevelNode(node);
                return  nodeInfo(node.get());
            }
        });

        addTool({
            "node_find",
            "按名查找节点(含子树),返回节点简况",
            objSchema(json{{"name",prop("string","节点名")}},json{"name"}),
            [this](const json& args)
            {
                FENode* node    =   requireNode(*_scene,args,"name","node_find");
                return  nodeInfo(node);
            }
        });

        // ---------- geometry / mesh ----------
        addTool({
            "geo_create",
            std::string("创建参数化几何 mesh 并登记 meshId;类型: ")   +   kGEO_TYPES +
            ";参数与 three.js/Lua 绑定同名(box:size/segs, sphere:radius/widthSegments/heightSegments, "
            "cylinder:radiusTop/radiusBottom/height, torus:radius/tube, ring:innerRadius/outerRadius, "
            "plane:width/height, grid:size/divs, ellipsoid:size, tube 必须给 points)",
            objSchema(json{
                {"type",prop("string","几何类型")},
                {"params",prop("object","几何参数(可选,有默认值)")},
                {"segs",prop("object","分段 {x,y,z}(box/ellipsoid 可用)")}
            },json{"type"}),
            [this](const json& args)
            {
                String  type    =   args.value("type",String());
                static  const   json    kEmptyObj   =   json::object();
                const   json&   p   =   (args.contains("params")    &&  args["params"].is_object())
                                            ?   args["params"]
                                            :   kEmptyObj;
                auto    numF    =   [&](const char* k,double def)
                {
                    return  p.contains(k)  ?   (float)p.at(k).get<double>()   :   (float)def;
                };
                auto    numI    =   [&](const char* k,int def)
                {
                    return  p.contains(k)  ?   (uint)p.at(k).get<double>()  :   (uint)def;
                };

                Mesh    mesh;
                Attrs   streams =   defaultStreams();
                if (type    ==  "box")
                {
                    FEGeometryBox   geo(*_ctx);
                    json    sizeJ   =   p.contains("size") ?   p.at("size")     :   json{{"x",1},{"y",1},{"z",1}};
                    json    segsJ   =   p.contains("segs") ?   p.at("segs")     :   json{{"x",1},{"y",1},{"z",1}};
                    real3   size    =   readV3(sizeJ,"box size");
                    geo.param()._size   =   float3((float)size.x,(float)size.y,(float)size.z);
                    geo.param()._segs   =   uint3((uint)segsJ.value("x",1.0),(uint)segsJ.value("y",1.0),(uint)segsJ.value("z",1.0));
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "sphere")
                {
                    FEGeometrySphere    geo(*_ctx);
                    geo.param().radius          =   numF("radius",1.0);
                    geo.param().widthSegments   =   numI("widthSegments",16);
                    geo.param().heightSegments  =   numI("heightSegments",12);
                    geo.param().phiStart        =   numF("phiStart",0.0);
                    geo.param().phiLength       =   numF("phiLength",float(TWO_PI));
                    geo.param().thetaStart      =   numF("thetaStart",0.0);
                    geo.param().thetaLength     =   numF("thetaLength",float(PI));
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "cylinder")
                {
                    FEGeometryCylinder  geo(*_ctx);
                    geo.param().radiusTop       =   numF("radiusTop",1.0);
                    geo.param().radiusBottom    =   numF("radiusBottom",1.0);
                    geo.param().height          =   numF("height",1.0);
                    geo.param().radialSegments  =   numI("radialSegments",16);
                    geo.param().heightSegments  =   numI("heightSegments",1);
                    geo.param().thetaStart      =   numF("thetaStart",0.0);
                    geo.param().thetaLength     =   numF("thetaLength",float(TWO_PI));
                    geo.param().openEnded       =   p.value("openEnded",false);
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "cone")
                {
                    FEGeometryCone    geo(*_ctx);
                    geo.param().radiusBottom    =   numF("radius",1.0);
                    geo.param().height          =   numF("height",1.0);
                    geo.param().radialSegments  =   numI("radialSegments",32);
                    geo.param().heightSegments  =   numI("heightSegments",1);
                    geo.param().openEnded       =   p.value("openEnded",false);
                    geo.param().radiusTop       =   0.0f;
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "pyramid")
                {
                    FEGeometryPyramid   geo(*_ctx);
                    geo.param().radiusBottom    =   numF("radius",1.0);
                    geo.param().height          =   numF("height",1.0);
                    geo.param().radialSegments  =   4;
                    geo.param().openEnded       =   p.value("openEnded",false);
                    geo.param().radiusTop       =   0.0f;
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "torus")
                {
                    FEGeometryTorus    geo(*_ctx);
                    geo.param().radius          =   numF("radius",1.0);
                    geo.param().tube            =   numF("tube",0.4);
                    geo.param().radialSegments  =   numI("radialSegments",20);
                    geo.param().tubularSegments =   numI("tubularSegments",64);
                    geo.param().arc             =   numF("arc",float(TWO_PI));
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "torusknot")
                {
                    FEGeometryTorusKnot    geo(*_ctx);
                    geo.param().radius          =   numF("radius",1.0);
                    geo.param().tube            =   numF("tube",0.4);
                    geo.param().radialSegments  =   numI("radialSegments",16);
                    geo.param().tubularSegments =   numI("tubularSegments",64);
                    geo.param().p               =   numI("p",2);
                    geo.param().q               =   numI("q",3);
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "circle")
                {
                    FEGeometryCircle    geo(*_ctx);
                    geo.param().radius          =   numF("radius",1.0);
                    geo.param().thetaStart      =   numF("thetaStart",0.0);
                    geo.param().thetaLength     =   numF("thetaLength",float(TWO_PI));
                    geo.param().segments        =   numI("segments",16);
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "ring")
                {
                    FEGeometryRing    geo(*_ctx);
                    geo.param().innerRadius     =   numF("innerRadius",0.5);
                    geo.param().outerRadius     =   numF("outerRadius",1.0);
                    geo.param().thetaStart      =   numF("thetaStart",0.0);
                    geo.param().thetaLength     =   numF("thetaLength",float(TWO_PI));
                    geo.param().thetaSegments   =   numI("thetaSegments",16);
                    geo.param().phiSegments     =   numI("phiSegments",1);
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "plane")
                {
                    FEGeometryPlane    geo(*_ctx);
                    geo.param()._width  =   numF("width",1.0);
                    geo.param()._height =   numF("height",1.0);
                    geo.param()._wSegs  =   numI("wSegs",1);
                    geo.param()._hSegs  =   numI("hSegs",1);
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "grid")
                {
                    FEGeometryGrid    geo(*_ctx);
                    geo.param()._size   =   (uint32_t)numI("size",10);
                    geo.param()._divs   =   (uint32_t)numI("divs",10);
                    geo.param()._dir    =   p.value("dir",0);
                    streams =   Attrs({IS_VERTEX_POS,IS_VERTEX_COLOR0});
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "ellipsoid")
                {
                    FEGeometryEllipsoidEGM    geo(*_ctx);
                    json    sizeJ   =   p.contains("size") ?   p.at("size")     :   json{{"x",1},{"y",1},{"z",1}};
                    json    segsJ   =   p.contains("segs") ?   p.at("segs")     :   json{{"x",10},{"y",10},{"z",10}};
                    real3   size    =   readV3(sizeJ,"ellipsoid size");
                    geo.param()._size   =   float3((float)size.x,(float)size.y,(float)size.z);
                    geo.param()._segs   =   uint3((uint)segsJ.value("x",10.0),(uint)segsJ.value("y",10.0),(uint)segsJ.value("z",10.0));
                    mesh    =   geo.triangular(streams);
                }
                else if (type   ==  "tube")
                {
                    if (!p.contains("points")  ||  !p.at("points").is_array()    ||  p.at("points").size()  <   2)
                        throw   std::runtime_error("geo_create tube: expect params.points (array of {x,y,z}, at least 2)");
                    FEGeometryTube    geo(*_ctx);
                    geo.param().type            =   FECurveType::CatmullRom;
                    geo.param().radius          =   numF("radius",0.1);
                    geo.param().tubularSegments =   numI("tubularSegments",64);
                    geo.param().radialSegments  =   numI("radialSegments",8);
                    geo.param().closed          =   p.value("closed",false);
                    geo.param().tension         =   numF("tension",0.5);
                    for (auto&  pt  :   p.at("points"))
                    {
                        real3   v   =   readV3(pt,"tube point");
                        geo.param().points.push_back(float3((float)v.x,(float)v.y,(float)v.z));
                    }
                    mesh    =   geo.triangular(streams);
                }
                else
                {
                    throw   std::runtime_error(std::string("unknown geometry type: ")  +   type    +   "; valid: " +   kGEO_TYPES);
                }

                /// geo_create 只管建 mesh;材质在 node_set_mesh 时挂到节点上
                /// (无材质时 addNodesToFactory 不创建 render factory,
                ///  后续动画改 transform 会触发 onNodePropChanged 断言 factory!=nullptr)
                /// 登记注册表并返回概要(aabb 现算,给 LLM 真实包围盒)
                String  id      =   "mesh-" +   std::to_string(_meshes->next++);
                mesh->updateAabb();
                _meshes->items[id]  =   mesh;
                json    out     =   meshInfo(mesh.get(),id);
                out["type"]     =   type;
                return  out;
            }
        });

        addTool({
            "node_set_mesh",
            "把已创建的 mesh 挂到节点",
            objSchema(json{
                {"node",prop("string","节点名")},
                {"meshId",prop("string","geo_create 返回的 meshId")}
            },json{"node","meshId"}),
            [this](const json& args)
            {
                FENode* node    =   requireNode(*_scene,args,"node","node_set_mesh");
                String  id      =   args.value("meshId",String());
                auto    itr     =   _meshes->items.find(id);
                if (itr ==  _meshes->items.end())
                    throw   std::runtime_error("mesh not found: "   +   id);
                node->setMesh(itr->second.get());
                /// 挂默认 PBR 材质:无材质时渲染工厂不创建,动画驱动会断言
                node->setMaterial(new FEMaterialPBR(*_ctx));
                return  json{{"ok",true},{"node",node->name()},{"meshId",id}};
            }
        });

        addTool({
            "mesh_info",
            "查询 mesh 概要(顶点数/属性/包围盒);参数二选一: meshId 或 node",
            objSchema(json{
                {"meshId",prop("string","geo_create 返回的 meshId")},
                {"node",prop("string","节点名(读取节点当前 mesh)")}
            }),
            [this](const json& args)
            {
                if (args.contains("meshId"))
                {
                    String  id      =   args["meshId"].get<String>();
                    auto    itr     =   _meshes->items.find(id);
                    if (itr ==  _meshes->items.end())
                        throw   std::runtime_error("mesh not found: "   +   id);
                    return  meshInfo(itr->second.get(),id);
                }
                FENode* node    =   requireNode(*_scene,args,"node","mesh_info");
                Mesh    mesh    =   node->mesh();
                if (!mesh)
                    throw   std::runtime_error(std::string("node has no mesh: ")    +   node->name());
                return  meshInfo(mesh.get(),String());
            }
        });

        // ---------- transform ----------
        addTool({
            "node_set_transform",
            "设置节点本地变换;translation/rotation(x,y,z,w 四元数)/scaling 任选",
            objSchema(json{
                {"node",prop("string","节点名")},
                {"translation",prop("object","{x,y,z}")},
                {"rotation",prop("object","四元数 {x,y,z,w}")},
                {"scaling",prop("object","{x,y,z}")}
            },json{"node"}),
            [this](const json& args)
            {
                FENode* node    =   requireNode(*_scene,args,"node","node_set_transform");
                if (args.contains("translation"))
                    node->setLocalTranslation(readV3(args["translation"],"translation"));
                if (args.contains("rotation"))
                    node->setLocalRotation(readQuat(args["rotation"],"rotation"));
                if (args.contains("scaling"))
                    node->setLocalScaling(readV3(args["scaling"],"scaling"));
                return  nodeInfo(node);
            }
        });

        addTool({
            "node_get_transform",
            "读取节点本地变换与全局平移",
            objSchema(json{{"node",prop("string","节点名")}},json{"node"}),
            [this](const json& args)
            {
                FENode* node    =   requireNode(*_scene,args,"node","node_get_transform");
                return  json{
                    {"translation",v3Json(node->localTranslation())},
                    {"rotation",quatJson(node->localRotation())},
                    {"scaling",v3Json(node->localScaling())},
                    {"globalTranslation",v3Json(node->globalTranslation())}
                };
            }
        });

        // ---------- hierarchy ----------
        addTool({
            "node_set_parent",
            "改挂父节点;parent 为 null 时提升为顶层(保持世界变换)",
            objSchema(json{
                {"child",prop("string","子节点名")},
                {"parent",prop("string","新父节点名,null 提升为顶层")}
            },json{"child"}),
            [this](const json& args)
            {
                FENode* child   =   requireNode(*_scene,args,"child","node_set_parent");
                if (args.contains("parent") &&  !args["parent"].is_null())
                {
                    FENode* parent  =   requireNode(*_scene,args,"parent","node_set_parent");
                    if (parent  ==  child)
                        throw   std::runtime_error("cannot reparent to self");
                    child->reparent(parent);
                }
                else
                {
                    child->reparent(nullptr);
                }
                return  nodeInfo(child);
            }
        });

        addTool({
            "node_add_child",
            "把节点挂为另一节点的孩子",
            objSchema(json{
                {"parent",prop("string","父节点名")},
                {"child",prop("string","子节点名")}
            },json{"parent","child"}),
            [this](const json& args)
            {
                FENode* parent  =   requireNode(*_scene,args,"parent","node_add_child");
                FENode* child   =   requireNode(*_scene,args,"child","node_add_child");
                if (parent  ==  child)
                    throw   std::runtime_error("cannot add self as child");
                parent->addChild(Node(child));
                return  json{{"ok",true},{"children",(double)parent->children().size()}};
            }
        });

        addTool({
            "node_remove_child",
            "从父节点移除孩子(节点保留,成为顶层)",
            objSchema(json{
                {"parent",prop("string","父节点名")},
                {"child",prop("string","子节点名")}
            },json{"parent","child"}),
            [this](const json& args)
            {
                FENode* parent  =   requireNode(*_scene,args,"parent","node_remove_child");
                FENode* child   =   requireNode(*_scene,args,"child","node_remove_child");
                bool    ok      =   parent->removeChild(Node(child));
                if (!ok)
                    throw   std::runtime_error(std::string("not a child: ") +   child->name());
                return  json{{"ok",true}};
            }
        });

        // ---------- animation ----------
        addTool({
            "anim_translate",
            "给节点创建平移动画并播放;keyframes 至少 2 个 {t,x,y,z},loop 默认循环,play 默认立即播放",
            objSchema(json{
                {"node",prop("string","节点名")},
                {"keyframes",json{
                    {"type","array"},
                    {"description","关键帧 [{t:0,x:0,y:0,z:0},...]"},
                    {"items",json{{"type","object"}}}
                }},
                {"loop",prop("boolean","循环播放,默认 true")},
                {"play",prop("boolean","创建后立即播放,默认 true")}
            },json{"node","keyframes"}),
            [this](const json& args)
            {
                FENode* node    =   requireNode(*_scene,args,"node","anim_translate");
                if (!args.contains("keyframes") ||  !args["keyframes"].is_array()  ||
                    args["keyframes"].size()    <   2)
                    throw   std::runtime_error("anim_translate: expect keyframes (array of {t,x,y,z}, at least 2)");

                Animation   anim    =   FEAnimationHelper::createNodeAnimtion(*_ctx,NodeProperyBits(NP_TRANSFORM_XYZ));
                AnimClip    clip    =   anim->clip();
                if (!clip)
                    throw   std::runtime_error("anim_translate: animation has no clip");
                for (auto&  kf  :   args["keyframes"])
                {
                    if (!kf.contains("t"))
                        throw   std::runtime_error("anim_translate: keyframe expect 't'");
                    real    t   =   (real)kf["t"].get<double>();
                    real3   p   =   readV3(kf,"keyframe position");
                    if (!clip->addKeyFrame(t,KFValue(p)))
                        throw   std::runtime_error("anim_translate: addKeyFrame failed at t=" + std::to_string((double)t));
                }
                node->addComponent(anim.get());

                /// 动画系统仅由 FEScene::setup 注册:数据场景(未 setup)下只挂组件,
                /// 不走 dispatchNodesToSystem(会进渲染工厂,无渲染后端会崩溃)
                String  note;
                FEAnimationSys* sys     =   _scene->animationSystem();
                if (sys !=  nullptr)
                {
                    _scene->dispatchNodesToSystem({node});
                    FEPlayback*     pb      =   sys->getOrCreate(String("default"));
                    bool            loop    =   args.value("loop",true);
                    pb->setMode(loop    ?   FEPlayback::PT_Loop :   FEPlayback::PT_Once);
                    if (args.value("play",true))
                        pb->play();
                    note    =   "playing";
                }
                else
                {
                    note    =   "data mode: animation bound to node, takes effect when scene runs with animation system";
                }
                return  json{{"ok",true},{"action","default"},{"note",note},
                             {"keyframes",(double)args["keyframes"].size()}};
            }
        });

        addTool({
            "anim_stop",
            "停止播放指定 action(默认 default)",
            objSchema(json{{"action",prop("string","action 名,缺省 default")}}),
            [this](const json& args)
            {
                String  name    =   args.value("action",String("default"));
                /// 数据场景(未 setup)没有动画系统,无可停止的播放
                FEAnimationSys* sys     =   _scene->animationSystem();
                if (sys ==  nullptr)
                    return  json{{"ok",false},{"action",name},
                                 {"note","data mode: no animation system, nothing to stop"}};
                FEPlayback* pb  =   sys->getOrCreate(name);
                pb->stop();
                return  json{{"ok",true},{"action",name}};
            }
        });
    }
}

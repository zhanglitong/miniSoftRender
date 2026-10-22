/// test.lua.geo
/// Lua 脚本几何/数学绑定测试:
///   1. 创建引擎 + 渲染窗口 + 场景
///   2. 几何/节点/变换全部在 geo.lua 中通过 fe.geo.* / fe.vec3 / fe.quat 等绑定完成,
///      C++ 侧不创建任何 mesh/node
///   3. 脚本执行后 C++ 侧为带 mesh 的节点兜底挂共享 PBR 材质,并统一分发到渲染工厂
///      (dispatch 依赖材质,而材质绑定不在本次 Lua 范围)
///   4. 视觉预期:13 种参数化几何按 4x4 网格摆开
/// 用法: test.lua.geo.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../core/script/lua/FELuaVM.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 深度优先收集节点树全部节点(含顶层与各级孩子)
static  void    collectNodes(const Nodes& list,Nodes& out)
{
    for (auto& node : list)
    {
        if (node == nullptr)
            continue;
        out.push_back(node);
        collectNodes(node->children(),out);
    }
}

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId = RS_WEBGPU;
        }
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
        {
            rendererId = RS_VULKAN;
        }
    }

    FEContext&  ctx =   FEContext::instance();

    /// 场景指针在引擎创建后赋值;notify 把窗口消息转发给场景
    FEScene*    pScene  =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    /// 2. 创建引擎并初始化渲染后端
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 创建 Lua VM 并执行脚本:几何/数学/节点的创建全部在 geo.lua 中
    FELuaVM lua(ctx);
    lua.setScene(pScene);
    String  script  =   app->path() + "/geo.lua";
    if (!lua.doFile(script.c_str()))
    {
        std::cerr << "run " << script << " failed, see engine log" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 4. 为脚本创建的带 mesh 节点兜底挂共享 PBR 材质,统一分发到渲染工厂
    Nodes   scriptNodes;
    collectNodes(pScene->nodeTree().topLevelNodes(),scriptNodes);

    FEMaterialPBR*  pMat        =   new FEMaterialPBR(ctx);
    Material        sharedMat   =   pMat;
    pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
    pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
    pMat->data().update();
    for (auto& node : scriptNodes)
    {
        if (node->mesh() && node->material() == nullptr)
            node->setMaterial(sharedMat);
    }
    pScene->dispatchNodesToSystem(scriptNodes);
    ctx.log().infor("dispatched %d script node(s)",(int)scriptNodes.size());

    /// 5. 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的节点与材质
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        scriptNodes.clear();
        sharedMat   =   nullptr;
    });

    /// 6. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

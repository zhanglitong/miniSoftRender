/// test.js.anim
/// QuickJS 脚本驱动动画测试(与 test.lua.anim 对齐):
///   1. 创建引擎 + 渲染窗口 + 场景
///   2. 生成立方体 mesh,创建名为 "Box" 的节点并加入节点树
///   3. 动画的全部创建/绑定/播放逻辑都在 anim.js 脚本中完成,
///      C++ 侧只负责 FEQuickJS 的创建与脚本执行
///   4. 视觉预期:立方体在 (0,0,0) <-> (10,0,0) 之间循环平移
/// 用法: test.js.anim.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../core/script/quickjs/FEQuickJS.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

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

    /// 3. 生成立方体 mesh 并创建 Box 节点(仅此而已,动画由脚本创建)
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setName("Box");
    node->setMesh(mesh);

    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
    pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
    pMat->data().update();
    node->setMaterial(pMat);

    /// 加入节点树,供脚本 fe.findNode("Box") 查找
    scene->addNodesToTree({node});

    /// 4. 创建 QuickJS VM 并执行脚本:动画的创建/绑定/播放都在 anim.js 中
    FEQuickJS   js(ctx);
    js.setScene(pScene);
    String  script  =   app->path() + "/anim.js";
    if (!js.doFile(script.c_str()))
    {
        std::cerr << "run " << script << " failed, see engine log" << std::endl;
        scene   =   nullptr;
        node    =   nullptr;
        mesh    =   nullptr;
        app->destroy();
        return  1;
    }

    /// <summary>
    /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放 main 持有的节点与网格
    /// </summary>
    app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
    {
        node        =   nullptr;
        mesh        =   nullptr;
    });
    /// 5. 进入渲染循环
    app->run();

    /// 释放本地引用(ctx 仍持有一份,最终在 context 析构时安全释放)
    scene   =   nullptr;
    app->destroy();

    return  0;
}

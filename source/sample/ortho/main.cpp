/// sample.ortho
/// 正交相机验证示例:
///   1. 创建引擎 + 渲染窗口
///   2. 相机默认设置为正交投影模式
///   3. 场景内容:
///      - 3 个相同尺寸的立方体,沿 z 轴分布在不同深度(z = -2, 0, +2),
///        正交投影下三者屏幕尺寸应完全一致(透视模式下近处大远处小)
///      - 5x5 立方体阵列(位于 z = 0 平面),用于观察平行线在正交投影下保持平行
///   4. 按空格键可在 正交 / 透视 之间切换,直观对比两种投影的差异
///      正交模式下:等大立方体在任何深度都等大,阵列行列严格平行
///      透视模式下:近大远小,远处的行列会收敛
///   5. 锚点(点击物体设置,红点标记)旋转/缩放:
///      - 左键拖动:绕锚点水平/垂直公转,锚点屏幕位置不变
///      - 滚轮:以锚点为中心缩放,锚点始终位于光标下
/// 用法: sample.ortho.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

/// 用一个 box 几何 + 指定材质颜色创建节点
static  Node    createBoxNode(FEContext& ctx, real boxSize, const float4& diffuse, const float4& emissive)
{
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(boxSize, boxSize, boxSize);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setMesh(mesh);

    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   diffuse;
    pMat->data()._value._emissive =   emissive;
    pMat->data().update();
    node->setMaterial(pMat);
    return  node;
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

    /// 场景指针在引擎创建后赋值;notify 把窗口消息(更新/渲染/缩放/输入)转发给场景
    FEScene*    pScene  =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        /// 拦截空格键(0x20)切换正交/透视投影
        if (msg.msgId() == MSG_KEYDOWN && pScene)
        {
            const auto& keyMsg =   static_cast<const MsgKeyDown&>(msg);
            if (keyMsg._info._key == 0x20) /// VK_SPACE
            {
                auto    cam     =   pScene->camera();
                bool    next    =   !cam->isOrtho();
                cam->setOrtho(next);
                std::cout << "camera projection: " << (next ? "ORTHO" : "PERSPECTIVE") << std::endl;
                return;
            }
        }
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

    /// 2.5 相机设置:
    ///     eye=(0,0,-6), target=(0,0,0), 沿 -Z 观察
    ///     默认开启正交投影,缩放系数 _scaler 由 FECamera 构造函数初始化为 100
    ///     视口 1280x720 下,可见区域约为 12.8 x 7.2 世界单位
    scene->camera()->setEye(real3(0.0, 0.0, -6.0));
    scene->camera()->setTarget(real3(0.0, 0.0, 0.0));
    scene->camera()->setOrtho(true);

    Nodes   allNodes;

    /// 3. 深度验证组:3 个相同尺寸立方体,沿 z 轴分布在不同深度
    ///    正交投影下三者屏幕尺寸应完全一致
    {
        const real  boxSize =   0.6f;
        const real  zPositions[3] =   { -2.0f, 0.0f, 2.0f };
        const float4 colors[3] =
        {
            float4(0.95f, 0.25f, 0.15f, 1.0f), /// 红:远(z=-2)
            float4(0.20f, 0.85f, 0.30f, 1.0f), /// 绿:中(z= 0)
            float4(0.15f, 0.45f, 0.95f, 1.0f), /// 蓝:近(z=+2)
        };
        for (int i = 0; i < 3; ++i)
        {
            Node    box =   createBoxNode(ctx, boxSize, colors[i],
                                 float4(colors[i].x * 0.5f, colors[i].y * 0.5f, colors[i].z * 0.5f, 1.0f));
            box->setLocalTranslation(real3(-3.5f, 0.0f, zPositions[i]));
            box->update();
            allNodes.push_back(box);
        }
    }

    /// 4. 平行性验证组:5x5 立方体阵列(z=0 平面)
    ///    正交投影下各行各列应严格平行,无透视收敛
    {
        const real  boxSize =   0.4f;
        const real  spacing =   1.0f;
        const int   count   =   5;
        const real  start   =   -spacing * (count - 1) * 0.5f;
        for (int ix = 0; ix < count; ++ix)
        {
            for (int iy = 0; iy < count; ++iy)
            {
                /// 棋盘格颜色,便于分辨行列
                bool    even    =   ((ix + iy) & 1) == 0;
                float4  diffuse =   even ? float4(0.85f, 0.85f, 0.90f, 1.0f)
                                         : float4(0.35f, 0.35f, 0.40f, 1.0f);
                Node    box =   createBoxNode(ctx, boxSize, diffuse,
                                     float4(diffuse.x * 0.3f, diffuse.y * 0.3f, diffuse.z * 0.3f, 1.0f));
                box->setLocalTranslation(real3(
                    start + ix * spacing,
                    start + iy * spacing,
                    0.0f));
                box->update();
                allNodes.push_back(box);
            }
        }
    }

    /// 5. 节点分发到渲染工厂与各组件系统
    scene->dispatchNodesToSystem(allNodes);
    /// 添加到模型树，支持拾取
    scene->addNodesToTree(allNodes);

    std::cout << "ortho camera running:" << std::endl;
    std::cout << "  - default: orthographic projection" << std::endl;
    std::cout << "  - 3 same-size boxes at different z (left column): should be equal size in ortho" << std::endl;
    std::cout << "  - 5x5 box grid (right): rows/columns stay parallel in ortho" << std::endl;
    std::cout << "  - press SPACE to toggle ortho/perspective" << std::endl;
    std::cout << "  - click an object to set anchor (red dot marker)" << std::endl;
    std::cout << "  - left-drag: orbit around anchor (anchor stays fixed)" << std::endl;
    std::cout << "  - right-drag: pan" << std::endl;
    std::cout << "  - wheel: zoom at anchor (anchor stays under cursor)" << std::endl;

    /// 6. 进入渲染循环
    app->run();

    /// 释放本地引用
    scene   =   nullptr;
    app->destroy();

    return  0;
}

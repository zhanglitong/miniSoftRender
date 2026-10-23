/// test.box.r16g16b16sscaled
/// 顶点格式 FMT_R16G16B16_SSCALED 绘制立方体测试:
///   1. 创建引擎 + 渲染窗口
///   2. 用 FEGeometryBox 生成 mesh,位置与法线属性均使用 FMT_R16G16B16_SSCALED
///      (SSCALED 会把 float 四舍五入到 int16,单位立方体的 ±0.5 坐标会被量化为 ±1,
///       渲染结果为边长 2 的立方体,形状完整,用于校验该格式的打包/上传/采样链路)
///   3. 经 FEMaterialPBR 分发到渲染工厂并进入渲染循环显示
/// 用法: test.box.r16g16b16sscaled.exe [--vulkan|--webgpu]

#include    <iostream>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

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

    /// 3. 用 FEGeometryBox 生成立方体 mesh,位置/法线使用 FMT_R16G16B16_SSCALED
    bool    passed  =   false;
    {
        FEGeometryBox   geo(ctx);
        geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
        geo.param()._segs  =   uint3(1, 1, 1);

        Mesh    mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R16G16B16_SSCALED},
            {IS_VERTEX_NOR,       FMT_R16G16B16_SSCALED},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

        /// 校验几何:位置/法线缓冲顶点数、属性缓冲数、绘制图元数
        uint    posCount    =   0;
        uint    norCount    =   0;
        if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
            posCount    =   p->count();
        if (auto* n = mesh->get(FEAttribute(IS_VERTEX_NOR)))
            norCount    =   n->count();
        std::cout << "box.sscaled buffers=" << mesh->buffers().size()
                  << " pos=" << posCount
                  << " nor=" << norCount
                  << " primitives=" << mesh->primitives().size() << std::endl;

        /// 校验 calcAabb 对 R16G16B16_SSCALED 的解析:
        /// 顶点坐标 ±0.5 被量化为 int16 ±1,包围盒应为 [-1,-1,-1]~[1,1,1]
        /// 注意 aabb() 返回缓存值,需先 updateAabb() 才会按顶点格式重新计算
        mesh->updateAabb();
        const   auto&   box     =   mesh->aabb();
        const   auto&   boxMin  =   box.getMinimum();
        const   auto&   boxMax  =   box.getMaximum();
        std::cout << "box.sscaled aabb=[" << boxMin.x << "," << boxMin.y << "," << boxMin.z
                  << "]-[" << boxMax.x << "," << boxMax.y << "," << boxMax.z << "]" << std::endl;

        passed  =   (mesh->buffers().size() >= 3)
                 && (posCount > 0)
                 && (norCount == posCount)
                 && (!mesh->primitives().empty());

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            /// 明亮漫反射 + 自发光,SSCALED 量化后边长为 2,默认相机下清晰可见
            /// 使用 pbrSScaled 管线:其顶点声明 POS/NOR 为 R16G16B16_SSCALED,
            /// 与 mesh 属性格式一致,否则 buildBuffer 会按 FLOAT32 stride 拷贝导致数据错位
            FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx, "pbrSScaled");
            pMat->data()._value._diffuse  =   float4(0.95f, 0.35f, 0.20f, 1.0f);
            pMat->data()._value._emissive =   float4(0.40f, 0.12f, 0.05f, 1.0f);
            pMat->data().update();
            node->setMaterial(pMat);
            /// SSCALED 把 ±0.5 四舍五入为 int16 ±1,GPU 还原为 float ±1.0,
            /// 几何边长从 1 变为 2;此处缩放到 0.5 补偿,使视觉边长仍为 1
            node->setLocalScaling(real3(0.5f));
            /// 旋转一定角度,呈现多个面以体现 3D 立体感
            node->setLocalRotation(quatr(glm::radians(real3(25.0, 35.0, 0.0))));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "box.sscaled build validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "box.sscaled build ok (FMT_R16G16B16_SSCALED), entering render loop..." << std::endl;

    /// 4. 进入渲染循环显示立方体
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

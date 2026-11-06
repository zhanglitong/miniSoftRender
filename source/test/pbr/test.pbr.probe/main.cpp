/// test.pbr.probe
/// 实时反射探针验证测试(无头离屏,Vulkan):
///   1. 场景中放一个镜面玻璃球(球心为原点),它是场景里的普通对象
///   2. FEScene::createReflectionProbe(球节点, 256) 创建 cubemap 探针
///   3. 探针每帧在主趟之前把场景渲进 cubemap,材质 setEnvironmentMap(cube)
///   4. 红色立方体放在相机背后(z=+9):主画面看不到它,
///      但球面反射(+Z 方向)能看到 —— 用于证明反射来自探针而非屏幕空间
/// 验证点:
///   a. 探针创建成功,cubemap 为 6 面、单 mip、尺寸 256
///   b. 材质环境贴图与探针 cubemap 为同一对象
///   c. 渲染后球心区域为红色(反射到相机背后的红立方体)
///   d. 解除环境贴图后再渲染,球心不再为红色(回落解析天空 IBL),
///      反向证明 c 的红色来自探针
/// 结果写入 verify_probe.txt,并保存 verify_probe.png。
/// 返回:0=全部通过,1=初始化失败,2=校验失败
/// 用法: test.pbr.probe.exe [--vulkan]

#include    <iostream>
#include    <filesystem>
#include    <fstream>
#include    <Windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FEReflectionProbe.h"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    int     g_passed    =   0;
    int     g_failed    =   0;

    void    check(bool condition,const char* message)
    {
        if (condition)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
            std::cerr << "[FAILED] " << message << std::endl;
        }
    }
    String  exePath()
    {
        char    szTemp[2048]    =   {};
        GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
        std::filesystem::path   filePath(szTemp);
        return  filePath.parent_path().string();
    }

    Mesh    buildSphere(FEContext& ctx,float radius)
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   radius;
        geo.param().widthSegments   =   64;
        geo.param().heightSegments  =   48;
        return  geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
    }
    Mesh    buildBox(FEContext& ctx)
    {
        FEGeometryBox   geo(ctx);
        return  geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
    }

    /// 中心区域是否红色主导。
    /// 判定阈值取 8 而非更严格的值:默认天空的辐照度很高,
    /// 红色漫反射材质被解析天空 IBL 冲淡(红通道饱和、绿蓝通道被抬到 200+),
    /// 但红仍是三通道中最强的,与"无环境贴图时的天空蓝"差异明显。
    bool    centerIsRed(const uint8s& pixels,uint width,uint height)
    {
        const uint  x0  =   width  * 2 / 5;
        const uint  x1  =   width  * 3 / 5;
        const uint  y0  =   height * 2 / 5;
        const uint  y1  =   height * 3 / 5;
        uint64  sr = 0,sg = 0,sb = 0,cnt = 0;
        for (uint y = y0; y < y1; ++y)
        {
            for (uint x = x0; x < x1; ++x)
            {
                const uint8*    p   =   &pixels[(y * width + x) * 4];
                sr += p[0]; sg += p[1]; sb += p[2];
                ++cnt;
            }
        }
        if (cnt == 0)
            return  false;
        const uint64    r   =   sr / cnt;
        const uint64    g   =   sg / cnt;
        const uint64    b   =   sb / cnt;
        const bool  red =   (r > g + 8 && r > b + 8);
        std::cout << "  center avg rgb = (" << r << "," << g << "," << b << ")"
                  << " redPass=" << (red ? 1 : 0) << std::endl;
        return  red;
    }
}

int     main(int argc,char** argv)
{
    UNUSED(argc,argv);

    FEContext&  ctx     =   FEContext::instance();
    const String    path    =   exePath();

    /// 1. 无头离屏渲染器(Vulkan)
    OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);
    FEOffscreenRenderer::CreateInfo renderInfo  =   {};
    renderInfo._width       =   256;
    renderInfo._height      =   256;
    renderInfo._rendererId  =   RS_VULKAN;
    renderInfo._workPath    =   path;
    renderInfo._resourcePath=   path + "/../";
    if (!renderer->create(renderInfo))
    {
        std::cerr << "FEOffscreenRenderer::create failed" << std::endl;
        renderer    =   nullptr;
        return  1;
    }
    Scene   scene   =   renderer->scene();
    if (scene == nullptr)
    {
        std::cerr << "renderer->scene() == nullptr" << std::endl;
        renderer    =   nullptr;
        return  1;
    }
    scene->setClearColor(float4(0,0,0,1));

    /// 2. 相机:+Z 侧看向原点
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0,0.0,6.0));
        cam->setTarget(real3(0.0,0.0,0.0));
        cam->setUp(real3(0.0,1.0,0.0));
        cam->setFov(DEG2RAD(45));
        cam->setNearFar(real2(0.05,256.0));
        cam->updatePerspective();
    }

    /// 3. 红色挡板:放在相机背后(z=+10),主画面不可见,
    ///    只能通过球面(+Z 方向)反射看到。面积要足够大:
    ///    镜面球把球心附近很小的立体角压缩到几个像素,挡板太小反射区会小到测不到
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(buildBox(ctx));
        node->setLocalScaling(real3(18.0,18.0,0.4));
        node->setLocalTranslation(real3(0.0,0.0,10.0));
        auto*   mat =   new FEMaterialPBR(ctx);
        /// 纯红(不加绿蓝分量):天空 IBL 会把漫反射整体抬亮,
        /// 保留 0.08 的绿蓝会让最后的红色判定余量过小
        mat->data()._value._diffuse   =   float4(1.0f,0.0f,0.0f,1.0f);
        mat->data()._value._metallic  =   0.0f;
        mat->data()._value._roughness =   0.9f;
        mat->data().update();
        node->setMaterial(mat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }

    /// 4. 镜面玻璃球(球心原点)
    Node            ballNode;
    FEMaterialPBR*  ballMat =   nullptr;
    {
        ballNode    =   new FENode(ctx);
        ballNode->setMesh(buildSphere(ctx,1.2f));
        ballNode->setLocalTranslation(real3(0.0,0.0,0.0));
        ballMat     =   new FEMaterialPBR(ctx);
        ballMat->data()._value._diffuse   =   float4(1.0f,1.0f,1.0f,1.0f);
        /// 全金属 + 低粗糙度:球面近似镜面,清晰映射探针 cubemap
        ballMat->data()._value._metallic  =   1.0f;
        ballMat->data()._value._roughness =   0.05f;
        ballMat->data().update();
        ballNode->setMaterial(ballMat);
        ballNode->makeDirty();
        ballNode->update();
        scene->dispatchNodesToSystem({ballNode});
    }

    /// 5. 创建反射探针(球心位置),并把 cubemap 交给球的材质
    ReflectionProbe probe   =   scene->createReflectionProbe(ballNode,256);
    check(probe != nullptr,"createReflectionProbe failed");
    if (probe == nullptr)
    {
        renderer    =   nullptr;
        return  1;
    }
    check(probe->isValid(),"probe is not valid");
    check(probe->size() == 256,"probe size != 256");
    check(probe->cube() != nullptr && probe->cube()->isCube(),"probe cube texture invalid");
    check(probe->cube() != nullptr && probe->cube()->mips() == 1,"probe cube mips != 1");

    ballMat->setEnvironmentMap(probe->cube());
    check(ballMat->environmentMap().get() == probe->cube().get(),
          "material environment map != probe cube");

    /// 6. 渲染并回读
    check(renderer->render(4),"render(probe) failed");
    check(renderer->pixels().size() == 256 * 256 * 4,"pixels size != 256*256*4");

    const String    pngFile =   path + "/verify_probe.png";
    std::filesystem::remove(pngFile);
    check(renderer->save(pngFile.c_str()),"save verify_probe.png failed");
    check(std::filesystem::exists(pngFile),"saved png does not exist");

    /// 7. 球心区域应反射到相机背后的红立方体
    const bool  probeRed    =   centerIsRed(renderer->pixels(),256,256);
    check(probeRed,"center is not red (probe reflection failed)");

    /// 8. 反向验证:解除环境贴图后球心不再为红色(回落解析天空 IBL)
    ballMat->setEnvironmentMap(nullptr);
    check(renderer->render(4),"render(no env) failed");
    const bool  noProbeRed  =   centerIsRed(renderer->pixels(),256,256);
    check(!noProbeRed,"center still red without env map (reflection not from probe)");

    /// 9. 写结果
    {
        std::ofstream   out(path + "/verify_probe.txt");
        out << "probe.size       = " << probe->size() << "\n"
            << "probe.cube.mips  = " << probe->cube()->mips() << "\n"
            << "center.red(probe)= " << (probeRed   ? 1 : 0) << "\n"
            << "center.red(none) = " << (noProbeRed? 1 : 0) << "\n"
            << "passed           = " << g_passed << "\n"
            << "failed           = " << g_failed << "\n";
    }
    std::cout << "PASSED: " << g_passed << "  FAILED: " << g_failed << std::endl;

    ballMat     =   nullptr;
    ballNode    =   nullptr;
    probe       =   nullptr;
    renderer    =   nullptr;
    return  (g_failed == 0) ? 0 : 2;
}

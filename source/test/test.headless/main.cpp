/// test.headless
/// 无头(离屏)渲染测试(通过 FEOffscreenRenderer 工具类驱动):
///   1. 创建后清屏回读,校验清屏色
///   2. 用 FEMaterialV3C4 + 大三角形验证几何确实被绘制到离屏目标
///   3. resize 后重新渲染,校验回读尺寸与像素
///   4. 保存 PNG 并校验文件产物
/// 用法: test.headless.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <filesystem>
#include    <Windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/mesh/FEMeshBuilder.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

static  int     g_passed    =   0;
static  int     g_failed    =   0;

static  void    check(bool condition,const char* message)
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

static  String  exePath()
{
    char    szTemp[2048]    =   {};
    GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
    std::filesystem::path   filePath(szTemp);
    return  filePath.parent_path().string();
}

static  bool    pixelIs(const uint8s& pixels,uint width,uint x,uint y
                       ,uint8 r,uint8 g,uint8 b,uint8 a)
{
    const uint8*    p   =   &pixels[(y * width + x) * 4];
    return  p[0] == r && p[1] == g && p[2] == b && p[3] == a;
}

int     main(int argc,char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
    }

    FEContext&  ctx     =   FEContext::instance();
    const String    path    =   exePath();

    OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);

    FEOffscreenRenderer::CreateInfo renderInfo  =   {};
    renderInfo._width       =   32;
    renderInfo._height      =   32;
    renderInfo._rendererId  =   rendererId;
    renderInfo._workPath    =   path;
    renderInfo._resourcePath=   path + "/../";
    if (!renderer->create(renderInfo))
    {
        std::cerr << "FEOffscreenRenderer::create failed" << std::endl;
        renderer    =   nullptr;
        return  1;
    }

    Scene   scene   =   renderer->scene();
    check(scene != nullptr,"renderer->scene() == nullptr");

    /// 用例 1:纯清屏色回读
    {
        scene->setClearColor(float4(1,0,0,1));
        check(renderer->render(),"render() failed");
        check(renderer->width()  ==  32,"width != 32");
        check(renderer->height() ==  32,"height != 32");

        const uint8s&   pixels  =   renderer->pixels();
        check(pixels.size() == 32 * 32 * 4,"pixels size != 32*32*4");
        check(pixelIs(pixels,32,0,0,255,0,0,255),"corner pixel != clear red");
        check(pixelIs(pixels,32,31,31,255,0,0,255),"corner pixel(31,31) != clear red");
        check(pixelIs(pixels,32,31,0,255,0,0,255),"corner pixel(31,0) != clear red");
    }

    /// 用例 2:几何绘制(蓝色背景 + 覆盖视口的绿色三角形)
    {
        scene->setClearColor(float4(0,0,1,1));

        float3s     poss    =   {float3(-2,-2,0),float3(2,-2,0),float3(0,2,0)};
        Rgba8s      colors  =   {Rgba8(0,255,0,255),Rgba8(0,255,0,255),Rgba8(0,255,0,255)};
        Mesh        mesh    =   FEMeshBuilder::makeMesh(ctx,poss,colors);
        check(mesh != nullptr,"triangle mesh build failed");

        if (mesh)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            node->setMaterial(new FEMaterialV3C4(ctx));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }

        check(renderer->render(),"render(triangle) failed");

        const uint8s&   pixels  =   renderer->pixels();
        /// 角落在三角形之外,应保持清屏蓝
        check(pixelIs(pixels,32,0,0,0,0,255,255),"corner pixel != clear blue");
        /// 三角形在视口中下部覆盖屏幕,取靠下中间位置的像素
        check(pixelIs(pixels,32,16,28,0,255,0,255),"triangle pixel(16,28) != green");
        check(pixelIs(pixels,32,8,26,0,255,0,255),"triangle pixel(8,26) != green");
    }

    /// 用例 3:尺寸切换后重新渲染与回读
    {
        check(renderer->resize(64,24),"resize(64,24) failed");
        scene->setClearColor(float4(0,1,0,1));
        check(renderer->render(),"render(resize) failed");

        check(renderer->width()  ==  64,"width != 64 after resize");
        check(renderer->height() ==  24,"height != 24 after resize");

        const uint8s&   pixels  =   renderer->pixels();
        check(pixels.size() == 64 * 24 * 4,"pixels size != 64*24*4 after resize");
        check(pixelIs(pixels,64,0,0,0,255,0,255),"corner pixel != clear green after resize");
        check(pixelIs(pixels,64,63,23,0,255,0,255),"corner pixel(63,23) != clear green after resize");
    }

    /// 用例 4:保存 PNG
    {
        const String    fileName    =   path + "/headless_out.png";
        std::filesystem::remove(fileName);

        check(renderer->save(fileName.c_str()),"save(png) failed");
        check(std::filesystem::exists(fileName),"saved png does not exist");
        check(std::filesystem::file_size(fileName) > 0,"saved png is empty");
        std::filesystem::remove(fileName);
    }

    renderer    =   nullptr;
    scene       =   nullptr;

    std::cout << "=== " << g_passed << " passed, " << g_failed << " failed ===" << std::endl;
    return  g_failed == 0 ? 0 : 1;
}

/// test.pbr.transmission
/// transmission 架构验证测试(PBR 材质自动自取场景折射纹理,无头离屏):
///   1. 创建无头离屏引擎(FEOffscreenRenderer),不依赖窗口/后台线程
///   2. 场景:背景红色立方体(opaque PBR,z=-3),前景白色球体(transmission=1,z=+1)
///      —— 渲染时透过球体应看到折射后的红色立方体
///   3. FEScene 在 resize 时创建场景折射纹理并注册到 device cache
///      (FEConstUuid::SceneTexId);PBR 材质渲染时经 refreshSceneTexture 按帧
///      节流自取并自动重绑,无需场景遍历挂接
/// 验证点(主线程,渲染 4 帧后判定):
///   a. device cache 中能查询到 FEConstUuid::SceneTexId(FEScene 已注册)
///   b. 玻璃材质 isTransmission()==true(引擎按材质/工厂查询趟数,无全局静态)
///   c. 玻璃材质的 sceneTexture() 与 cache 中纹理为同一对象
///   d. sceneTex mip 链:Vulkan 满链(>1),WebGPU 单 mip(==1)
///   e. volume 参数已写入 UBO(thickness=2.0, attDist=1.5)
///   f. 像素级校验:球体中心区域红色分量显著高于绿/蓝(折射生效;若单趟
///      渲染则 center 无漫反射、无折射,基本为暗色)
///   g. resize 回归:sceneTex 重建后 device cache 同步更新(同 ID 覆盖),
///      材质下一帧重绑到新纹理,中心区域仍为红色
/// 结果写入 verify_transmission.txt,并保存 verify_transmission.png。
/// 返回:0=全部通过,1=初始化失败,2=校验失败
/// 用法: test.pbr.transmission.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <filesystem>
#include    <Windows.h>
#include    <cstdio>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FEConstUuid.h"
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

    /// 创建 PBR 球体 mesh(pos/nor/uv)
    Mesh    buildSphere(FEContext& ctx,float radius)
    {
        FEGeometrySphere    geo(ctx);
        geo.param().radius          =   radius;
        geo.param().widthSegments   =   48;
        geo.param().heightSegments  =   32;
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

    /// 中心区域是否红色主导(折射后的红立方体 + Beer-Lambert 红吸收)
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
        std::cout << "  center avg rgb = (" << r << "," << g << "," << b << ")"
                  << " redPass=" << ((r > 40 && r > g + 15 && r > b + 15) ? 1 : 0)
                  << std::endl;
        return  r > 40 && r > g + 15 && r > b + 15;
    }
}

int     main(int argc,char** argv)
{
    /// 选择渲染后端,默认 Vulkan
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
    }

    FEContext&  ctx     =   FEContext::instance();
    const String    path    =   exePath();

    /// 1. 无头离屏渲染器
    OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);
    FEOffscreenRenderer::CreateInfo renderInfo  =   {};
    renderInfo._width       =   256;
    renderInfo._height      =   256;
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
    if (scene == nullptr)
    {
        std::cerr << "renderer->scene() == nullptr" << std::endl;
        renderer    =   nullptr;
        return  1;
    }
    scene->setClearColor(float4(0,0,0,1));

    /// 2. 相机:透视,正前方观察原点
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0f,0.0f,10.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->setNear(0.1f);
        cam->setFar(100.0f);
        cam->updatePerspective();
    }

    /// 3. 背景:红色立方体(opaque PBR)——折射画面中的"景物"
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(buildBox(ctx));
        node->setLocalScaling(real3(2.0,2.0,2.0));
        node->setLocalTranslation(real3(0.0,0.0,-3.0));
        auto*   mat =   new FEMaterialPBR(ctx);
        mat->data()._value._diffuse   =   float4(1.0f,0.15f,0.15f,1.0f);
        mat->data()._value._metallic  =   0.0f;
        mat->data()._value._roughness =   0.8f;
        mat->data().update();
        node->setMaterial(mat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }

    /// 4. 前景:白色球体(transmission=1,磨砂玻璃 + 体积吸收)
    FEMaterialPBR*  glassMat    =   nullptr;
    {
        Node    node    =   new FENode(ctx);
        node->setMesh(buildSphere(ctx,1.2f));
        node->setLocalTranslation(real3(0.0,0.0,1.0));
        glassMat    =   new FEMaterialPBR(ctx);
        glassMat->data()._value._diffuse      =   float4(1.0f,1.0f,1.0f,1.0f);
        glassMat->data()._value._metallic     =   0.0f;
        /// 磨砂玻璃(roughness>0):shader 按 roughness*maxLod 采样 sceneTex
        /// mip 链,覆盖对标 1 的模糊折射路径(Vulkan 满链;WebGPU 单 mip)
        glassMat->data()._value._roughness    =   0.5f;
        glassMat->data().update();
        glassMat->setTransmission(1.0f);
        /// 对标 2(KHR_materials_volume):厚度 2.0(球径 2.4)产生折射偏移,
        /// 吸收色 (1,0.35,0.35) + 吸收距离 1.5 → Beer-Lambert 红玻璃效果
        glassMat->setVolume(2.0f,float3(1.0f,0.35f,0.35f),1.5f);
        glassMat->setAlphaBlend(true);
        node->setMaterial(glassMat);
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }

    /// 5. 渲染并回读(4 帧:双趟 + mip 生成 + 材质重绑均已稳定)
    check(renderer->render(4),"render(transmission) failed");
    check(renderer->width()  ==  256,"width != 256");
    check(renderer->height() ==  256,"height != 256");
    check(renderer->pixels().size() == 256 * 256 * 4,"pixels size != 256*256*4");

    /// 6. 保存 PNG 供人工确认
    const String    pngFile     =   path + "/verify_transmission.png";
    std::filesystem::remove(pngFile);
    check(renderer->save(pngFile.c_str()),"save verify_transmission.png failed");
    check(std::filesystem::exists(pngFile),"saved png does not exist");
    check(std::filesystem::file_size(pngFile) > 0,"saved png is empty");

    /// 7. 引擎自检点
    const bool      bTrans      =   (glassMat != nullptr) && glassMat->isTransmission();
    check(bTrans,"glass material isTransmission() != true");
    check(glassMat->transmission() == 1.0f,"glass transmission factor != 1.0");

    auto    cached  =   ctx.device().queryCache(FEConstUuid::SceneTexId);
    const bool  cacheOk =   (cached != nullptr);
    check(cacheOk,"device cache SceneTexId missing");

    bool        sameTex =   false;
    uint        mipCnt  =   0;
    bool        mipOk   =   false;
    if (cacheOk)
    {
        sameTex =   (glassMat->sceneTexture() != nullptr)
                    && (glassMat->sceneTexture().get() == cached.get());
        check(sameTex,"glass sceneTexture() != cached scene texture");
        if (sameTex)
        {
            mipCnt  =   glassMat->sceneTexture()->mips();
            mipOk   =   (rendererId == RS_VULKAN) ? (mipCnt > 1) : (mipCnt == 1);
            check(mipOk,"scene texture mip count unexpected");
        }
    }

    const float2    vol     =   glassMat->volume();
    const bool      volOk   =   (fabsf(vol.x - 2.0f) < 1e-5f)
                                && (fabsf(vol.y - 1.5f) < 1e-5f);
    check(volOk,"volume params != (2.0, 1.5)");

    /// 8. 像素级红色折射校验
    const bool  centerPass  =   centerIsRed(renderer->pixels(),256,256);
    check(centerPass,"center is not red (refraction failed)");

    /// 9. resize 回归:sceneTex 重建后 cache 同 ID 覆盖,材质重绑到新纹理
    bool        resizeTexChanged    =   false;
    bool        resizeRebindOk      =   false;
    bool        resizePixelOk       =   false;
    {
        auto    oldTex =   cached;
        check(renderer->resize(200,150),"resize(200,150) failed");
        check(renderer->render(4),"render(resize) failed");
        check(renderer->width()  ==  200,"width != 200 after resize");
        check(renderer->height() ==  150,"height != 150 after resize");
        check(renderer->pixels().size() == 200 * 150 * 4,"pixels size != 200*150*4 after resize");

        auto    cached2 =   ctx.device().queryCache(FEConstUuid::SceneTexId);
        check(cached2 != nullptr,"device cache SceneTexId missing after resize");
        if (cached2 != nullptr && oldTex != nullptr)
        {
            resizeTexChanged    =   (cached2.get() != oldTex.get());
            check(resizeTexChanged,"resize did not recreate scene texture");
            resizeRebindOk      =   (glassMat->sceneTexture() != nullptr)
                                    && (glassMat->sceneTexture().get() == cached2.get());
            check(resizeRebindOk,"material did not rebind scene texture after resize");
        }
        resizePixelOk
            =   centerIsRed(renderer->pixels(),200,150);
        check(resizePixelOk,"center is not red after resize");
    }
    /// 关键:resize 前的旧场景纹理已被 device cache 同 ID 覆盖,此刻仅由本地
    /// cached 持有;必须在销毁 renderer(内部销毁 device)之前释放,
    /// 否则纹理析构会落在 vkDestroyDevice 之后,触发验证错误与进程崩溃
    cached  =   nullptr;

    /// 10. 写结果文件
    const String    txtFile =   path + "/verify_transmission.txt";
    {
        if (auto* fp = fopen(txtFile.c_str(),"w"))
        {
            const bool  pass    =   (g_failed == 0);
            fprintf(fp,"renderer=%s\n",(rendererId == RS_VULKAN) ? "vulkan" : "webgpu");
            fprintf(fp,"cacheOk=%d isTransmission=%d sameTex=%d mips=%u mipOk=%d volOk=%d\n",
                    cacheOk ? 1 : 0,bTrans ? 1 : 0,sameTex ? 1 : 0,
                    mipCnt,mipOk ? 1 : 0,volOk ? 1 : 0);
            fprintf(fp,"centerRed=%d\n",centerPass ? 1 : 0);
            fprintf(fp,"resize: texChanged=%d rebindOk=%d centerRed=%d\n",
                    resizeTexChanged ? 1 : 0,resizeRebindOk ? 1 : 0,resizePixelOk ? 1 : 0);
            fprintf(fp,"png=%s\n",pngFile.c_str());
            fprintf(fp,"passed=%d failed=%d => %s\n",
                    g_passed,g_failed,pass ? "PASS" : "FAIL");
            fclose(fp);
        }
    }

    /// 11. 正常销毁(不再使用 exit(),资源释放路径可被验证层检查)
    renderer    =   nullptr;
    scene       =   nullptr;

    std::cout << "=== " << g_passed << " passed, " << g_failed << " failed ===" << std::endl;
    return  (g_failed == 0) ? 0 : 2;
}

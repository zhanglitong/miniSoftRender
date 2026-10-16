/// test.sdf.text.simsun
/// 绘制示例:使用"系统宋体"(C:\Windows\Fonts\simsun.ttc)在屏幕上绘制多行文本。
///
/// 与 test.sdf.text 的区别:
///   - 字体来自系统字库(非内置素材),并打印实际加载的字族名以确认是 SimSun
///   - 中文码点不连续,无法用连续区间烘焙:改用 FEFontCreateInfo::_characters
///     只烘焙本示例所用到的那些码点
///   - 多行文本共用同一份 SDF 字体,按不同缩放绘制(距离场可无级缩放)
///
/// 用法: test.sdf.text.simsun.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <string>
#include    <cstdint>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/font/FEFont.hpp"
#include    "../../../inc/material/FEMaterialText.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 在系统字体目录中返回首个存在的字体文件(按优先级)
    String  findSystemFont(const char* const* names,uint count)
    {
        char    winDir[MAX_PATH]    =   {0};
        if (GetWindowsDirectoryA(winDir,MAX_PATH) == 0)
            return  {};
        const   String  dir =   String(winDir) + "/Fonts/";
        for (uint i = 0; i < count; ++i)
        {
            const   String  p   =   dir + names[i];
            if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES)
                return  p;
        }
        return  {};
    }

    /// 一行文本的绘制参数
    struct  Line
    {
        const   char*   _text;
        float           _y;
        /// 显示字号(屏幕像素),内部换算为相对烘焙尺寸的缩放
        float           _size;
        uint8_t         _r;
        uint8_t         _g;
        uint8_t         _b;
    };
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const   char*   arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0 ||
            _stricmp(arg,"/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
    }

    FEContext&  ctx =   FEContext::instance();
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

    /// 2b. 配置相机:原点观察 + 正交投影。
    ///     注意:场景每帧会调用 camera->update() 重算投影(居中正交),
    ///     因此屏幕空间文本通过节点变换把"像素坐标"映射到居中正交空间。
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0f,0.0f,0.0f));
        cam->setTarget(real3(0.0f,0.0f,-1.0f));
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->setNear(-1.0f);            /// 允许 z=0 的屏幕叠加层可见
        cam->updateOrtho();             /// 居中正交投影(每帧保持)
    }
    /// 3. 定位系统宋体(宋体优先,缺失时回退到其它中文系统字体)
    const   char*   candidates[]    =   { "simsun.ttc", "simhei.ttf", "msyh.ttc" };
    const   String  fontFile        =   findSystemFont(candidates,3);
    if (fontFile.empty())
    {
        std::cerr << "system Chinese font not found (simsun.ttc/simhei.ttf/msyh.ttc)" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "system font: " << fontFile << std::endl;

    /// 4. 待绘制的文本(宋体 + 英文/数字混排),_size 为屏幕显示字号。
    ///    所有字号统一走 SDF(距离场无级缩放),靠 5x5 超采样 + gamma 强化
    ///    兼顾大字号锐利与小字号可读性
    const   Line    lines[] =
    {
        { "宋体 SimSun 距离场文本渲染",     40.0f,  40.0f, 255, 255, 255 },
        { "SDF Text Rendering 自适应抗锯齿",130.0f,  30.0f, 120, 220, 255 },
        { "字体文件 C:\\Windows\\Fonts\\simsun.ttc", 210.0f,  14.0f, 255, 190, 90 },
    };
    const   uint    lineCount   =   uint(sizeof(lines) / sizeof(lines[0]));

    /// 5. 只烘焙本示例用到的码点(中文码点不连续,不能用连续区间)
    String  charset;
    for (uint i = 0; i < lineCount; ++i)
        charset    +=  lines[i]._text;
    /// 矢量轮廓线演示文本(也需加入字符集,以便度量/步进可用)
    const   char*   outlineText =   "矢量轮廓字体 Outline Font";
    charset +=  outlineText;

    /// 统一烘焙一份高分辨率距离场,所有字号按"显示字号/烘焙尺寸"缩放采样。
    /// 烘焙 64px:宋体细横画宽约为 em 的 1/30,64px 时约 2.1 个纹素,
    /// 是"细笔画在距离场中至少占 2 纹素"的下限;spread 取 em 的 1/8。
    FEFontCreateInfo    ci;
    ci._pixelSize       =   64;
    ci._spread          =   8;
    ci._characters      =   charset;
    ci._atlasWidth      =   512;

    Font    font    =   new FEFont(ctx);
    if (!font->loadFromFile(fontFile,ci))
    {
        std::cerr << "font load failed: " << fontFile << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "family=" << font->familyName()
              << " glyphs=" << font->glyphCount()
              << " atlas=" << font->atlasWidth() << "x" << font->atlasHeight()
              << " spread=" << font->sdfSpread() << std::endl;

    /// 6. 文本材质(SDF 字体 + alpha 混合屏幕叠加)
    FEMaterialText* pMat    =   new FEMaterialText(ctx);
    pMat->setFont(font);
    if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
    {
        std::cerr << "pipeline text/TRIANGLES not found" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 矢量轮廓线材质:复用 v3c4(顶点位置 + 颜色,支持 PRI_LINES)
    FEMaterialV3C4* pMatOutline    =   new FEMaterialV3C4(ctx);
    if (pMatOutline->pipeline(PRI_LINES) == nullptr)
    {
        std::cerr << "pipeline v3c4/LINES not found" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 7. 逐行生成文本网格并分发(多行共用同一 SDF 字体与材质)
    bool    ok  =   (font->glyphCount() > 0);
    for (uint i = 0; ok && i < lineCount; ++i)
    {
        const   float   scale   =   lines[i]._size / (float)ci._pixelSize;
        Mesh    mesh    =   font->createTextMesh(lines[i]._text,
                                                 float2(40.0f,lines[i]._y),
                                                 scale);
        if (mesh == nullptr)
        {
            std::cerr << "createTextMesh failed for line " << i << std::endl;
            ok  =   false;
            break;
        }
        std::cout << "  line " << i
                  << " size=" << lines[i]._size
                  << " vertices=" << mesh->get(FEAttribute(IS_VERTEX_POS))->count()
                  << " width=" << font->measureWidth(lines[i]._text,scale)
                  << std::endl;

        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        node->setMaterial(pMat);
        node->setColor(Rgba8(lines[i]._r,lines[i]._g,lines[i]._b,255));
        node->makeDirty();
        node->update();
        scene->dispatchNodesToSystem({node});
    }

    /// 8. 矢量轮廓线测试用例(复用 v3c4 + PRI_LINES):
    ///    v3c4 走世界空间相机,视口约为 ±6.4x±4.85 世界单位(见 FEScene 默认相机),
    ///    因此网格坐标不能直接用像素/字体单位,统一按节点缩放折算到世界单位。
    ///    用例 A:世界空间原点处轮廓(绿色),节点缩放把字体单位折算成世界单位
    ///    用例 B:世界空间下方轮廓(青色),含 y 翻转(网格 y 向下,相机 y 向上)
    if (ok)
    {
        const   float   outlineSize =   32.0f;
        const   float   outlineScale=   outlineSize / (float)ci._pixelSize;
        /// 字体单位 -> 世界单位:轮廓网格宽约几百字体单位,乘 0.006 后
        /// 约 2~3 个世界单位宽,落在 ±6.4 视口内
        const   float   worldScale  =   0.006f;

        /// ---- 用例 A:世界空间轮廓(绿色),位于原点左上 ----
        Mesh    screenMesh  =   font->createTextOutlineMesh(outlineText,
                                                             float2(0.0f,0.0f),
                                                             outlineScale,12,
                                                             Rgba8(80,255,120,255));
        if (screenMesh == nullptr)
        {
            std::cerr << "createTextOutlineMesh(screen) failed" << std::endl;
            ok  =   false;
        }
        else
        {
            std::cout << "  outline[caseA] vertices="
                      << screenMesh->get(FEAttribute(IS_VERTEX_POS))->count()
                      << " pris=" << screenMesh->primitives().size()
                      << " aabb=[" << screenMesh->aabb().getMinimum().x << ","
                      << screenMesh->aabb().getMinimum().y << " "
                      << screenMesh->aabb().getMaximum().x << ","
                      << screenMesh->aabb().getMaximum().y << "]";
            Node    node    =   new FENode(ctx);
            node->setMesh(screenMesh);
            node->setMaterial(pMatOutline);
            /// 折算到世界单位并移到左上区域,不做旋转(便于观察验证)
            node->setLocalScaling(real3(worldScale,worldScale,1.0f));
            node->setLocalTranslation(real3(-2.4f,1.6f,0.0f));
            node->makeDirty();
            node->update();
            DispatchResult dr;
            scene->dispatchNodesToSystem({node},&dr);
            std::cout << " factories=" << dr.rFactorys.size() << std::endl;
            for (auto& f : dr.rFactorys)
            {
                f->setResident(true);
                f->setGPUCull(false);
            }
        }
    }

    if (ok)
    {
        const   float   outlineSize =   40.0f;
        const   float   outlineScale=   outlineSize / (float)ci._pixelSize;

        /// ---- 用例 B:三维空间轮廓(青色) ----
        /// 放在世界空间中下区域,y 翻转对齐相机上方向,不做旋转(便于观察验证)
        Mesh    spaceMesh   =   font->createTextOutlineMesh(outlineText,
                                                             float2(0.0f,0.0f),
                                                             outlineScale,12,
                                                             Rgba8(100,200,255,255));
        if (spaceMesh == nullptr)
        {
            std::cerr << "createTextOutlineMesh(3d) failed" << std::endl;
            ok  =   false;
        }
        else
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(spaceMesh);
            node->setMaterial(pMatOutline);
            /// y 翻转:网格顶点 y 向下,正交相机 y 向上;下移到中下区域
            node->setLocalScaling(real3(0.006f,-0.006f,1.0f));
            node->setLocalTranslation(real3(-2.4f,-1.8f,0.0f));
            node->makeDirty();
            node->update();
            DispatchResult dr3d;
            scene->dispatchNodesToSystem({node},&dr3d);
            for (auto& f : dr3d.rFactorys)
            {
                f->setResident(true);
                f->setGPUCull(false);
            }
            std::cout << "  outline[caseB] vertices="
                      << spaceMesh->get(FEAttribute(IS_VERTEX_POS))->count() << std::endl;
        }
    }

    if (!ok)
    {
        std::cerr << "sdf simsun validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    std::cout << "sdf simsun ok, entering render loop..." << std::endl;

    /// 8. 进入渲染循环显示系统宋体文本
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

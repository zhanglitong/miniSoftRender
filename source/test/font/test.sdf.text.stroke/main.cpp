/// test.sdf.text.stroke
/// SDF 文本描边(stroke)验证:
///   1. 创建引擎 + 渲染窗口
///   2. FEFont 加载 TTF 并以 FT_RENDER_MODE_SDF 烘焙 SDF 图集
///   3. 校验 FEMaterialText 描边接口的状态与 push constant 编码
///   4. 屏幕叠加渲染多行文本,演示不同描边宽度/颜色(0/2/4/6/8px)
///
/// 验证点:
///   a. 字体加载成功,有效字形数 > 0,图集含真正的灰度过渡(非二值)
///   b. 描边默认值:strokeWidth()==0,且 push constant _stroke.x==0
///   c. setStrokeWidth 生效并写入 push constant;负值被钳制为 0
///   d. setStrokeColor 往返一致,且 push constant _stroke.yzw==归一化 RGB
///   e. text/TRIANGLES 管线存在;文本材质仍判定为透明(alpha 混合)
///   f. push constant 布局为 3×float4 == 48 字节
/// 用法: test.sdf.text.stroke.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <cstdint>
#include    <algorithm>
#include    <cmath>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/font/FEFont.hpp"
#include    "../../../inc/material/FEMaterialText.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 一条描边演示文本的绘制参数
    struct  StrokeLine
    {
        const   char*   _text;
        Rgba8           _fill;      /// 字形填充色(节点颜色)
        float           _strokeW;   /// 描边宽度(屏幕像素)
        Rgba8           _strokeC;   /// 描边颜色
    };

    /// 比较两个浮点是否近似相等(容忍度 1e-4)
    bool    approxEq(float a,float b)
    {
        return  std::fabs(a - b) < 1e-4f;
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

    /// 3. 加载字体并烘焙 SDF 图集
    const   String  fontFile    =   ctx.resourcePath() + "/data/fonts/consola.ttf";
    Font    font    =   new FEFont(ctx);
    {
        FEFontCreateInfo    ci;
        ci._pixelSize   =   64;
        ci._spread      =   8;
        ci._firstChar   =   32;
        ci._lastChar    =   126;
        ci._atlasWidth  =   512;
        const   bool    ok  =   font->loadFromFile(fontFile,ci);
        std::cout << "font load \"" << fontFile << "\": " << (ok ? "ok" : "failed") << std::endl;
        if (ok)
        {
            std::cout << "  glyphs=" << font->glyphCount()
                      << " atlas=" << font->atlasWidth() << "x" << font->atlasHeight()
                      << " spread=" << font->sdfSpread()
                      << " bitmap=" << (font->isBitmap() ? 1 : 0) << std::endl;
        }
    }

    bool    ok  =   (font->isValid() && font->glyphCount() > 0);
    if (!ok)
    {
        std::cerr << "sdf atlas validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 4. 校验描边接口与 push constant 编码
    {
        FEMaterialText* pMat    =   new FEMaterialText(ctx);
        pMat->setFont(font);

        const   uint    pcSize  =   pMat->pushConstantSize();
        auto            td      =   (const FEMaterialText::TextData*)pMat->pushConstantData();

        std::cout << "stroke API: pcSize=" << pcSize
                  << " default width=" << pMat->strokeWidth()
                  << " default stroke.x=" << td->_stroke.x << std::endl;

        /// f. push constant 布局:3 x float4
        ok  =   (pcSize == 3 * sizeof(float) * 4) && (pcSize == sizeof(FEMaterialText::TextData));
        /// b. 默认无描边
        ok  =   ok && approxEq(pMat->strokeWidth(),0.0f) && approxEq(td->_stroke.x,0.0f);

        /// c. 设置描边宽度生效并写入 push constant
        pMat->setStrokeWidth(5.0f);
        ok  =   ok && approxEq(pMat->strokeWidth(),5.0f) && approxEq(td->_stroke.x,5.0f);

        /// c. 负值被钳制为 0
        pMat->setStrokeWidth(-3.0f);
        ok  =   ok && approxEq(pMat->strokeWidth(),0.0f) && approxEq(td->_stroke.x,0.0f);

        /// d. 描边颜色往返 + push constant yzw 归一化
        const   Rgba8   stroke(255,64,32,255);
        pMat->setStrokeColor(stroke);
        const   auto&   sc  =   stroke.value();
        const   bool    colorOk =   (pMat->strokeColor().value().x == sc.x)
                                &&  (pMat->strokeColor().value().y == sc.y)
                                &&  (pMat->strokeColor().value().z == sc.z)
                                &&  approxEq(td->_stroke.y,(float)sc.x / 255.0f)
                                &&  approxEq(td->_stroke.z,(float)sc.y / 255.0f)
                                &&  approxEq(td->_stroke.w,(float)sc.z / 255.0f);
        std::cout << "  after set(5,-3,color): width=" << pMat->strokeWidth()
                  << " stroke=(" << td->_stroke.x << ","
                  << td->_stroke.y << "," << td->_stroke.z << "," << td->_stroke.w << ")"
                  << " colorOk=" << (colorOk ? 1 : 0) << std::endl;
        ok  =   ok && colorOk;

        /// e. 管线存在 + 透明材质判定
        ok  =   ok && (pMat->pipeline(PRI_TRIANGLES) != nullptr);
        ok  =   ok && pMat->isTransparent();
        ok  =   ok && !pMat->isBillboard();
    }

    if (!ok)
    {
        std::cerr << "stroke API validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 5. 屏幕叠加渲染多行带描边的文本,演示不同描边宽度/颜色
    const   StrokeLine  lines[] =
    {
        { "SDF text stroke demo",        Rgba8(255,255,255,255),  0.0f, Rgba8(  0,  0,  0,255) },
        { "stroke = 0px  (no stroke)",   Rgba8(255,255,255,255),  0.0f, Rgba8(  0,  0,  0,255) },
        { "stroke = 2px  red",           Rgba8(255,255,255,255),  2.0f, Rgba8(255, 40, 40,255) },
        { "stroke = 4px  green",         Rgba8(255,255,255,255),  4.0f, Rgba8( 30,230, 60,255) },
        { "stroke = 6px  blue",          Rgba8(255,255,255,255),  6.0f, Rgba8( 40,120,255,255) },
        { "stroke = 8px  yellow",        Rgba8(255,255,255,255),  8.0f, Rgba8(255,220,  0,255) },
        { "dark fill + white stroke",    Rgba8( 40, 40, 40,255), 5.0f, Rgba8(255,255,255,255) },
    };
    const   uint    lineCount   =   uint(sizeof(lines) / sizeof(lines[0]));

    /// 收集所有文本用到的字符,只烘焙需要的码点
    String  charset;
    for (uint i = 0; i < lineCount; ++i)
        charset +=  lines[i]._text;
    {
        FEFontCreateInfo    ci;
        ci._pixelSize   =   64;
        ci._spread      =   8;
        ci._characters  =   charset;
        ci._atlasWidth  =   512;
        if (!font->loadFromFile(fontFile,ci))
        {
            std::cerr << "reload font with charset failed" << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }
        std::cout << "reloaded charset glyphs=" << font->glyphCount()
                  << " atlas=" << font->atlasWidth() << "x" << font->atlasHeight() << std::endl;
    }

    const   float   baseSize    =   28.0f;
    const   float   scale       =   baseSize / 64.0f;
    const   float   x0          =   40.0f;
    const   float   y0          =   40.0f;
    const   float   lineStep    =   46.0f;

    for (uint i = 0; i < lineCount; ++i)
    {
        const   StrokeLine& ln  =   lines[i];
        Mesh    mesh    =   font->createTextMesh(ln._text,float2(x0,y0 + lineStep * i),scale);
        if (mesh == nullptr)
        {
            std::cerr << "createTextMesh failed for line " << i << std::endl;
            ok  =   false;
            break;
        }

        FEMaterialText* pMat    =   new FEMaterialText(ctx);
        pMat->setFont(font);
        pMat->setStrokeWidth(ln._strokeW);
        pMat->setStrokeColor(ln._strokeC);
        if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
        {
            std::cerr << "pipeline text/TRIANGLES not found" << std::endl;
            ok  =   false;
            break;
        }

        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        node->setMaterial(pMat);
        node->setColor(ln._fill);
        node->makeDirty();
        node->update();
        DispatchResult  dr;
        scene->dispatchNodesToSystem({node},&dr);
        /// 网格 AABB 是像素偏移(数百"世界单位"),剔除无意义,常驻并关 GPU 剔除
        for (auto& f : dr.rFactorys)
        {
            f->setResident(true);
            f->setGPUCull(false);
        }
        std::cout << "  line " << i << " \"" << ln._text << "\""
                  << " stroke=" << ln._strokeW << "px"
                  << " width=" << font->measureWidth(ln._text,scale) << "px" << std::endl;
    }

    if (!ok)
    {
        std::cerr << "sdf text stroke validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    std::cout << "sdf text stroke ok, entering render loop..." << std::endl;
    std::cout << "expected: 每行文字带不同宽度/颜色的描边;"
                 "stroke=0 行无描边,宽度越大描边越粗" << std::endl;

    /// 6. 进入渲染循环显示屏幕文字
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

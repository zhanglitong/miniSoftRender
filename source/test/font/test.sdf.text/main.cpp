/// test.sdf.text
/// 距离场(SDF)文本绘制测试:
///   1. 创建引擎 + 渲染窗口
///   2. FEFont 用内置 FreeType 加载 TTF,并以 FT_RENDER_MODE_SDF 烘焙 SDF 图集
///   3. 校验 SDF 图集编码(128 为轮廓边界,>128 在字形内部,<128 在外部)
///   4. FEFont::createTextMesh 生成屏幕空间文本网格(像素坐标 pos + 图集 uv)
///   5. 经 FEMaterialText(text/* 管线,alpha 混合,关闭深度)分发渲染
/// 验证点:
///   a. 字体加载成功,有效字形数 > 0,图集含真正的灰度过渡(非二值)
///   b. 'H' 字形位图:内部存在 >128 的高值,外部(角落 padding)存在 <128 的低值
///   c. 文本网格四边形数 == 有效字形数 * 2(每字形 6 顶点)
///   d. text/TRIANGLES 管线存在
/// 用法: test.sdf.text.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <cstdint>
#include    <algorithm>

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
    /// 采样 SDF 图集中 (u,v) 处的灰度值(R 通道)
    uint8_t sampleAtlas(const FEFont& font,float u,float v)
    {
        uint    w   =   font.atlasWidth();
        uint    h   =   font.atlasHeight();
        const   auto&   px  =   font.atlasPixels();
        if (w == 0 || h == 0 || px.empty())
            return  0;
        uint    x   =   std::min<uint>(uint(u * (float)w),w - 1);
        uint    y   =   std::min<uint>(uint(v * (float)h),h - 1);
        return  px[((size_t)y * w + x) * 4 + 0];
    }
    /// 统计单个字形位图范围内的最小/最大灰度
    void    glyphMinMax(const FEFont& font,const FEFontGlyph& g,uint8_t& minV,uint8_t& maxV)
    {
        minV    =   255;
        maxV    =   0;
        uint    w   =   font.atlasWidth();
        uint    h   =   font.atlasHeight();
        const   auto&   px  =   font.atlasPixels();
        uint    x0  =   std::min<uint>(uint(g._uv0.x * (float)w),w - 1);
        uint    y0  =   std::min<uint>(uint(g._uv0.y * (float)h),h - 1);
        uint    x1  =   std::min<uint>(uint(g._uv1.x * (float)w),w);
        uint    y1  =   std::min<uint>(uint(g._uv1.y * (float)h),h);
        for (uint y = y0; y < y1; ++y)
        {
            for (uint x = x0; x < x1; ++x)
            {
                uint8_t v   =   px[((size_t)y * w + x) * 4 + 0];
                minV    =   std::min(minV,v);
                maxV    =   std::max(maxV,v);
            }
        }
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
        ci._pixelSize   =   48;
        ci._spread      =   8;
        ci._firstChar   =   32;
        ci._lastChar    =   126;
        ci._atlasWidth  =   256;
        const   bool    ok  =   font->loadFromFile(fontFile,ci);
        std::cout << "font load \"" << fontFile << "\": " << (ok ? "ok" : "failed") << std::endl;
        if (ok)
        {
            std::cout << "  glyphs=" << font->glyphCount()
                      << " atlas=" << font->atlasWidth() << "x" << font->atlasHeight()
                      << " lineHeight=" << font->lineHeight()
                      << " ascent=" << font->ascent() << std::endl;
        }
    }

    /// 4. 校验 SDF 图集编码
    bool    sdfOk   =   false;
    {
        if (font->isValid() && font->glyphCount() > 0)
        {
            /// a. 图集存在真正的灰度过渡(既非全黑也非全白),即 SDF 有效
            uint    edgeBand    =   0;      /// 接近轮廓边界(128)的像素数
            uint    midGray     =   0;      /// 中间灰度像素数
            const   auto&   px  =   font->atlasPixels();
            for (size_t i = 0; i < px.size(); i += 4)
            {
                uint8_t v   =   px[i];
                if (v > 8 && v < 248)
                    ++midGray;
                if (v >= 110 && v <= 146)
                    ++edgeBand;
            }

            /// b. 'H'(0x48)字形:内部 > max,外部(角落) < min
            const   FEFontGlyph&    gH  =   font->glyph('H');
            uint8_t minV    =   255;
            uint8_t maxV    =   0;
            uint8_t centerV =   0;
            uint8_t cornerV =   0;
            if (gH._valid)
            {
                glyphMinMax(*font,gH,minV,maxV);
                /// 位图中心:应位于字形内部(>128)
                centerV =   sampleAtlas(*font,(gH._uv0.x + gH._uv1.x) * 0.5f,
                                               (gH._uv0.y + gH._uv1.y) * 0.5f);
                /// 位图左上角:位于 spread padding 内,确实在字形外部(<128)
                cornerV =   sampleAtlas(*font,gH._uv0.x,gH._uv0.y);
            }

            std::cout << "sdf: midGray=" << midGray
                      << " edgeBand=" << edgeBand
                      << " | 'H' min=" << uint(minV)
                      << " max=" << uint(maxV)
                      << " center=" << uint(centerV)
                      << " corner=" << uint(cornerV) << std::endl;

            sdfOk   =   (midGray > 100)         /// 存在灰度过渡
                    &&  (edgeBand > 0)          /// 存在轮廓边界带
                    &&  gH._valid
                    &&  (maxV > 150)            /// 字形内部:编码 > 128
                    &&  (minV < 105)            /// 字形外部:编码 < 128
                    &&  (centerV > 128)         /// 中心在内部
                    &&  (cornerV < 128);        /// 角落在外部
        }
    }
    if (!sdfOk)
    {
        std::cerr << "sdf atlas validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 5. 生成屏幕空间文本网格并校验顶点数
    bool    passed  =   false;
    {
        const   String  text    =   "Hello, SDF!";
        /// 有效字形数(ASCII,每字符一个字形)
        uint    validGlyphs =   0;
        for (char c : text)
        {
            const   FEFontGlyph&    g   =   font->glyph((uint32)(uint8_t)c);
            if (g._valid)
                ++validGlyphs;
        }

        Mesh    mesh    =   font->createTextMesh(text,float2(40.0f,40.0f),1.0f);

        uint    posCount    =   0;
        uint    uvCount     =   0;
        if (mesh != nullptr)
        {
            if (auto* p = mesh->get(FEAttribute(IS_VERTEX_POS)))
                posCount    =   p->count();
            if (auto* uv = mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0)))
                uvCount     =   uv->count();
        }

        std::cout << "text mesh: validGlyphs=" << validGlyphs
                  << " buffers=" << (mesh ? mesh->buffers().size() : 0)
                  << " pos=" << posCount
                  << " uv=" << uvCount
                  << " width=" << font->measureWidth(text) << std::endl;

        passed  =   (mesh != nullptr)
                &&  (validGlyphs > 0)
                &&  (posCount == validGlyphs * 6)   /// 每字形 2 个三角形(6 顶点)
                &&  (uvCount  == posCount)
                &&  (mesh->buffers().size() >= 2);

        if (passed)
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            /// 文本材质:SDF 字体 + alpha 混合屏幕叠加
            FEMaterialText* pMat    =   new FEMaterialText(ctx);
            pMat->setFont(font);
            if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
            {
                std::cerr << "pipeline text/TRIANGLES not found" << std::endl;
                passed  =   false;
            }
            node->setMaterial(pMat);
            /// 白色文字,便于观察 SDF 抗锯齿效果
            node->setColor(Rgba8(255,255,255,255));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
        }
    }

    if (!passed)
    {
        std::cerr << "sdf text validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "sdf text ok, entering render loop..." << std::endl;

    /// 6. 进入渲染循环显示屏幕文字
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}

/// test.sdf.font
/// 距离场字体(FEFont)单元测试 —— 纯 CPU,不需要窗口与渲染设备
/// (通过 FEFontCreateInfo::_uploadTexture=false 跳过 GPU 上传)。
///
/// 覆盖的"单元":
///   1. 加载失败路径(不存在文件 / 空数据 / 非法字节流)
///   2. SDF 图集烘焙:字形数、图集尺寸(高度 2 的幂)、行高/基线度量
///   3. SDF 编码约定:128=轮廓边界,>128 内部,<128 外部;四通道一致
///   4. 字形度量:uv/size/offset 自洽、advance>0、空白字符无位图但有步进
///   5. 货架打包:所有字形图集矩形互不重叠且落在图集内
///   6. 文本宽度测量:空串、单字、累加、缩放、换行不计宽、空格占位
///   7. 屏幕空间网格几何:6 顶点矩形、位置/uv 精确对应、换行、缩放、空串
///   8. UTF-8 解码:两字节序列解码为单个码点(黑盒验证)
/// 用法: test.sdf.font.exe

#include    <iostream>
#include    <cmath>
#include    <string>
#include    <vector>
#include    <algorithm>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/font/FEFont.hpp"

using   namespace   FE;

namespace
{
    int     g_failed    =   0;
    int     g_checked   =   0;

    #define CHECK(cond,msg)                                                     \
        do {                                                                    \
            ++g_checked;                                                        \
            if (!(cond)) {                                                      \
                ++g_failed;                                                     \
                std::cerr << "[FAIL] " << (msg) << "  (" << __LINE__ << ")"     \
                          << std::endl;                                         \
            }                                                                   \
        } while (0)

    /// 浮点近似比较(网格几何按相同算式生成,允许极小误差)
    bool    feq(float a,float b)
    {
        return  std::fabs(a - b) <= 1e-4f;
    }
    bool    veq2(const float2& a,const float2& b)
    {
        return  feq(a.x,b.x) && feq(a.y,b.y);
    }
    bool    veq3(const float3& a,const float3& b)
    {
        return  feq(a.x,b.x) && feq(a.y,b.y) && feq(a.z,b.z);
    }

    /// 可执行文件所在目录下的测试字体
    String  fontPath()
    {
        char    buf[MAX_PATH]   =   {0};
        GetModuleFileNameA(nullptr,buf,MAX_PATH);
        String  p   =   buf;
        size_t  s   =   p.find_last_of("\\/");
        if (s != String::npos)
            p.resize(s);
        return  p + "/../data/fonts/consola.ttf";
    }

    /// 纯 CPU 烘焙一个字体的指定码点区间
    Font    makeFont(uint first,uint last,uint pixelSize = 48,uint spread = 8)
    {
        FEFontCreateInfo    ci;
        ci._pixelSize       =   pixelSize;
        ci._spread          =   spread;
        ci._firstChar       =   first;
        ci._lastChar        =   last;
        ci._atlasWidth      =   256;
        /// 单元测试不创建渲染设备
        ci._uploadTexture   =   false;

        Font    font    =   new FEFont(FEContext::instance());
        if (!font->loadFromFile(fontPath(),ci))
            return  nullptr;
        return  font;
    }

    /// 采样 SDF 图集 (u,v) 处的灰度值(R 通道)
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
    /// 单个字形位图范围内的最小/最大灰度
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

    /// 1. 加载失败路径
    void    testLoadFailure(FEContext& ctx)
    {
        FEFont  f0(ctx);
        CHECK(!f0.loadFromFile("__no_such_font__.ttf"),    "loadFromFile(不存在) 应失败");
        CHECK(!f0.isValid(),                               "未加载字体 isValid 应为 false");
        CHECK(f0.glyphCount() == 0,                        "未加载字体 glyphCount 应为 0");
        CHECK(!f0.glyph('A')._valid,                       "未加载字体 glyph 应无效");
        CHECK(f0.measureWidth("ABC") == 0.0f,              "未加载字体 measureWidth 应为 0");
        CHECK(f0.createTextMesh("A",float2(0,0)) == nullptr,"未加载字体 createTextMesh 应为 null");
        CHECK(f0.texture() == nullptr,                     "CPU-only 模式 texture 应为 null");

        FEFont  f1(ctx);
        CHECK(!f1.loadFromMemory(nullptr,0),               "loadFromMemory(nullptr) 应失败");

        FEFont  f2(ctx);
        const   char    junk[4] =   {0,1,2,3};
        CHECK(!f2.loadFromMemory(junk,sizeof(junk)),       "loadFromMemory(非法字节) 应失败");
    }

    /// 2. 烘焙基础度量
    void    testBakeBasics(const Font& font)
    {
        CHECK(font->isValid(),                             "烘焙后 isValid 应为 true");
        CHECK(font->glyphCount() > 0,                      "烘焙后 glyphCount 应 > 0");
        CHECK(font->atlasWidth() >= 256,                   "图集宽度不应小于配置值");

        const   uint    h   =   font->atlasHeight();
        CHECK(h >= 4 && (h & (h - 1)) == 0,                "图集高度应为 2 的幂");
        CHECK(font->atlasPixels().size() ==
              size_t(font->atlasWidth()) * h * 4,          "图集字节数 = 宽*高*4");

        CHECK(font->lineHeight() > 0.0f,                   "lineHeight 应 > 0");
        CHECK(font->ascent() > 0.0f,                       "ascent 应 > 0");
        CHECK(font->ascent() < font->lineHeight(),         "ascent 应小于 lineHeight");
        CHECK(font->lineHeight() > 40.0f &&
              font->lineHeight() < 80.0f,                  "48px 字号行高应在合理区间");

        /// 可打印 ASCII 中仅空格无位图(上下标仍应有步进)
        uint    invalid =   0;
        for (uint c = 32; c <= 126; ++c)
        {
            if (!font->glyph(c)._valid)
                ++invalid;
        }
        CHECK(invalid == 1,                                "可打印 ASCII 中仅 1 个无位图(空格)");
        CHECK(!font->glyph(' ')._valid,                    "空格应无位图(_valid=false)");
        CHECK(font->glyph(' ')._advance > 0.0f,            "空格应仍有步进(_advance>0)");
        CHECK(font->glyphCount() == 95 - 1,                "字形数 = 95 个码位减空格");
    }

    /// 3. SDF 编码约定
    void    testSdfEncoding(const Font& font)
    {
        /// 图集应含真正的灰度过渡(非二值),且存在轮廓边界(≈128)
        uint    midGray =   0;
        uint    edgeBand=   0;
        const   auto&   px  =   font->atlasPixels();
        for (size_t i = 0; i < px.size(); i += 4)
        {
            const   uint8_t v   =   px[i];
            if (v > 8 && v < 248)
                ++midGray;
            if (v >= 110 && v <= 146)
                ++edgeBand;
        }
        CHECK(midGray > 100,                               "图集应存在灰度过渡(SDF 非二值)");
        CHECK(edgeBand > 0,                                "图集应存在轮廓边界像素(≈128)");

        /// R/G/B/A 四通道写入同一 SDF 值
        uint    sameChannel =   0;
        for (size_t i = 0; i < px.size(); i += 4)
        {
            if (px[i] == px[i+1] && px[i] == px[i+2] && px[i] == px[i+3])
                ++sameChannel;
        }
        CHECK(sameChannel == px.size() / 4,                "RGBA 四通道应写入相同 SDF 值");

        /// 'H':内部编码 >128,外部编码 <128
        const   FEFontGlyph&    gH  =   font->glyph('H');
        CHECK(gH._valid,                                   "'H' 应有有效字形");
        if (!gH._valid)
            return;

        uint8_t minV    =   255;
        uint8_t maxV    =   0;
        glyphMinMax(*font,gH,minV,maxV);

        const   uint8_t centerV =   sampleAtlas(*font,(gH._uv0.x + gH._uv1.x) * 0.5f,
                                                      (gH._uv0.y + gH._uv1.y) * 0.5f);
        const   uint8_t cornerV =   sampleAtlas(*font,gH._uv0.x,gH._uv0.y);

        CHECK(maxV > 150,                                  "'H' 字形内部应编码 >128(实测最大)");
        CHECK(minV < 105,                                  "'H' 字形外部应编码 <128(实测最小)");
        CHECK(centerV > 128,                               "'H' 位图中心应位于字形内部");
        CHECK(cornerV < 128,                               "'H' 位图角落(spread padding)应在字形外部");
    }

    /// 4. 字形度量自洽性 + 5. 打包不重叠
    void    testGlyphMetricsAndPacking(const Font& font)
    {
        const   float   aw  =   (float)font->atlasWidth();
        const   float   ah  =   (float)font->atlasHeight();

        struct  Rect { uint x0,y0,x1,y1; };
        std::vector<Rect>   rects;

        uint    checkedGlyph=   0;
        bool    metricsOk   =   true;
        bool    boundsOk    =   true;

        for (uint c = 32; c <= 126; ++c)
        {
            const   FEFontGlyph&    g   =   font->glyph(c);
            if (!g._valid)
                continue;
            ++checkedGlyph;

            /// uv 顺序与范围
            metricsOk   =   metricsOk
                        &&  (g._uv0.x < g._uv1.x) && (g._uv0.y < g._uv1.y)
                        &&  (g._uv0.x >= 0.0f)    && (g._uv1.x <= 1.0f)
                        &&  (g._uv0.y >= 0.0f)    && (g._uv1.y <= 1.0f);
            /// 尺寸为正且与 uv 差自洽
            metricsOk   =   metricsOk
                        &&  (g._size.x > 0.0f) && (g._size.y > 0.0f)
                        &&  feq((g._uv1.x - g._uv0.x) * aw, g._size.x)
                        &&  feq((g._uv1.y - g._uv0.y) * ah, g._size.y);
            /// 步进为正
            metricsOk   =   metricsOk && (g._advance > 0.0f);

            /// 图集矩形取整后落在图集内
            const   uint    x0  =   uint(g._uv0.x * aw + 0.5f);
            const   uint    y0  =   uint(g._uv0.y * ah + 0.5f);
            const   uint    x1  =   uint(g._uv1.x * aw + 0.5f);
            const   uint    y1  =   uint(g._uv1.y * ah + 0.5f);
            boundsOk    =   boundsOk && (x1 <= font->atlasWidth()) && (y1 <= font->atlasHeight());
            rects.push_back({x0,y0,x1,y1});
        }

        CHECK(checkedGlyph > 90,                           "应校验到 >90 个有效字形");
        CHECK(metricsOk,                                   "字形 uv/size/advance 度量应自洽");
        CHECK(boundsOk,                                    "字形图集矩形应落在图集范围内");

        /// 两两不重叠(允许相邻共边)
        uint    overlaps    =   0;
        for (size_t i = 0; i < rects.size(); ++i)
        {
            for (size_t j = i + 1; j < rects.size(); ++j)
            {
                const   Rect&   a   =   rects[i];
                const   Rect&   b   =   rects[j];
                if (a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1)
                    ++overlaps;
            }
        }
        CHECK(overlaps == 0,                               "货架打包:字形图集矩形不应重叠");
    }

    /// 6. 文本宽度测量
    void    testMeasureWidth(const Font& font)
    {
        const   float   advA    =   font->glyph('A')._advance;
        const   float   advB    =   font->glyph('B')._advance;
        const   float   advC    =   font->glyph('C')._advance;

        CHECK(font->measureWidth("") == 0.0f,              "空串宽度应为 0");
        CHECK(feq(font->measureWidth("A"),advA),           "单字宽度应等于其步进");
        CHECK(feq(font->measureWidth("ABC"),advA + advB + advC), "多字宽度应为步进之和");
        CHECK(feq(font->measureWidth("ABC",2.0f),
                  font->measureWidth("ABC",1.0f) * 2.0f),   "宽度应随缩放线性变化");
        CHECK(font->measureWidth("\n") == 0.0f,            "纯换行宽度应为 0");
        CHECK(feq(font->measureWidth("A\nB"),
                  font->measureWidth("AB")),               "换行不贡献宽度");
        /// 空格有步进,故 "A B" 宽于 "AB"
        CHECK(font->measureWidth("A B") > font->measureWidth("AB"),
                                                           "空格应占用宽度");
    }

    /// 7. 屏幕空间网格几何
    void    testTextMeshGeometry(const Font& font)
    {
        const   FEFontGlyph&    gA  =   font->glyph('A');
        const   FEFontGlyph&    gB  =   font->glyph('B');
        CHECK(gA._valid && gB._valid,                      "网格几何测试依赖 'A'/'B' 字形");
        if (!gA._valid || !gB._valid)
            return;

        /// 空串 / 纯换行 -> 空网格
        CHECK(font->createTextMesh("",float2(0,0)) == nullptr,    "空串应生成 null 网格");
        CHECK(font->createTextMesh("\n",float2(0,0)) == nullptr,  "纯换行应生成 null 网格");

        const   float2  origin  =   float2(10.0f,20.0f);
        Mesh    mesh    =   font->createTextMesh("A",origin,1.0f);
        CHECK(mesh != nullptr,                             "'A' 应生成网格");
        if (mesh == nullptr)
            return;

        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        auto*   uvBuf   =   mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0));
        CHECK(posBuf != nullptr && uvBuf != nullptr,       "网格应含 pos/uv 缓冲");
        if (posBuf == nullptr || uvBuf == nullptr)
            return;
        CHECK(posBuf->count() == 6,                        "单字形应为 6 顶点");
        CHECK(uvBuf->count() == 6,                         "单字形应有 6 个 uv");

        const   float3* pos =   posBuf->buffer()->dataAs<float3>();
        const   float2* uv  =   uvBuf->buffer()->dataAs<float2>();

        const   float   x0  =   origin.x + gA._offset.x;
        const   float   y0  =   origin.y + font->ascent() + gA._offset.y;
        const   float   x1  =   x0 + gA._size.x;
        const   float   y1  =   y0 + gA._size.y;

        CHECK(veq3(pos[0],float3(x0,y1,0.0f)),             "顶点0(左下)位置");
        CHECK(veq3(pos[1],float3(x1,y1,0.0f)),             "顶点1(右下)位置");
        CHECK(veq3(pos[2],float3(x1,y0,0.0f)),             "顶点2(右上)位置");
        CHECK(veq3(pos[3],float3(x0,y1,0.0f)),             "顶点3 与顶点0 重合");
        CHECK(veq3(pos[4],float3(x1,y0,0.0f)),             "顶点4 与顶点2 重合");
        CHECK(veq3(pos[5],float3(x0,y0,0.0f)),             "顶点5(左上)位置");

        CHECK(veq2(uv[0],float2(gA._uv0.x,gA._uv1.y)),     "uv0 映射(左下)");
        CHECK(veq2(uv[1],float2(gA._uv1.x,gA._uv1.y)),     "uv1 映射(右下)");
        CHECK(veq2(uv[2],float2(gA._uv1.x,gA._uv0.y)),     "uv2 映射(右上)");
        CHECK(veq2(uv[3],float2(gA._uv0.x,gA._uv1.y)),     "uv3 与 uv0 一致");
        CHECK(veq2(uv[4],float2(gA._uv1.x,gA._uv0.y)),     "uv4 与 uv2 一致");
        CHECK(veq2(uv[5],float2(gA._uv0.x,gA._uv0.y)),     "uv5 映射(左上)");

        /// 缩放:四边形宽高应随 scale 线性变化
        Mesh    mesh2   =   font->createTextMesh("A",origin,2.0f);
        if (mesh2 != nullptr && mesh2->get(FEAttribute(IS_VERTEX_POS)) != nullptr)
        {
            const   float3* p2  =   mesh2->get(FEAttribute(IS_VERTEX_POS))->buffer()->dataAs<float3>();
            CHECK(feq(p2[1].x - p2[0].x, gA._size.x * 2.0f), "缩放 2x:四边形宽度应翻倍");
            CHECK(feq(p2[0].y - p2[2].y, gA._size.y * 2.0f), "缩放 2x:四边形高度应翻倍");
        }

        /// 换行:第二行整体下移一个行高
        Mesh    mesh3   =   font->createTextMesh("A\nB",float2(0,0),1.0f);
        CHECK(mesh3 != nullptr,                            "'A\\nB' 应生成网格");
        if (mesh3 != nullptr && mesh3->get(FEAttribute(IS_VERTEX_POS)) != nullptr)
        {
            auto*   b3  =   mesh3->get(FEAttribute(IS_VERTEX_POS));
            CHECK(b3->count() == 12,                       "两行两字形应为 12 顶点");
            const   float3* p3  =   b3->buffer()->dataAs<float3>();
            const   float   y0B =   font->ascent() + font->lineHeight() + gB._offset.y;
            const   float   y1B =   y0B + gB._size.y;
            CHECK(veq3(p3[6],float3(gB._offset.x,y1B,0.0f)),  "第二行首顶点应下移一个行高");
        }
    }

    /// 8. UTF-8 解码(黑盒):两字节序列须解码为单个码点
    void    testUtf8Decode()
    {
        /// 烘焙 Latin-1 补充区,使多字节码点(如 U+00E9 'é')真正拥有字形
        Font    latin   =   makeFont(0xC0,0xFF);
        CHECK(latin != nullptr,                            "Latin-1 区间字体应烘焙成功");
        if (latin == nullptr)
            return;

        const   FEFontGlyph&    ge  =   latin->glyph(0xE9);
        CHECK(ge._valid,                                   "U+00E9 ('é') 应烘焙出字形");

        /// UTF-8 编码 U+00E9 = 0xC3 0xA9。
        /// 用字节数组拼装:既避免 "\x" 贪婪吞掉后续字符,也避免常量截断告警
        const   unsigned char   eBytes[2]   =   {0xC3,0xA9};
        const   String  e   =   String((const char*)eBytes,2);

        CHECK(feq(latin->measureWidth(e),ge._advance),     "两字节 UTF-8 序列应解码为单码点");
        CHECK(latin->measureWidth(e) > 0.0f,               "多字节字符应产生非零步进");

        Mesh    mesh    =   latin->createTextMesh(e,float2(0,0),1.0f);
        CHECK(mesh != nullptr,                             "多字节字符应能生成网格");
        if (mesh != nullptr && mesh->get(FEAttribute(IS_VERTEX_POS)) != nullptr)
            CHECK(mesh->get(FEAttribute(IS_VERTEX_POS))->count() == 6,
                                                           "多字节字符应只生成 1 个字形(6 顶点)");

        /// 截断的多字节序列不得越界(鲁棒性)
        const   unsigned char   tBytes[2]   =   {0xE4,0xB8};
        const   String  trunc   =   String((const char*)tBytes,2);
        CHECK(latin->measureWidth(trunc) == 0.0f,          "截断多字节序列应为 0 宽且不越界");

        const   unsigned char   lBytes[1]   =   {0xC3};
        const   String  loneLead    =   String((const char*)lBytes,1);
        CHECK(loneLead.size() == 1,                        "前置字节序列长度");
        CHECK(latin->measureWidth(loneLead) == 0.0f,       "孤立前置字节应为 0 宽且不越界");
    }

    /// 9. 显式字符集烘焙(FEFontCreateInfo::_characters):中文等码点不连续场景
    void    testExplicitCharset()
    {
        FEFontCreateInfo    ci;
        ci._pixelSize       =   48;
        ci._spread          =   8;
        ci._atlasWidth      =   256;
        ci._uploadTexture   =   false;
        /// 只烘焙列出的字符
        const   String  charset =   "Hello! 0123";
        ci._characters      =   charset;

        Font    font    =   new FEFont(FEContext::instance());
        CHECK(font->loadFromFile(fontPath(),ci),           "显式字符集应烘焙成功");
        if (!font->isValid())
            return;

        /// 统计字符集中不重复的码点数
        std::vector<uint32> seen;
        for (char c : charset)
        {
            const   uint32  code    =   (uint32)(uint8_t)c;
            if (std::find(seen.begin(),seen.end(),code) == seen.end())
                seen.push_back(code);
        }

        CHECK(font->glyphCount() > 0,                      "显式字符集应有有效字形");
        CHECK(font->glyphCount() == seen.size() - 1,       "字符集模式:仅烘焙列出码点(空格除外)");

        /// 列出的字符全部有效
        bool    listedOk    =   true;
        for (char c : charset)
        {
            if (c == ' ')
                continue;
            listedOk    =   listedOk && font->glyph((uint32)(uint8_t)c)._valid;
        }
        CHECK(listedOk,                                    "字符集内的字符应全部烘焙出字形");
        /// 未列出的码点保持无效(证明不是整段区间烘焙)
        CHECK(!font->glyph('Z')._valid,                    "字符集外的码点应无效");
        CHECK(!font->glyph('z')._valid,                    "字符集外的码点应无效");
        CHECK(!font->glyph('\n')._valid,                   "换行不应被烘焙");

        CHECK(font->measureWidth(charset) > 0.0f,          "字符集文本宽度应 > 0");
        const   uint    h   =   font->atlasHeight();
        CHECK(h >= 4 && (h & (h - 1)) == 0,                "字符集模式图集高度应为 2 的幂");

        /// 非连续多字节码点:U+00C0 'À' 与 U+00CA 'Ê',夹在中间的 U+00C5 不应被烘焙
        const   unsigned char   latin2[4]   =   {0xC3,0x80, 0xC3,0x8A};
        FEFontCreateInfo    ci2 =   ci;
        ci2._characters     =   String((const char*)latin2,4);

        Font    latin   =   new FEFont(FEContext::instance());
        CHECK(latin->loadFromFile(fontPath(),ci2),         "非连续多字节字符集应烘焙成功");
        if (latin->isValid())
        {
            CHECK(latin->glyph(0xC0)._valid,               "U+00C0 'À' 应烘焙");
            CHECK(latin->glyph(0xCA)._valid,               "U+00CA 'Ê' 应烘焙");
            CHECK(!latin->glyph(0xC5)._valid,              "区间内未列出的 U+00C5 应无效");
            CHECK(latin->glyphCount() == 2,                "该字符集应仅有 2 个有效字形");
        }
    }
}

int     main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    FEContext&  ctx =   FEContext::instance();

    std::cout << "font: " << fontPath() << std::endl;

    testLoadFailure(ctx);

    Font    font    =   makeFont(32,126);
    CHECK(font != nullptr,                                 "ASCII 字体应烘焙成功");
    if (font != nullptr)
    {
        testBakeBasics(font);
        testSdfEncoding(font);
        testGlyphMetricsAndPacking(font);
        testMeasureWidth(font);
        testTextMeshGeometry(font);
    }

    testUtf8Decode();
    testExplicitCharset();

    std::cout << "checked=" << g_checked
              << " failed=" << g_failed << std::endl;
    if (g_failed == 0)
    {
        std::cout << "test.sdf.font: ALL PASS" << std::endl;
        return  0;
    }
    std::cerr << "test.sdf.font: FAILED (" << g_failed << ")" << std::endl;
    return  1;
}

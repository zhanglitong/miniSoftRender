/// test.sdf.text.fill
/// 矢量填充模式(FEFont::createTextFillMesh / createTextExtrudeMesh)纯算法测试:
/// 通过 FEFontCreateInfo::_uploadTexture=false 跳过 GPU 上传,只校验三角化网格数据;
/// 窗口绘制见独立工程 test.sdf.text.fill.visual。
///
/// 覆盖的"单元":
///   1. 未加载字体的失败路径(空串/纯换行/加载失败均返回 null)
///   2. 基础网格:非空、顶点数为 3 的倍数、pos+color0 缓冲、TRIANGLES 图元
///   3. 顶点属性:坐标有限、颜色全部等于指定颜色
///   4. 坐标范围:落在 origin..advance / baseline..ascent 内
///   5. 带孔字形: 'O' 中心为孔洞,上下左右环带被填充
///   6. 三角形绕序:所有三角形同一绕序(非退化),便于背面剔除正确
///   7. 多字形: "AB" 顶点数 = "A" + "B"(逐字形独立三角化)
///   8. 缩放:包围盒宽高随 scale 线性变化;换行:第二行下移约一个行高
///   9. 实体挤出:厚度(z 范围)、单位法线(前盖/后盖/侧壁)、顶点色、图元
///  10. UV:挤出网格附带 TEXCOORD0,供 PBR 等带贴图的管线使用
/// 用法: test.sdf.text.fill.exe

#include    <iostream>
#include    <cmath>
#include    <string>
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

    bool    feq(float a,float b)
    {
        return  std::fabs(a - b) <= 1e-4f;
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

    /// 只烘焙测试所需字符,纯 CPU 模式
    Font    makeFont()
    {
        FEFontCreateInfo    ci;
        ci._pixelSize       =   48;
        ci._spread          =   8;
        ci._atlasWidth      =   512;
        /// 矢量填充不采样图集,这里只烘焙 ASCII 以便度量/步进可用
        ci._uploadTexture   =   false;

        Font    font    =   new FEFont(FEContext::instance());
        if (!font->loadFromFile(fontPath(),ci))
            return  nullptr;
        return  font;
    }

    /// 二维包围盒
    struct  FillBounds
    {
        float   _minX   =   0.0f;
        float   _maxX   =   0.0f;
        float   _minY   =   0.0f;
        float   _maxY   =   0.0f;
        bool    _valid  =   false;
    };

    FillBounds  fillBounds(Mesh mesh)
    {
        FillBounds  b;
        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        if (posBuf == nullptr || posBuf->count() == 0)
            return  b;
        const   float3* pos =   posBuf->buffer()->dataAs<float3>();
        b._minX =   b._maxX =   pos[0].x;
        b._minY =   b._maxY =   pos[0].y;
        for (uint i = 1; i < posBuf->count(); ++i)
        {
            b._minX =   std::min(b._minX,pos[i].x);
            b._maxX =   std::max(b._maxX,pos[i].x);
            b._minY =   std::min(b._minY,pos[i].y);
            b._maxY =   std::max(b._maxY,pos[i].y);
        }
        b._valid    =   true;
        return  b;
    }

    /// 二维点是否落在三角形内(含边界)
    bool    pointInTriangle(const float3& a,const float3& b,const float3& c,const float2& p)
    {
        const   float2  pa(a.x,a.y);
        const   float2  pb(b.x,b.y);
        const   float2  pc(c.x,c.y);
        auto    cross2  =   [](const float2& u,const float2& v)
        {
            return  u.x * v.y - u.y * v.x;
        };
        /// 退化三角形(投影后面积为 0,如挤出的竖直侧壁)不构成覆盖
        if (std::fabs(cross2(pb - pa,pc - pa)) < 1e-4f)
            return  false;
        const   float   d0  =   cross2(pb - pa,p - pa);
        const   float   d1  =   cross2(pc - pb,p - pb);
        const   float   d2  =   cross2(pa - pc,p - pc);
        const   bool    hasNeg  =   (d0 < 0.0f) || (d1 < 0.0f) || (d2 < 0.0f);
        const   bool    hasPos  =   (d0 > 0.0f) || (d1 > 0.0f) || (d2 > 0.0f);
        return  !(hasNeg && hasPos);
    }

    bool    meshCovers(Mesh mesh,const float2& p)
    {
        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        if (posBuf == nullptr)
            return  false;
        const   float3* pos =   posBuf->buffer()->dataAs<float3>();
        const   uint    n   =   posBuf->count();
        for (uint i = 0; i + 2 < n; i += 3)
        {
            if (pointInTriangle(pos[i],pos[i + 1],pos[i + 2],p))
                return  true;
        }
        return  false;
    }

    /// 1. 未加载字体失败路径
    void    testUnloaded(FEContext& ctx)
    {
        FEFont  font(ctx);
        CHECK(font.createTextFillMesh("A",float2(0,0)) == nullptr,
                                                           "未加载字体应返回 null 填充网格");
        CHECK(font.createTextFillMesh("",float2(0,0)) == nullptr,
                                                           "未加载字体空串应返回 null");
    }

    /// 2~4. 基础网格:图元/缓冲/属性/范围
    Mesh    testBasicMesh(const Font& font)
    {
        const   float2  origin(10.0f,20.0f);
        const   Rgba8   color(255,128,32,255);

        CHECK(font->createTextFillMesh("",float2(0,0)) == nullptr,   "空串应生成 null 填充网格");
        CHECK(font->createTextFillMesh("\n",float2(0,0)) == nullptr, "纯换行应生成 null 填充网格");

        Mesh    mesh    =   font->createTextFillMesh("A",origin,1.0f,8,color);
        CHECK(mesh != nullptr,                             "'A' 应生成矢量填充网格");
        if (mesh == nullptr)
            return  nullptr;

        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        auto*   colBuf  =   mesh->get(FEAttribute(IS_VERTEX_COLOR0));
        CHECK(posBuf != nullptr && colBuf != nullptr,      "填充网格应含 pos/color 缓冲");
        if (posBuf == nullptr || colBuf == nullptr)
            return  nullptr;

        CHECK(posBuf->count() == colBuf->count(),          "顶点数与颜色数应一致");
        CHECK(posBuf->count() % 3 == 0,                    "三角形网格顶点数应为 3 的倍数");
        CHECK(posBuf->count() >= 3,                        "填充网格至少含 1 个三角形");
        CHECK(mesh->primitives().size() == 1 &&
              mesh->primitives()[0]->primitive() == PRI_TRIANGLES,
                                                           "填充网格图元应为 TRIANGLES");

        const   float3* pos =   posBuf->buffer()->dataAs<float3>();
        const   Rgba8*  col =   colBuf->buffer()->dataAs<Rgba8>();
        const   auto&   cv  =   color.value();
        bool    finiteOk    =   true;
        bool    colorOk     =   true;
        for (uint i = 0; i < posBuf->count(); ++i)
        {
            finiteOk    =   finiteOk
                        &&  std::isfinite(pos[i].x) && std::isfinite(pos[i].y);
            const   auto&   v   =   col[i].value();
            colorOk     =   colorOk
                        &&  (v.x == cv.x) && (v.y == cv.y) && (v.z == cv.z) && (v.w == cv.w);
        }
        CHECK(finiteOk,                                    "填充顶点坐标应为有限值");
        CHECK(colorOk,                                     "所有顶点颜色应等于指定颜色");

        /// 三角形面积:三角化若退化(零面积)则 GPU 上什么也画不出来
        float   area        =   0.0f;
        uint    degenerate  =   0;
        for (uint i = 0; i + 2 < posBuf->count(); i += 3)
        {
            const   float2  a(pos[i].x,pos[i].y);
            const   float2  b(pos[i + 1].x,pos[i + 1].y);
            const   float2  c(pos[i + 2].x,pos[i + 2].y);
            const   float   s   =   std::fabs((b.x - a.x) * (c.y - a.y) -
                                              (c.x - a.x) * (b.y - a.y)) * 0.5f;
            area    +=  s;
            if (s < 1e-4f)
                ++degenerate;
        }
        CHECK(area > (float)font->glyph('A')._advance * (float)font->ascent() * 0.2f,
                                                           "填充三角形总面积应接近字形面积");
        CHECK(degenerate * 10 < posBuf->count() / 3,        "退化三角形占比应低于 10%");

        const   FEFontGlyph&    gA  =   font->glyph('A');
        const   FillBounds      b   =   fillBounds(mesh);
        CHECK(b._valid,                                    "'A' 填充网格应有有效包围盒");
        CHECK(b._minX >= origin.x - 2.0f &&
              b._maxX <= origin.x + gA._advance + 2.0f,     "'A' 填充 x 范围应在 origin..advance 内");
        CHECK(b._minY >= origin.y - 2.0f &&
              b._maxY <= origin.y + font->ascent() + 2.0f,  "'A' 填充 y 范围应在 baseline..ascent 内");
        return  mesh;
    }

    /// 5. 带孔字形镂空
    void    testHole(const Font& font)
    {
        const   float2  origin(10.0f,20.0f);
        const   Rgba8   color(255,255,255,255);

        Mesh    mesh    =   font->createTextFillMesh("O",origin,1.0f,12,color);
        CHECK(mesh != nullptr,                             "'O' 应生成矢量填充网格");
        if (mesh == nullptr)
            return;

        const   FillBounds  b   =   fillBounds(mesh);
        CHECK(b._valid,                                    "'O' 填充网格应有有效包围盒");
        if (!b._valid)
            return;

        const   float2  center((b._minX + b._maxX) * 0.5f,
                               (b._minY + b._maxY) * 0.5f);
        const   float   w   =   b._maxX - b._minX;
        const   float   h   =   b._maxY - b._minY;
        CHECK(!meshCovers(mesh,center),                    "'O' 中心应为孔洞(未填充)");
        CHECK(meshCovers(mesh,float2(b._minX + w * 0.06f,center.y)),
                                                           "'O' 左侧环带应被填充");
        CHECK(meshCovers(mesh,float2(b._maxX - w * 0.06f,center.y)),
                                                           "'O' 右侧环带应被填充");
        CHECK(meshCovers(mesh,float2(center.x,b._minY + h * 0.06f)),
                                                           "'O' 顶部环带应被填充");
        CHECK(meshCovers(mesh,float2(center.x,b._maxY - h * 0.06f)),
                                                           "'O' 底部环带应被填充");
    }

    /// 6. 三角形绕序一致(便于背面剔除)
    void    testTriangleWinding(Mesh mesh)
    {
        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        CHECK(posBuf != nullptr,                           "绕序测试需要 pos 缓冲");
        if (posBuf == nullptr)
            return;

        const   float3* pos     =   posBuf->buffer()->dataAs<float3>();
        const   uint    n       =   posBuf->count();
        int             total   =   0;
        int             nonDeg  =   0;
        bool    sameSignOk      =   true;
        for (uint i = 0; i + 2 < n; i += 3)
        {
            const   float2  a(pos[i].x,pos[i].y);
            const   float2  b(pos[i + 1].x,pos[i + 1].y);
            const   float2  c(pos[i + 2].x,pos[i + 2].y);
            const   float   area    =   (b.x - a.x) * (c.y - a.y)
                                    -   (b.y - a.y) * (c.x - a.x);
            ++total;
            /// 所有三角形不得为反向绕序(正面积),允许退化为 0
            sameSignOk  =   sameSignOk && (area <= 1e-5f);
            if (std::fabs(area) > 1e-4f)
                ++nonDeg;
        }
        CHECK(total > 0,                                   "应有三角形可校验");
        CHECK(nonDeg > total / 2,                          "大部分三角形应为非退化");
        CHECK(sameSignOk,                                  "所有三角形应保持同一绕序");
    }

    /// 7~8. 多字形、缩放、换行
    void    testLayout(const Font& font,Mesh meshA)
    {
        if (meshA == nullptr)
            return;

        const   float2  origin(10.0f,20.0f);
        const   Rgba8   color(255,255,255,255);

        auto*   posA    =   meshA->get(FEAttribute(IS_VERTEX_POS));
        CHECK(posA != nullptr,                             "'A' 网格应含 pos 缓冲");
        if (posA == nullptr)
            return;

        /// 多字形:逐字形独立三角化,顶点数应可加
        Mesh    meshB   =   font->createTextFillMesh("B",origin,1.0f,8,color);
        Mesh    meshAB  =   font->createTextFillMesh("AB",origin,1.0f,8,color);
        CHECK(meshB != nullptr && meshAB != nullptr,       "'B'/'AB' 应生成填充网格");
        if (meshB != nullptr && meshAB != nullptr)
        {
            auto*   posB    =   meshB->get(FEAttribute(IS_VERTEX_POS));
            auto*   posAB   =   meshAB->get(FEAttribute(IS_VERTEX_POS));
            CHECK(posB != nullptr && posAB != nullptr,     "'B'/'AB' 网格应含 pos 缓冲");
            if (posB != nullptr && posAB != nullptr)
                CHECK(posAB->count() == posA->count() + posB->count(),
                                                           "多字形顶点数应为各字形之和");
        }

        /// 缩放:包围盒宽高线性变化
        Mesh    meshA2  =   font->createTextFillMesh("A",origin,2.0f,8,color);
        CHECK(meshA2 != nullptr,                           "缩放 2x 应生成填充网格");
        if (meshA2 != nullptr)
        {
            const   FillBounds  b1  =   fillBounds(meshA);
            const   FillBounds  b2  =   fillBounds(meshA2);
            CHECK(feq(b2._maxX - b2._minX,(b1._maxX - b1._minX) * 2.0f),
                                                           "缩放 2x:填充宽度应翻倍");
            CHECK(feq(b2._maxY - b2._minY,(b1._maxY - b1._minY) * 2.0f),
                                                           "缩放 2x:填充高度应翻倍");
        }

        /// 换行:第二行下移约一个行高
        Mesh    meshNL  =   font->createTextFillMesh("A\nB",origin,1.0f);
        CHECK(meshNL != nullptr,                           "'A\\nB' 应生成填充网格");
        if (meshNL != nullptr)
        {
            const   FillBounds  b1  =   fillBounds(meshA);
            const   FillBounds  b2  =   fillBounds(meshNL);
            CHECK(b2._maxY > b1._maxY + font->lineHeight() * 0.9f,
                                                           "第二行填充应下移约一个行高");
        }
    }

    /// 9~10. 实体挤出:厚度(z 范围)、法线、颜色、图元、UV
    void    testExtrude(const Font& font)
    {
        const   float2  origin(10.0f,20.0f);
        const   Rgba8   color(200,180,255,255);
        const   float   depth   =   20.0f;

        Mesh    mesh    =   font->createTextExtrudeMesh("A",origin,1.0f,depth,8,color);
        CHECK(mesh != nullptr,                             "'A' 应生成挤出网格");
        if (mesh == nullptr)
            return;

        auto*   posBuf  =   mesh->get(FEAttribute(IS_VERTEX_POS));
        auto*   norBuf  =   mesh->get(FEAttribute(IS_VERTEX_NOR));
        auto*   colBuf  =   mesh->get(FEAttribute(IS_VERTEX_COLOR0));
        CHECK(posBuf != nullptr && norBuf != nullptr && colBuf != nullptr,
                                                           "挤出网格应含 pos/normal/color 缓冲");
        if (posBuf == nullptr || norBuf == nullptr || colBuf == nullptr)
            return;

        CHECK(posBuf->count() == norBuf->count() &&
              posBuf->count() == colBuf->count(),          "pos/normal/color 数量应一致");
        CHECK(posBuf->count() % 3 == 0,                    "挤出网格顶点数应为 3 的倍数");
        CHECK(mesh->primitives().size() == 1 &&
              mesh->primitives()[0]->primitive() == PRI_TRIANGLES,
                                                           "挤出网格图元应为 TRIANGLES");

        const   float3* pos =   posBuf->buffer()->dataAs<float3>();
        const   float3* nor =   norBuf->buffer()->dataAs<float3>();
        const   Rgba8*  col =   colBuf->buffer()->dataAs<Rgba8>();
        const   auto&   cv  =   color.value();

        float   minZ    =   1e9f;
        float   maxZ    =   -1e9f;
        bool    normalOk=   true;
        bool    colorOk =   true;
        uint    frontCnt=   0;
        uint    backCnt =   0;
        uint    sideCnt =   0;
        for (uint i = 0; i < posBuf->count(); ++i)
        {
            minZ    =   std::min(minZ,pos[i].z);
            maxZ    =   std::max(maxZ,pos[i].z);

            const   float   len =   std::sqrt(nor[i].x * nor[i].x +
                                              nor[i].y * nor[i].y +
                                              nor[i].z * nor[i].z);
            normalOk    =   normalOk && feq(len,1.0f);
            if (nor[i].z > 0.9f)        ++frontCnt;
            else if (nor[i].z < -0.9f)  ++backCnt;
            else                        ++sideCnt;

            const   auto&   v   =   col[i].value();
            colorOk     =   colorOk && (v.x == cv.x) && (v.y == cv.y) &&
                            (v.z == cv.z) && (v.w == cv.w);
        }
        /// 厚度:前盖 z=0,后盖 z=-depth
        CHECK(feq(minZ,-depth) && feq(maxZ,0.0f),          "挤出 z 范围应为 [-depth,0]");
        CHECK(frontCnt > 0 && backCnt > 0 && sideCnt > 0,   "应同时含前盖/后盖/侧壁法线");
        CHECK(normalOk,                                    "所有法线应为单位向量");
        CHECK(colorOk,                                     "所有顶点颜色应等于指定颜色");

        /// UV:按字体像素尺寸归一化的平面映射,供 PBR 等需要 TEXCOORD0 的管线
        auto*   uvBuf   =   mesh->get(FEAttribute(IS_VERTEX_TEXCOORD0));
        CHECK(uvBuf != nullptr,                            "挤出网格应含 TEXCOORD0(供 PBR)");
        if (uvBuf != nullptr)
        {
            CHECK(uvBuf->count() == posBuf->count(),       "UV 数量应与顶点数一致");
            const   float2* uv  =   uvBuf->buffer()->dataAs<float2>();
            bool    uvOk        =   true;
            for (uint i = 0; i < uvBuf->count(); ++i)
                uvOk    =   uvOk && std::isfinite(uv[i].x) && std::isfinite(uv[i].y);
            CHECK(uvOk,                                    "UV 应为有限值");
        }

        /// 前盖平面覆盖字形区域,后盖镜像,故顶点数明显多于纯填充
        Mesh    fill    =   font->createTextFillMesh("A",origin,1.0f,8,color);
        CHECK(fill != nullptr,                             "对比用填充网格应生成");
        if (fill != nullptr)
        {
            auto*   fillPos =   fill->get(FEAttribute(IS_VERTEX_POS));
            CHECK(fillPos != nullptr && posBuf->count() > fillPos->count(),
                                                           "挤出顶点数应多于平面填充");
        }

        /// depth <= 0 时按最小厚度 1 处理
        Mesh    flat    =   font->createTextExtrudeMesh("A",origin,1.0f,0.0f,8,color);
        CHECK(flat != nullptr,                             "depth=0 应生成挤出网格");
        if (flat != nullptr)
        {
            auto*   p2  =   flat->get(FEAttribute(IS_VERTEX_POS));
            const   float3* pos2 =   p2 ? p2->buffer()->dataAs<float3>() : nullptr;
            float   minZ2 = 1e9f;
            for (uint i = 0; p2 != nullptr && i < p2->count(); ++i)
                minZ2 = std::min(minZ2,pos2[i].z);
            CHECK(feq(minZ2,-1.0f),                        "depth<=0 应钳制为 1");
        }
    }
}

int     main()
{
    FEContext&  ctx =   FEContext::instance();
    std::cout << "font: " << fontPath() << std::endl;

    testUnloaded(ctx);

    Font    font    =   makeFont();
    CHECK(font != nullptr,                                 "测试字体应烘焙成功");
    if (font != nullptr)
    {
        Mesh    meshA   =   testBasicMesh(font);
        CHECK(meshA != nullptr,                            "基础填充网格应生成成功");
        if (meshA != nullptr)
            testTriangleWinding(meshA);
        testHole(font);
        testLayout(font,meshA);
        testExtrude(font);
    }

    std::cout << "checked=" << g_checked
              << " failed=" << g_failed << std::endl;
    if (g_failed != 0)
    {
        std::cerr << "test.sdf.text.fill: FAILED (" << g_failed << ")" << std::endl;
        return  1;
    }
    std::cout << "test.sdf.text.fill: CPU checks PASS" << std::endl;
    return  0;
}

#include    "font/FEFont.hpp"

#include    <ft2build.h>
#include    FT_FREETYPE_H
/// FT_Property_Set(SDF spread 属性)声明于 ftmodapi.h
#include    FT_MODULE_H
/// FT_Outline_Decompose(字形轮廓提取)声明于 ftoutln.h
#include    FT_OUTLINE_H

#include    <fstream>
#include    <algorithm>
#include    <cmath>

namespace   FE
{
    struct  FEFontImpl
    {
        FT_Library              _library    =   nullptr;
        FT_Face                 _face       =   nullptr;
        /// 字库原始数据(内存模式必须在其生命周期内保持有效)
        std::vector<uint8_t>    _data;
    };

    namespace
    {
        /// SDF 渲染必须关闭 hinting/bitmap,以得到平滑轮廓
        const   FT_Int32    kLoadFlags  =   FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;

        inline  uint    nextPow2(uint v)
        {
            uint    r   =   1;
            while (r < v)
                r   <<= 1;
            return  r;
        }

        /// <summary>
        /// UTF-8 解码:返回当前码点并前移指针(非法字节按单字节处理)
        /// </summary>
        inline  uint32  decodeUTF8(const char*& p,const char* end)
        {
            uint8_t c   =   (uint8_t)(*p++);
            if (c < 0x80)
                return  c;
            uint32  code    =   0;
            int     extra   =   0;
            if ((c & 0xE0) == 0xC0)         { code = c & 0x1F; extra = 1; }
            else if ((c & 0xF0) == 0xE0)    { code = c & 0x0F; extra = 2; }
            else if ((c & 0xF8) == 0xF0)    { code = c & 0x07; extra = 3; }
            else
                return  c;
            for (int i = 0; i < extra && p < end; ++i)
            {
                uint8_t cc  =   (uint8_t)(*p);
                if ((cc & 0xC0) != 0x80)
                    break;
                ++p;
                code    =   (code << 6) | (cc & 0x3F);
            }
            return  code;
        }
    }

    FEFont::FEFont(FEContext& ctx)
        :FEObject(ctx)
    {
        _impl   =   new FEFontImpl();
    }

    FEFont::FEFont(const FEFont& other)
        :FEObject(other)
    {
        /// 字体句柄不可跨实例共享,拷贝仅复制图集引用与度量
        _texture    =   other._texture;
        _glyphIndex =   other._glyphIndex;
        _glyphs     =   other._glyphs;
        _atlasPixels=   other._atlasPixels;
        _atlasW     =   other._atlasW;
        _atlasH     =   other._atlasH;
        _spread     =   other._spread;
        _ascent     =   other._ascent;
        _lineHeight =   other._lineHeight;
    }

    FEFont::~FEFont()
    {
        if (_impl != nullptr)
        {
            if (_impl->_face != nullptr)
                FT_Done_Face(_impl->_face);
            if (_impl->_library != nullptr)
                FT_Done_FreeType(_impl->_library);
            delete  _impl;
            _impl   =   nullptr;
        }
    }

    bool    FEFont::loadFromFile(const String& path,const FEFontCreateInfo& ci)
    {
        std::ifstream   ifs(path,std::ios::binary);
        if (!ifs.is_open())
            return  false;
        ifs.seekg(0,std::ios::end);
        auto    bytes   =   (size_t)ifs.tellg();
        if (bytes == 0)
            return  false;
        ifs.seekg(0,std::ios::beg);
        std::vector<uint8_t>    data(bytes);
        ifs.read((char*)data.data(),(std::streamsize)bytes);
        if (!ifs)
            return  false;
        return  loadFromMemory(data.data(),data.size(),ci);
    }

    bool    FEFont::loadFromMemory(const void* data,size_t bytes,const FEFontCreateInfo& ci)
    {
        if (_impl == nullptr || data == nullptr || bytes == 0)
            return  false;
        _impl->_data.assign((const uint8_t*)data,(const uint8_t*)data + bytes);
        if (_impl->_library == nullptr)
        {
            if (FT_Init_FreeType(&_impl->_library) != 0)
                return  false;
        }
        if (_impl->_face != nullptr)
        {
            FT_Done_Face(_impl->_face);
            _impl->_face    =   nullptr;
        }
        if (FT_New_Memory_Face(_impl->_library,_impl->_data.data(),
                               (FT_Long)_impl->_data.size(),0,&_impl->_face) != 0)
            return  false;
        /// 纯 CPU 烘焙图集与度量
        if (!bake(ci))
            return  false;
        /// CPU-only 模式(单元测试):不上传 GPU
        if (!ci._uploadTexture)
            return  true;
        return  uploadAtlas();
    }

    bool    FEFont::bake(const FEFontCreateInfo& ci)
    {
        if (_impl == nullptr || _impl->_face == nullptr)
            return  false;
        FT_Face face    =   _impl->_face;

        /// ---- 选择渲染路径 ----
        /// SDF: 高焙烤尺寸 + 带符号距离场,大字号无级缩放
        /// 位图(hinting 灰度):原生像素尺寸 hinted 8-bit 灰度,小字号防断笔
        const   bool            bitmap      =   ci._bitmapMode;
        const   FT_Render_Mode  renderMode  =   bitmap ? FT_RENDER_MODE_NORMAL
                                                       : FT_RENDER_MODE_SDF;
        /// SDF 必须关闭 hinting/bitmap 以得到平滑轮廓;位图启用默认 hinting
        const   FT_Int32        loadFlags   =   bitmap ? FT_LOAD_DEFAULT
                                                       : kLoadFlags;
        _bitmap =   bitmap;

        if (bitmap)
        {
            _spread =   0;
        }
        else
        {
            /// SDF 扩散半径须在渲染前设置(FreeType 允许 [2,32])
            FT_Int  spread  =   (FT_Int)(ci._spread < 2 ? 2 : (ci._spread > 32 ? 32 : ci._spread));
            FT_Property_Set(_impl->_library,"sdf","spread",&spread);
            _spread =   (uint)spread;
        }

        if (FT_Set_Pixel_Sizes(face,0,ci._pixelSize) != 0)
            return  false;
        _pixelSize  =   ci._pixelSize;

        /// ---- 收集待烘焙码点:显式字符集优先,否则用连续区间 ----
        std::vector<uint32>     codes;
        if (!ci._characters.empty())
        {
            const   char*   p   =   ci._characters.c_str();
            const   char*   end =   p + ci._characters.size();
            while (p < end)
            {
                uint32  code    =   decodeUTF8(p,end);
                /// 去重(同一字符只在图集中烘焙一次)
                if (std::find(codes.begin(),codes.end(),code) == codes.end())
                    codes.emplace_back(code);
            }
        }
        else
        {
            for (uint32 code = ci._firstChar; code <= ci._lastChar; ++code)
                codes.emplace_back(code);
        }
        if (codes.empty())
            return  false;

        _glyphs.assign(codes.size(),FEFontGlyph());
        _glyphIndex.clear();
        _glyphIndex.reserve(codes.size());
        for (uint i = 0; i < (uint)codes.size(); ++i)
            _glyphIndex[codes[i]]   =   i;

        /// 行高/基线(26.6 定点转像素)
        _lineHeight =   (float)face->size->metrics.height   / 64.0f;
        _ascent     =   (float)face->size->metrics.ascender / 64.0f;

        /// <summary>
        /// ---- 第一遍:SDF 渲染取得每个字形的位图尺寸,并做货架式打包 ----
        /// </summary>
        struct  GlyphBox
        {
            uint32  _code   =   0;
            /// 在 _glyphs 中的下标
            uint    _index  =   0;
            uint    _x      =   0;
            uint    _y      =   0;
            float   _advance=   0.0f;
        };
        std::vector<GlyphBox>   boxes;
        boxes.reserve(codes.size());

        uint    maxW    =   0;
        for (uint i = 0; i < (uint)codes.size(); ++i)
        {
            GlyphBox    box;
            box._code   =   codes[i];
            box._index  =   i;
            if (FT_Load_Char(face,box._code,loadFlags) == 0 &&
                FT_Render_Glyph(face->glyph,renderMode) == 0)
            {
                box._advance    =   (float)face->glyph->advance.x / 64.0f;
                maxW            =   std::max(maxW,(uint)face->glyph->bitmap.width);
            }
            boxes.emplace_back(box);
        }

        uint    atlasW  =   std::max(ci._atlasWidth,maxW + 2);
        if (atlasW < 4)
            atlasW  =   4;

        /// <summary>
        /// 按高度降序做货架打包,保证同一行内不会溢出
        /// </summary>
        std::vector<uint>   order(boxes.size());
        for (uint i = 0; i < (uint)order.size(); ++i)
            order[i]    =   i;
        std::sort(order.begin(),order.end(),[&](uint a,uint b)
        {
            return  boxes[a]._advance < boxes[b]._advance;
        });
        /// <summary>
        /// 用实际位图高度排序(需要重新渲染取高度,此处复用第一遍结果)
        /// </summary>
        std::vector<uint>   heights(boxes.size(),0);
        for (size_t i = 0; i < boxes.size(); ++i)
        {
            if (FT_Load_Char(face,boxes[i]._code,loadFlags) == 0 &&
                FT_Render_Glyph(face->glyph,renderMode) == 0)
            {
                heights[i]  =   (uint)face->glyph->bitmap.rows;
            }
        }
        std::sort(order.begin(),order.end(),[&](uint a,uint b)
        {
            return  heights[a] > heights[b];
        });

        uint    curX    =   0;
        uint    curY    =   0;
        uint    rowH    =   0;
        for (uint i : order)
        {
            if (FT_Load_Char(face,boxes[i]._code,loadFlags) == 0 &&
                FT_Render_Glyph(face->glyph,renderMode) == 0)
            {
                uint    w   =   (uint)face->glyph->bitmap.width;
                uint    h   =   (uint)face->glyph->bitmap.rows;
                if (w == 0 || h == 0)
                    continue;
                if (curX + w > atlasW)
                {
                    curX    =   0;
                    curY    +=  rowH;
                    rowH    =   0;
                }
                boxes[i]._x =   curX;
                boxes[i]._y =   curY;
                curX        +=  w;
                rowH        =   std::max(rowH,h);
            }
        }

        uint    usedH   =   curY + rowH;
        if (usedH == 0)
            usedH   =   4;
        uint    atlasH  =   std::max(nextPow2(usedH),4u);

        /// ---- 第二遍:渲染并写入 SDF 图集(RGBA8,R 通道存 SDF) ----
        _atlasPixels.assign((size_t)atlasW * atlasH * 4,0);
        _atlasW =   atlasW;
        _atlasH =   atlasH;

        for (const GlyphBox& box : boxes)
        {
            FEFontGlyph&    g   =   _glyphs[box._index];
            if (FT_Load_Char(face,box._code,loadFlags) != 0 ||
                FT_Render_Glyph(face->glyph,renderMode) != 0)
                continue;

            const   FT_Bitmap&  bm  =   face->glyph->bitmap;
            g._advance  =   (float)face->glyph->advance.x / 64.0f;
            /// 空白字形(如空格):无位图,仅保留步进
            if (bm.width == 0 || bm.rows == 0)
                continue;

            for (uint r = 0; r < bm.rows; ++r)
            {
                const   uint8_t*    src =   bm.buffer + (size_t)r * bm.pitch;
                uint8_t*            dst =   _atlasPixels.data() + ((size_t)(box._y + r) * atlasW + box._x) * 4;
                for (uint c = 0; c < bm.width; ++c)
                {
                    uint8_t v   =   src[c];
                    dst[c * 4 + 0]  =   v;
                    dst[c * 4 + 1]  =   v;
                    dst[c * 4 + 2]  =   v;
                    dst[c * 4 + 3]  =   v;
                }
            }

            g._uv0      =   float2((float)box._x / (float)atlasW,
                                   (float)box._y / (float)atlasH);
            g._uv1      =   float2((float)(box._x + bm.width)  / (float)atlasW,
                                   (float)(box._y + bm.rows)   / (float)atlasH);
            g._size     =   float2((float)bm.width,(float)bm.rows);
            /// bitmap_left/top 是字形位图相对笔位置的偏移
            /// (SDF 模式下额外包含 spread padding),转成 y 向下的左上偏移
            g._offset   =   float2((float)face->glyph->bitmap_left,
                                   (float)(-face->glyph->bitmap_top));
            g._valid    =   true;
        }
        return  true;
    }

    bool    FEFont::uploadAtlas()
    {
        if (!isValid())
            return  false;

        _texture    =   _ctx.device().createTexture();
        if (_texture == nullptr)
            return  false;

        FETexture::CreateInfo   texInfo =   {};
        texInfo._width      =   _atlasW;
        texInfo._height     =   _atlasH;
        texInfo._format     =   FMT_R8G8B8A8_UNORM;
        texInfo._magFilter  =   FILTER_LINEAR;
        texInfo._minFilter  =   FILTER_LINEAR;
        /// SDF 图集必须钳制寻址,避免跨字形渗色
        texInfo._addressMode=   ADDRESS_CLAMP_TO_EDGE;
        if (!_texture->create(texInfo))
            return  false;
        return  _texture->update(_atlasPixels.data(),_atlasPixels.size());
    }

    uint    FEFont::glyphCount() const
    {
        uint    n   =   0;
        for (const FEFontGlyph& g : _glyphs)
        {
            if (g._valid)
                ++n;
        }
        return  n;
    }

    String  FEFont::familyName() const
    {
        if (_impl == nullptr || _impl->_face == nullptr || _impl->_face->family_name == nullptr)
            return  {};
        return  _impl->_face->family_name;
    }

    const   FEFontGlyph&    FEFont::glyph(uint32 code) const
    {
        static  FEFontGlyph sInvalid;
        auto    itr =   _glyphIndex.find(code);
        if (itr == _glyphIndex.end() || itr->second >= _glyphs.size())
            return  sInvalid;
        return  _glyphs[itr->second];
    }

    float   FEFont::measureWidth(const String& text,float scale) const
    {
        float       width   =   0.0f;
        const char* p       =   text.c_str();
        const char* end     =   p + text.size();
        while (p < end)
        {
            uint32  code    =   decodeUTF8(p,end);
            if (code == '\n' || code == '\r')
                continue;
            width   +=  glyph(code)._advance * scale;
        }
        return  width;
    }

    Mesh    FEFont::createTextMesh(const String& text,const float2& origin,float scale) const
    {
        if (!isValid())
            return  nullptr;

        float3s     poss;
        float2s     uvs;

        float       penX    =   origin.x;
        float       penY    =   origin.y + _ascent * scale;
        const char* p       =   text.c_str();
        const char* end     =   p + text.size();
        while (p < end)
        {
            uint32  code    =   decodeUTF8(p,end);
            if (code == '\n' || code == '\r')
            {
                penX    =   origin.x;
                penY    +=  _lineHeight * scale;
                continue;
            }

            const   FEFontGlyph&    g   =   glyph(code);
            if (g._valid)
            {
                float   x0  =   penX + g._offset.x * scale;
                float   y0  =   penY + g._offset.y * scale;
                float   x1  =   x0   + g._size.x   * scale;
                float   y1  =   y0   + g._size.y   * scale;
                /// 屏幕像素空间中 y 向下,两个三角形共 6 个顶点
                poss.emplace_back(float3(x0,y1,0.0f));  uvs.emplace_back(float2(g._uv0.x,g._uv1.y));
                poss.emplace_back(float3(x1,y1,0.0f));  uvs.emplace_back(float2(g._uv1.x,g._uv1.y));
                poss.emplace_back(float3(x1,y0,0.0f));  uvs.emplace_back(float2(g._uv1.x,g._uv0.y));
                poss.emplace_back(float3(x0,y1,0.0f));  uvs.emplace_back(float2(g._uv0.x,g._uv1.y));
                poss.emplace_back(float3(x1,y0,0.0f));  uvs.emplace_back(float2(g._uv1.x,g._uv0.y));
                poss.emplace_back(float3(x0,y0,0.0f));  uvs.emplace_back(float2(g._uv0.x,g._uv0.y));
            }
            penX    +=  g._advance * scale;
        }

        if (poss.empty())
            return  nullptr;
        return  FEMeshBuilder::makeMesh(_ctx,poss,Rgba8s(),float3s(),uvs);
    }

    // ====================================================================
    // 矢量轮廓线:从 FreeType 字形中提取轮廓,离散为 PRI_LINES 线段
    // ====================================================================
    namespace
    {
        /// <summary>
        /// 轮廓提取回调上下文
        /// </summary>
        struct  OutlineCtx
        {
            float3s*    _poss       =   nullptr;
            Rgba8s*     _colors     =   nullptr;
            float       _penX       =   0.0f;
            float       _penY       =   0.0f;
            float       _scale      =   1.0f;
            uint        _curveSteps =   8;
            Rgba8       _color      =   Rgba8(255,255,255,255);
            /// 当前点(烘焙像素,FreeType y 向上)
            float2      _cur        =   float2(0.0f);
            /// 轮廓起点(用于闭合)
            float2      _start      =   float2(0.0f);
            bool        _hasStart   =   false;
        };

        /// <summary>
        /// FT_Vector(26.6 定点)转烘焙像素
        /// </summary>
        inline  float2  ftToPx(const FT_Vector* v)
        {
            return  float2(v->x / 64.0f,v->y / 64.0f);
        }

        /// <summary>
        /// 发射一条线段(a->b,烘焙像素),换算到屏幕像素(y 向下)
        /// </summary>
        inline  void    emitSeg(OutlineCtx& c,const float2& a,const float2& b)
        {
            c._poss->emplace_back(float3(c._penX + a.x * c._scale,
                                         c._penY - a.y * c._scale,0.0f));
            c._poss->emplace_back(float3(c._penX + b.x * c._scale,
                                         c._penY - b.y * c._scale,0.0f));
            c._colors->emplace_back(c._color);
            c._colors->emplace_back(c._color);
        }

        int outlineMoveTo(const FT_Vector* to,void* user)
        {
            OutlineCtx& c    =   *(OutlineCtx*)user;
            /// 新轮廓开始前,闭合上一条轮廓
            if (c._hasStart)
                emitSeg(c,c._cur,c._start);
            c._cur      =   ftToPx(to);
            c._start    =   c._cur;
            c._hasStart =   true;
            return  0;
        }
        int outlineLineTo(const FT_Vector* to,void* user)
        {
            OutlineCtx& c    =   *(OutlineCtx*)user;
            float2      t    =   ftToPx(to);
            emitSeg(c,c._cur,t);
            c._cur  =   t;
            return  0;
        }
        int outlineConicTo(const FT_Vector* control,const FT_Vector* to,void* user)
        {
            OutlineCtx& c    =   *(OutlineCtx*)user;
            const float2    p0  =   c._cur;
            const float2    p1  =   ftToPx(control);
            const float2    p2  =   ftToPx(to);
            const uint      n   =   c._curveSteps;
            for (uint i = 1; i <= n; ++i)
            {
                float   t   =   (float)i / (float)n;
                float   u   =   1.0f - t;
                float2  pt  =   u*u*p0 + 2.0f*u*t*p1 + t*t*p2;
                emitSeg(c,c._cur,pt);
                c._cur  =   pt;
            }
            c._cur  =   p2;
            return  0;
        }
        int outlineCubicTo(const FT_Vector* c1,const FT_Vector* c2,const FT_Vector* to,void* user)
        {
            OutlineCtx& c    =   *(OutlineCtx*)user;
            const float2    p0  =   c._cur;
            const float2    p1  =   ftToPx(c1);
            const float2    p2  =   ftToPx(c2);
            const float2    p3  =   ftToPx(to);
            const uint      n   =   c._curveSteps;
            for (uint i = 1; i <= n; ++i)
            {
                float   t   =   (float)i / (float)n;
                float   u   =   1.0f - t;
                float2  pt  =   u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
                emitSeg(c,c._cur,pt);
                c._cur  =   pt;
            }
            c._cur  =   p3;
            return  0;
        }
    }

    Mesh    FEFont::createTextOutlineMesh(const String& text,const float2& origin,
                                          float scale,uint curveSteps,const Rgba8& color) const
    {
        if (!isValid() || _impl == nullptr || _impl->_face == nullptr || _pixelSize == 0)
            return  nullptr;
        FT_Face face    =   _impl->_face;
        /// 确保字形按烘焙尺寸加载,使轮廓坐标与度量单位一致
        if (FT_Set_Pixel_Sizes(face,0,_pixelSize) != 0)
            return  nullptr;

        float3s     poss;
        Rgba8s      colors;

        OutlineCtx  ctx;
        ctx._poss       =   &poss;
        ctx._colors     =   &colors;
        ctx._scale      =   scale;
        ctx._curveSteps =   (curveSteps == 0) ? 8u : curveSteps;
        ctx._color      =   color;

        FT_Outline_Funcs    funcs   =   {};
        funcs.move_to   =   outlineMoveTo;
        funcs.line_to   =   outlineLineTo;
        funcs.conic_to  =   outlineConicTo;
        funcs.cubic_to  =   outlineCubicTo;
        funcs.shift     =   0;
        funcs.delta     =   0;

        float       penX    =   origin.x;
        float       penY    =   origin.y + _ascent * scale;
        const char* p       =   text.c_str();
        const char* end     =   p + text.size();
        while (p < end)
        {
            uint32  code    =   decodeUTF8(p,end);
            if (code == '\n' || code == '\r')
            {
                penX    =   origin.x;
                penY    +=  _lineHeight * scale;
                continue;
            }

            const   FEFontGlyph&    g   =   glyph(code);
            if (g._valid)
            {
                ctx._penX     =   penX;
                ctx._penY     =   penY;
                ctx._hasStart =   false;
                FT_UInt idx   =   FT_Get_Char_Index(face,code);
                if (idx != 0 && FT_Load_Glyph(face,idx,FT_LOAD_NO_BITMAP) == 0)
                {
                    FT_Outline_Decompose(&face->glyph->outline,&funcs,&ctx);
                    /// 闭合最后一条轮廓
                    if (ctx._hasStart)
                    {
                        emitSeg(ctx,ctx._cur,ctx._start);
                        ctx._hasStart =   false;
                    }
                }
            }
            penX    +=  g._advance * scale;
        }

        if (poss.empty())
            return  nullptr;
        Attrs   attrs   =   { FEAttribute(IS_VERTEX_POS,    FMT_R32G32B32_FLOAT),
                              FEAttribute(IS_VERTEX_COLOR0, FMT_R8G8B8A8_UNORM) };
        return  FEMeshBuilder::makeMeshLine(_ctx,attrs,poss,colors);
    }

    // ====================================================================
    // 矢量填充模式:轮廓提取 -> 非零环绕三角化 -> PRI_TRIANGLES 网格
    // ====================================================================
    namespace
    {
        using   FillContour     =   std::vector<float2>;
        using   FillContours    =   std::vector<FillContour>;

        /// <summary>
        /// 填充轮廓提取上下文:轮廓点直接换算到屏幕像素(y 向下)
        /// </summary>
        struct  FillOutlineCtx
        {
            FillContours*   _contours   =   nullptr;
            /// 当前轮廓(指向 _contours 的最后一个元素)
            FillContour*    _cur        =   nullptr;
            float           _penX       =   0.0f;
            float           _penY       =   0.0f;
            float           _scale      =   1.0f;
            uint            _curveSteps =   8;

            inline  float2  toScreen(const FT_Vector* v) const
            {
                return  float2(_penX + (v->x / 64.0f) * _scale,
                               _penY - (v->y / 64.0f) * _scale);
            }
        };

        int fillMoveTo(const FT_Vector* to,void* user)
        {
            FillOutlineCtx& c   =   *(FillOutlineCtx*)user;
            c._contours->emplace_back();
            c._cur  =   &c._contours->back();
            c._cur->emplace_back(c.toScreen(to));
            return  0;
        }
        int fillLineTo(const FT_Vector* to,void* user)
        {
            FillOutlineCtx& c   =   *(FillOutlineCtx*)user;
            c._cur->emplace_back(c.toScreen(to));
            return  0;
        }
        int fillConicTo(const FT_Vector* control,const FT_Vector* to,void* user)
        {
            FillOutlineCtx& c   =   *(FillOutlineCtx*)user;
            const   float2  p0  =   c._cur->back();
            const   float2  p1  =   c.toScreen(control);
            const   float2  p2  =   c.toScreen(to);
            const   uint    n   =   c._curveSteps;
            for (uint i = 1; i <= n; ++i)
            {
                const   float   t   =   (float)i / (float)n;
                const   float   u   =   1.0f - t;
                c._cur->emplace_back(u * u * p0 + 2.0f * u * t * p1 + t * t * p2);
            }
            return  0;
        }
        int fillCubicTo(const FT_Vector* c1,const FT_Vector* c2,const FT_Vector* to,void* user)
        {
            FillOutlineCtx& c   =   *(FillOutlineCtx*)user;
            const   float2  p0  =   c._cur->back();
            const   float2  p1  =   c.toScreen(c1);
            const   float2  p2  =   c.toScreen(c2);
            const   float2  p3  =   c.toScreen(to);
            const   uint    n   =   c._curveSteps;
            for (uint i = 1; i <= n; ++i)
            {
                const   float   t   =   (float)i / (float)n;
                const   float   u   =   1.0f - t;
                c._cur->emplace_back(u * u * u * p0 + 3.0f * u * u * t * p1 +
                                     3.0f * u * t * t * p2 + t * t * t * p3);
            }
            return  0;
        }

        /// <summary>
        /// 提取单个字形的全部闭合轮廓(屏幕像素坐标,首尾不重复)
        /// </summary>
        void    collectGlyphContours(FT_Face face,uint32 code,float penX,float penY,
                                     float scale,uint curveSteps,FillContours& contours)
        {
            contours.clear();
            const   FT_UInt idx =   FT_Get_Char_Index(face,code);
            if (idx == 0 || FT_Load_Glyph(face,idx,kLoadFlags) != 0)
                return;

            FillOutlineCtx  ctx;
            ctx._contours   =   &contours;
            ctx._penX       =   penX;
            ctx._penY       =   penY;
            ctx._scale      =   scale;
            ctx._curveSteps =   curveSteps;

            FT_Outline_Funcs    funcs   =   {};
            funcs.move_to   =   fillMoveTo;
            funcs.line_to   =   fillLineTo;
            funcs.conic_to  =   fillConicTo;
            funcs.cubic_to  =   fillCubicTo;
            FT_Outline_Decompose(&face->glyph->outline,&funcs,&ctx);
        }

        /// <summary>
        /// 一条跨扫描带的非水平边(屏幕像素,y 向下)
        /// </summary>
        struct  FillEdge
        {
            float2  _top        =   float2(0.0f);
            float2  _bottom     =   float2(0.0f);
            /// 沿原轮廓方向穿过该边时环绕数的增量(向下 +1,向上 -1)
            int     _dir        =   0;

            inline  float   xAt(float y) const
            {
                const   float   dy  =   _bottom.y - _top.y;
                if (dy <= 1e-6f)
                    return  _top.x;
                return  _top.x + (_bottom.x - _top.x) * ((y - _top.y) / dy);
            }
        };

        /// <summary>
        /// 扫描线与边的交点
        /// </summary>
        struct  FillCrossing
        {
            float   _x      =   0.0f;
            int     _edge   =   0;
        };

        /// <summary>
        /// 按非零环绕规则把多条闭合轮廓三角化为梯形条带。
        /// 扫描带由所有顶点 y 值切分,带内每条边互不交叉,
        /// 因此交点按 x 排序后累计环绕数,环绕数从 0 变为非 0 即为填充区间的左右边界。
        /// </summary>
        void    triangulateContours(const FillContours& contours,float3s& poss,
                                    Rgba8s& colors,const Rgba8& color)
        {
            std::vector<FillEdge>   edges;
            edges.reserve(contours.size() * 4);
            std::vector<float>      ys;
            for (const FillContour& contour : contours)
            {
                const   size_t  n   =   contour.size();
                if (n < 3)
                    continue;
                for (size_t i = 0; i < n; ++i)
                {
                    const   float2  a   =   contour[i];
                    const   float2  b   =   contour[(i + 1) % n];
                    /// 水平边不参与扫描线求交
                    if (std::fabs(b.y - a.y) < 1e-6f)
                        continue;
                    FillEdge    e;
                    if (a.y < b.y)
                    {
                        e._top      =   a;
                        e._bottom   =   b;
                        e._dir      =   1;
                    }
                    else
                    {
                        e._top      =   b;
                        e._bottom   =   a;
                        e._dir      =   -1;
                    }
                    ys.emplace_back(a.y);
                    ys.emplace_back(b.y);
                    edges.emplace_back(e);
                }
            }
            if (edges.empty())
                return;

            std::sort(ys.begin(),ys.end());
            ys.erase(std::unique(ys.begin(),ys.end(),
                     [](float a,float b){ return std::fabs(a - b) < 1e-4f; }),ys.end());
            if (ys.size() < 2)
                return;

            const   auto    emitTri =   [&](const float2& a,const float2& b,const float2& c)
            {
                poss.emplace_back(float3(a.x,a.y,0.0f));    colors.emplace_back(color);
                poss.emplace_back(float3(b.x,b.y,0.0f));    colors.emplace_back(color);
                poss.emplace_back(float3(c.x,c.y,0.0f));    colors.emplace_back(color);
            };

            std::vector<FillCrossing>   crossings;
            for (size_t i = 0; i + 1 < ys.size(); ++i)
            {
                const   float   y0  =   ys[i];
                const   float   y1  =   ys[i + 1];
                if (y1 - y0 < 1e-4f)
                    continue;
                const   float   yMid=   (y0 + y1) * 0.5f;

                crossings.clear();
                for (size_t k = 0; k < edges.size(); ++k)
                {
                    const   FillEdge&   e   =   edges[k];
                    if (e._top.y <= y0 + 1e-4f && e._bottom.y >= y1 - 1e-4f)
                        crossings.emplace_back(FillCrossing{e.xAt(yMid),(int)k});
                }
                if (crossings.empty())
                    continue;
                std::sort(crossings.begin(),crossings.end(),
                        [](const FillCrossing& a,const FillCrossing& b)
                        { return a._x < b._x; });

                int     winding     =   0;
                int     spanStart   =   -1;
                for (size_t k = 0; k < crossings.size(); ++k)
                {
                    const   int     prev    =   winding;
                    winding +=  edges[crossings[k]._edge]._dir;
                    if (prev == 0 && winding != 0)
                    {
                        spanStart   =   (int)k;
                    }
                    else if (prev != 0 && winding == 0 && spanStart >= 0)
                    {
                        const   FillEdge&   eL  =   edges[crossings[spanStart]._edge];
                        const   FillEdge&   eR  =   edges[crossings[k]._edge];
                        /// y0 为上边(值较小),y1 为下边
                        const   float2  tl(eL.xAt(y0),y0);
                        const   float2  tr(eR.xAt(y0),y0);
                        const   float2  br(eR.xAt(y1),y1);
                        const   float2  bl(eL.xAt(y1),y1);
                        /// 与 createTextMesh 相同的绕序(节点 y 翻转后为正面)
                        emitTri(bl,br,tr);
                        emitTri(bl,tr,tl);
                    }
                }
            }
        }
    }

    Mesh    FEFont::createTextFillMesh(const String& text,const float2& origin,
                                       float scale,uint curveSteps,const Rgba8& color) const
    {
        if (!isValid() || _impl == nullptr || _impl->_face == nullptr || _pixelSize == 0)
            return  nullptr;
        FT_Face face    =   _impl->_face;
        /// 轮廓与字形度量使用同一烘焙像素尺寸
        if (FT_Set_Pixel_Sizes(face,0,_pixelSize) != 0)
            return  nullptr;

        float3s     poss;
        Rgba8s      colors;

        const   uint    steps   =   (curveSteps == 0) ? 8u : curveSteps;
        float       penX    =   origin.x;
        float       penY    =   origin.y + _ascent * scale;
        const char* p       =   text.c_str();
        const char* end     =   p + text.size();
        while (p < end)
        {
            uint32  code    =   decodeUTF8(p,end);
            if (code == '\n' || code == '\r')
            {
                penX    =   origin.x;
                penY    +=  _lineHeight * scale;
                continue;
            }

            const   FEFontGlyph&    g   =   glyph(code);
            if (g._valid)
            {
                FillContours    contours;
                collectGlyphContours(face,code,penX,penY,scale,steps,contours);
                triangulateContours(contours,poss,colors,color);
            }
            penX    +=  g._advance * scale;
        }

        if (poss.empty())
            return  nullptr;
        Mesh    mesh    =   FEMeshBuilder::makeMesh(_ctx,poss,colors);
        if (mesh != nullptr)
            mesh->updateAabb();
        return  mesh;
    }

    // ====================================================================
    // 实体挤出:前/后盖 + 侧壁,输出 pos + normal + color0
    // ====================================================================
    namespace
    {
        /// <summary>
        /// 计算轮廓的有向面积(用于判定外法线方向)
        /// </summary>
        float   contourArea2(const FillContour& contour)
        {
            const   size_t  n   =   contour.size();
            if (n < 3)
                return  0.0f;
            float   area    =   0.0f;
            for (size_t i = 0; i < n; ++i)
            {
                const   float2& a   =   contour[i];
                const   float2& b   =   contour[(i + 1) % n];
                area    +=  a.x * b.y - b.x * a.y;
            }
            return  area * 0.5f;
        }
    }

    Mesh    FEFont::createTextExtrudeMesh(const String& text,const float2& origin,
                                          float scale,float depth,uint curveSteps,
                                          const Rgba8& color) const
    {
        if (!isValid() || _impl == nullptr || _impl->_face == nullptr || _pixelSize == 0)
            return  nullptr;
        FT_Face face    =   _impl->_face;
        if (FT_Set_Pixel_Sizes(face,0,_pixelSize) != 0)
            return  nullptr;
        if (depth <= 0.0f)
            depth   =   1.0f;

        float3s     poss;
        float3s     normals;
        float2s     uvs;
        Rgba8s      colors;

        const   uint    steps   =   (curveSteps == 0) ? 8u : curveSteps;
        const   float3  nFront  =   float3(0.0f,0.0f,1.0f);
        const   float3  nBack   =   float3(0.0f,0.0f,-1.0f);
        /// 平面 UV:按字体像素尺寸归一化(PBR 等带贴图的材质需要 TEXCOORD0)
        const   float   uvScale =   1.0f / (float)_pixelSize;

        const   auto    emitTri =   [&](const float3& a,const float3& b,const float3& c,
                                        const float3& n)
        {
            poss.emplace_back(a);   normals.emplace_back(n);   colors.emplace_back(color);
            poss.emplace_back(b);   normals.emplace_back(n);   colors.emplace_back(color);
            poss.emplace_back(c);   normals.emplace_back(n);   colors.emplace_back(color);
            uvs.emplace_back(float2(a.x * uvScale,a.y * uvScale));
            uvs.emplace_back(float2(b.x * uvScale,b.y * uvScale));
            uvs.emplace_back(float2(c.x * uvScale,c.y * uvScale));
        };

        float       penX    =   origin.x;
        float       penY    =   origin.y + _ascent * scale;
        const char* p       =   text.c_str();
        const char* end     =   p + text.size();
        while (p < end)
        {
            uint32  code    =   decodeUTF8(p,end);
            if (code == '\n' || code == '\r')
            {
                penX    =   origin.x;
                penY    +=  _lineHeight * scale;
                continue;
            }

            const   FEFontGlyph&    g   =   glyph(code);
            if (g._valid)
            {
                FillContours    contours;
                collectGlyphContours(face,code,penX,penY,scale,steps,contours);

                /// 前后盖:复用填充三角化,平移到 z=0 / z=-depth
                float3s     caps;
                Rgba8s      capColors;
                triangulateContours(contours,caps,capColors,color);
                for (size_t i = 0; i + 2 < caps.size(); i += 3)
                {
                    const   float2  a(caps[i].x,caps[i].y);
                    const   float2  b(caps[i + 1].x,caps[i + 1].y);
                    const   float2  c(caps[i + 2].x,caps[i + 2].y);
                    /// 前盖 (+z)
                    emitTri(float3(a.x,a.y,0.0f),float3(b.x,b.y,0.0f),
                            float3(c.x,c.y,0.0f),nFront);
                    /// 后盖 (-z),反转绕序
                    emitTri(float3(c.x,c.y,-depth),float3(b.x,b.y,-depth),
                            float3(a.x,a.y,-depth),nBack);
                }

                /// 侧壁:每条轮廓边生成一个四边形,法线取轮廓外法线
                for (const FillContour& contour : contours)
                {
                    const   size_t  n   =   contour.size();
                    if (n < 3)
                        continue;
                    const   float   sign    =   (contourArea2(contour) >= 0.0f) ? 1.0f : -1.0f;
                    for (size_t i = 0; i < n; ++i)
                    {
                        const   float2  a   =   contour[i];
                        const   float2  b   =   contour[(i + 1) % n];
                        const   float   dx  =   b.x - a.x;
                        const   float   dy  =   b.y - a.y;
                        const   float   len =   std::sqrt(dx * dx + dy * dy);
                        if (len < 1e-6f)
                            continue;
                        /// CCW 轮廓外法线为 (dy,-dx);CW 轮廓取反
                        const   float3  nrm(sign * dy / len, -sign * dx / len, 0.0f);

                        const   float3  a0(a.x,a.y,0.0f);
                        const   float3  b0(b.x,b.y,0.0f);
                        const   float3  b1(b.x,b.y,-depth);
                        const   float3  a1(a.x,a.y,-depth);
                        emitTri(a0,b0,b1,nrm);
                        emitTri(a0,b1,a1,nrm);
                    }
                }
            }
            penX    +=  g._advance * scale;
        }

        if (poss.empty())
            return  nullptr;
        Mesh    mesh    =   FEMeshBuilder::makeMesh(_ctx,poss,colors,normals,uvs);
        if (mesh != nullptr)
            mesh->updateAabb();
        return  mesh;
    }
}

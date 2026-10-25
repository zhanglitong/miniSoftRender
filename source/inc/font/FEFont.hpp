#pragma     once

#include    <vector>
#include    <cstdint>
#include    <cstddef>
#include    <unordered_map>

#include    "../FEObject.h"
#include    "../graphic/FEDevice.h"
#include    "../graphic/FETexture.h"
#include    "../mesh/FEMesh.hpp"
#include    "../mesh/FEMeshBuilder.hpp"

namespace   FE
{
    /// <summary>
    /// 单个字形在 SDF 图集中的布局与度量(烘焙像素单位)
    /// </summary>
    struct  FEFontGlyph
    {
        /// <summary>
        /// 图集 uv(左上)
        /// </summary>
        float2  _uv0         =   float2(0.0f);
        /// <summary>
        /// 图集 uv(右下)
        /// </summary>
        float2  _uv1         =   float2(0.0f);
        /// <summary>
        /// 四边形尺寸(含 SDF padding)
        /// </summary>
        float2  _size        =   float2(0.0f);
        /// <summary>
        /// 相对笔位置(baseline)的左上偏移,y 向下
        /// </summary>
        float2  _offset      =   float2(0.0f);
        /// <summary>
        /// 水平步进(像素)
        /// </summary>
        float   _advance     =   0.0f;
        /// <summary>
        /// false 表示空白字形(如空格),仅使用步进
        /// </summary>
        bool    _valid       =   false;
    };

    /// <summary>
    /// 字体烘焙参数
    /// </summary>
    struct  FEFontCreateInfo
    {
        /// <summary>
        /// 烘焙像素高度。绘制时只使用这份高分辨率距离场,
        /// 显示字号按"字号/烘焙尺寸"缩放采样。
        /// 默认 64:中文(如宋体)最细横画宽约为 em 的 1/30,
        /// 64px 时约 2 个纹素,是"细笔画在距离场中至少占 2 纹素"的下限;
        /// 再低(如 40/48)缩小采样时细笔画会被平均到阈值之下而断续
        /// </summary>
        uint    _pixelSize  =   64;
        /// <summary>
        /// SDF 扩散半径(像素,FreeType 允许 2~32),建议取 em 的 1/8 左右
        /// </summary>
        uint    _spread     =   8;
        /// <summary>
        /// 起始码点(_characters 为空时生效)
        /// </summary>
        uint    _firstChar  =   32;
        /// <summary>
        /// 结束码点(_characters 为空时生效)
        /// </summary>
        uint    _lastChar   =   126;
        /// <summary>
        /// 显式烘焙的字符集(UTF-8)。非空时忽略 _firstChar/_lastChar,
        /// 仅烘焙该串中出现的不重复码点——适合中文等码点不连续的字体
        /// </summary>
        String  _characters;
        /// <summary>
        /// SDF 图集宽度(高度按需向上取 2 的幂)
        /// </summary>
        uint    _atlasWidth =   256;
        /// <summary>
        /// 烘焙后是否将图集上传到 GPU。
        /// 关闭时仅保留 CPU 侧图集与字形度量,无需渲染设备(供单元测试)
        /// </summary>
        bool    _uploadTexture =   true;
        /// <summary>
        /// 位图模式(小字号 hinting 灰度):
        ///   启用后 bake() 用 FT_RENDER_MODE_NORMAL 渲染 hinted 8-bit 灰度位图,
        ///   R 通道直接是覆盖率(0~255),不生成距离场。
        ///   适合字号 < 16px:此时 SDF 细笔画会因双线性欠采样而断笔,
        ///   OS 级文本渲染(DirectWrite/CoreText)正是大字号轮廓/SDF、小字号 hinting 位图。
        /// </summary>
        bool    _bitmapMode    =   false;
    };

    /// <summary>
    /// FreeType 私有实现(pimpl),避免第三方类型泄漏到公开头文件
    /// </summary>
    struct  FEFontImpl;

    DEFINE_CLASS_UUID(FEFont,"{A1F6C3D2-7E48-4B95-8F21-9C3D6E5B07A4}");
    /// <summary>
    /// 距离场字体:基于 FreeType 加载 TTF,运行时用 FT_RENDER_MODE_SDF
    /// 烘焙带符号距离场图集,并提供字形度量与屏幕空间文本网格构建。
    /// SDF 编码约定:128 为字形轮廓边界,>128 位于字形内部。
    /// </summary>
    class   FE_API  FEFont:public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEFont)
    public:
        FEFont(FEContext& ctx);
        FEFont(const FEFont& other);
        virtual ~FEFont();

        /// <summary>
        /// 从字体文件加载并烘焙 SDF 图集
        /// </summary>
        bool    loadFromFile(const String& path,const FEFontCreateInfo& ci = FEFontCreateInfo());
        /// <summary>
        /// 从内存加载(数据会被内部复制,供 FreeType 使用)
        /// </summary>
        bool    loadFromMemory(const void* data,size_t bytes,const FEFontCreateInfo& ci = FEFontCreateInfo());

        inline  const   TexPtr&     texture()           const   { return _texture; }
        /// <summary>
        /// 图集已烘焙完成(CPU 侧图集/度量可用)。
        /// CPU-only 模式(_uploadTexture=false)下 texture() 为空但仍返回 true
        /// </summary>
        inline  bool                isValid()           const   { return !_atlasPixels.empty(); }
        /// <summary>
        /// 有效字形数量
        /// </summary>
        uint                        glyphCount()        const;
        /// <summary>
        /// 图集原始像素(RGBA8,SDF 存于 R 通道),供测试与调试校验
        /// </summary>
        inline  const   std::vector<uint8_t>&   atlasPixels()   const   { return _atlasPixels; }
        inline  uint                atlasWidth()        const   { return _atlasW; }
        inline  uint                atlasHeight()       const   { return _atlasH; }
        /// <summary>
        /// SDF 扩散半径(烘焙时实际生效值,FreeType 允许 2~32)。
        /// 每个烘焙像素的编码增量为 1/(2*spread),供着色器换算抗锯齿宽度
        /// </summary>
        inline  uint                sdfSpread()         const   { return _spread; }
        /// <summary>
        /// 是否为位图模式(hinting 灰度,小字号专用)
        /// </summary>
        inline  bool                isBitmap()          const   { return _bitmap; }
        /// <summary>
        /// 行高与基线(烘焙像素)
        /// </summary>
        inline  float               lineHeight()        const   { return _lineHeight; }
        inline  float               ascent()            const   { return _ascent; }
        /// <summary>
        /// 字族名(如 "SimSun"),未加载时为空
        /// </summary>
        String                      familyName()        const;

        /// <summary>
        /// 获取字形度量(未烘焙的码点返回无效字形)
        /// </summary>
        const   FEFontGlyph&        glyph(uint32 code)  const;
        /// <summary>
        /// 单行文本像素宽度
        /// </summary>
        float   measureWidth(const String& text,float scale = 1.0f) const;
        /// <summary>
        /// 构建屏幕空间文本网格:origin 为文本左上角(像素坐标,y 向下),
        /// 顶点为 pos(像素)+ uv(图集),四边形按 y 翻转后 CCW 绕序
        /// </summary>
        Mesh    createTextMesh(const String& text,const float2& origin,float scale = 1.0f) const;
        /// <summary>
        /// 从字体字形中提取矢量轮廓,构建 PRI_LINES 网格(矢量线绘制字体)。
        /// 用 FreeType FT_Outline_Decompose 取出每个字形的轮廓(move/line/conic/cubic),
        /// 贝塞尔曲线按 curveSteps 段离散为直线段。顶点带颜色,屏幕像素坐标。
        /// origin 为文本左上角(像素,y 向下),scale 为相对烘焙尺寸的缩放。
        /// </summary>
        Mesh    createTextOutlineMesh(const String& text,const float2& origin,
                                      float scale = 1.0f,uint curveSteps = 8,
                                      const Rgba8& color = Rgba8(255,255,255,255)) const;
    protected:
        /// <summary>
        /// 纯 CPU 烘焙:渲染 SDF 字形、货架打包、写入图集并生成度量
        /// </summary>
        bool    bake(const FEFontCreateInfo& ci);
        /// <summary>
        /// 将已烘焙的图集上传到 GPU(需要渲染设备)
        /// </summary>
        bool    uploadAtlas();
    protected:
        using   GlyphIndex  =   std::unordered_map<uint32,uint32>;
        using   Glyphs      =   std::vector<FEFontGlyph>;
    protected:
        TexPtr                      _texture    =   nullptr;
        FEFontImpl*                 _impl       =   nullptr;
        /// <summary>
        /// 码点 -> _glyphs 下标(支持非连续字符集)
        /// </summary>
        GlyphIndex                  _glyphIndex;
        Glyphs                      _glyphs;
        uint8s                      _atlasPixels;
        uint                        _atlasW     =   0;
        uint                        _atlasH     =   0;
        /// <summary>
        /// 烘焙像素尺寸(轮廓提取需在该尺寸下加载字形,以匹配度量单位)
        /// </summary>
        uint                        _pixelSize  =   0;
        /// <summary>
        /// 烘焙时实际生效的 SDF 扩散半径(已按 FreeType 允许区间钳制)
        /// </summary>
        uint                        _spread     =   8;
        /// <summary>
        /// 位图模式标志(hinting 灰度图集,小字号用)
        /// </summary>
        bool                        _bitmap     =   false;
        float                       _ascent     =   0.0f;
        float                       _lineHeight =   0.0f;
    };
    using   Font    =   SharedPtr<FEFont>;
}

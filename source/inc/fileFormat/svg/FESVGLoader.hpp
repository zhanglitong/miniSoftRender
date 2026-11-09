#pragma     once

#include    "FESVGPathParser.hpp"
#include    "../../xml/rapidxml.hpp"
#include    <algorithm>
#include    <cctype>
#include    <cmath>
#include    <cstdlib>
#include    <cstring>
#include    <fstream>
#include    <sstream>

namespace   FE
{
    /// <summary>
    /// SVG 解析选项
    /// </summary>
    struct  FESVGParseOptions
    {
        /// 曲线/圆弧细分数量
        uint    _curveSegments  =   16;
    };

    /// <summary>
    /// SVG 文件加载与解析器,参考 three.js SVGLoader 的实现思路:
    /// 1. 使用 XML 解析器遍历 svg 节点树;
    /// 2. 将 path/rect/circle/ellipse/line/polyline/polygon 转换为折线子路径;
    /// 3. 处理 transform 与样式继承。
    /// </summary>
    class   FESVGLoader
    {
    public:
        /// <summary>
        /// 从文件加载并解析 SVG
        /// </summary>
        /// <param name="file">文件路径</param>
        /// <param name="doc">输出文档</param>
        /// <param name="opt">解析选项</param>
        /// <returns>是否成功</returns>
        static  bool    loadFile(const String& file,FESVGDocument& doc,
                                 const FESVGParseOptions& opt = FESVGParseOptions())
        {
            std::ifstream   stream(file,std::ios::binary);
            if (!stream.is_open())
                return  false;

            std::stringstream   buffer;
            buffer << stream.rdbuf();
            return  parseText(buffer.str(),doc,opt);
        }

        /// <summary>
        /// 解析 SVG 文本
        /// </summary>
        /// <param name="text">SVG 全文</param>
        /// <param name="doc">输出文档</param>
        /// <param name="opt">解析选项</param>
        /// <returns>是否成功</returns>
        static  bool    parseText(const String& text,FESVGDocument& doc,
                                  const FESVGParseOptions& opt = FESVGParseOptions())
        {
            doc.clear();
            if (text.empty())
                return  false;

            /// rapidxml 需要可修改且以 0 结尾的缓冲区
            std::vector<char>   buffer(text.begin(),text.end());
            buffer.push_back('\0');

            rapidxml::xml_document<>    xml;
            try
            {
                xml.parse<rapidxml::parse_default>(buffer.data());
            }
            catch (...)
            {
                return  false;
            }

            rapidxml::xml_node<>*   root    =   xml.first_node("svg");
            if (root == nullptr)
            {
                for (rapidxml::xml_node<>* n = xml.first_node(); n != nullptr; n = n->next_sibling())
                {
                    if (n->type() == rapidxml::node_element && equals(n->name(),"svg"))
                    {
                        root    =   n;
                        break;
                    }
                }
            }
            if (root == nullptr)
                return  false;

            /// 记录根节点尺寸与 viewBox(供相机/缩放使用)
            if (const char* w = attr(root,"width"))
                doc.width   =   parseLeadingFloat(w);
            if (const char* h = attr(root,"height"))
                doc.height  =   parseLeadingFloat(h);
            if (const char* vb = attr(root,"viewBox"))
            {
                float   values[4]   =   {0};
                if (parseFloatList(vb,values,4) == 4)
                {
                    doc.hasViewBox  =   true;
                    doc.viewBox     =   float4(values[0],values[1],values[2],values[3]);
                }
            }

            FESVGStyle  style;
            parseNode(root,FESVGMatrix::identity(),style,doc,opt);

            /// 未显式给出宽高时退回 viewBox 尺寸
            if (doc.width <= 0.0f && doc.hasViewBox)
                doc.width   =   doc.viewBox.z;
            if (doc.height <= 0.0f && doc.hasViewBox)
                doc.height  =   doc.viewBox.w;

            return  !doc.paths.empty();
        }

    private:
        /// <summary>
        /// 该标签是否为不参与渲染的容器
        /// </summary>
        static  bool    isSkippedTag(const char* name)
        {
            static  const   char*   tags[]  =   {
                "defs","title","desc","metadata","style","script","symbol",
                "marker","pattern","mask","clipPath","linearGradient","radialGradient",
                "filter","foreignObject"
            };
            for (const char* tag : tags)
            {
                if (equals(name,tag))
                    return  true;
            }
            return  false;
        }

        /// <summary>
        /// 递归解析节点
        /// </summary>
        static  void    parseNode(rapidxml::xml_node<>* node,const FESVGMatrix& parentMatrix,
                                  const FESVGStyle& parentStyle,FESVGDocument& doc,
                                  const FESVGParseOptions& opt)
        {
            const   char*   name    =   node->name();
            if (name == nullptr || isSkippedTag(name))
                return;

            FESVGMatrix matrix  =   parentMatrix;
            if (const char* tr = attr(node,"transform"))
                matrix  =   matrix * parseTransform(tr);

            FESVGStyle  style   =   parseStyle(node,parentStyle);
            if (!style.visible)
                return;

            if (equals(name,"g") || equals(name,"svg") || equals(name,"a") || equals(name,"switch"))
            {
                for (rapidxml::xml_node<>* child = node->first_node();
                     child != nullptr; child = child->next_sibling())
                {
                    if (child->type() == rapidxml::node_element)
                        parseNode(child,matrix,style,doc,opt);
                }
                return;
            }

            std::vector<FESVGSubPath>   subPaths;
            FESVGPathParser::Options    pathOpt;
            pathOpt._curveSegments  =   opt._curveSegments;

            if (equals(name,"path"))
            {
                const   char*   d   =   attr(node,"d");
                if (d != nullptr)
                    FESVGPathParser::parse(d,matrix,pathOpt,subPaths);
            }
            else if (equals(name,"rect"))
            {
                buildRect(node,matrix,opt,subPaths);
            }
            else if (equals(name,"circle"))
            {
                const   float   cx  =   attrFloat(node,"cx",0.0f);
                const   float   cy  =   attrFloat(node,"cy",0.0f);
                const   float   r   =   attrFloat(node,"r",0.0f);
                if (r > 0.0f)
                    buildEllipse(matrix,cx,cy,r,r,opt,subPaths);
            }
            else if (equals(name,"ellipse"))
            {
                const   float   cx  =   attrFloat(node,"cx",0.0f);
                const   float   cy  =   attrFloat(node,"cy",0.0f);
                const   float   rx  =   attrFloat(node,"rx",0.0f);
                const   float   ry  =   attrFloat(node,"ry",0.0f);
                if (rx > 0.0f && ry > 0.0f)
                    buildEllipse(matrix,cx,cy,rx,ry,opt,subPaths);
            }
            else if (equals(name,"line"))
            {
                float2s     points;
                points.push_back(matrix.apply(float2(attrFloat(node,"x1",0.0f),
                                                     attrFloat(node,"y1",0.0f))));
                points.push_back(matrix.apply(float2(attrFloat(node,"x2",0.0f),
                                                     attrFloat(node,"y2",0.0f))));
                pushContour(subPaths,points,false);
            }
            else if (equals(name,"polyline") || equals(name,"polygon"))
            {
                float2s     points;
                if (const char* pts = attr(node,"points"))
                    parsePoints(pts,matrix,points);
                pushContour(subPaths,points,equals(name,"polygon"));
            }
            else
            {
                return;
            }

            if (subPaths.empty())
                return;

            FESVGPath   path;
            if (const char* id = attr(node,"id"))
                path.id     =   id;
            path.style      =   style;
            path.subPaths   =   std::move(subPaths);
            doc.paths.push_back(std::move(path));
        }

        /// <summary>
        /// 矩形(支持圆角)
        /// </summary>
        static  void    buildRect(rapidxml::xml_node<>* node,const FESVGMatrix& matrix,
                                  const FESVGParseOptions& opt,std::vector<FESVGSubPath>& out)
        {
            const   float   x   =   attrFloat(node,"x",0.0f);
            const   float   y   =   attrFloat(node,"y",0.0f);
            const   float   w   =   attrFloat(node,"width",0.0f);
            const   float   h   =   attrFloat(node,"height",0.0f);
            if (w <= 0.0f || h <= 0.0f)
                return;

            float   rx  =   attrFloat(node,"rx",-1.0f);
            float   ry  =   attrFloat(node,"ry",-1.0f);
            if (rx < 0.0f && ry < 0.0f)
            {
                rx  =   0.0f;
                ry  =   0.0f;
            }
            else if (rx < 0.0f)
            {
                rx  =   ry;
            }
            else if (ry < 0.0f)
            {
                ry  =   rx;
            }
            rx  =   std::min(rx,w * 0.5f);
            ry  =   std::min(ry,h * 0.5f);

            float2s     points;
            if (rx <= 0.0f || ry <= 0.0f)
            {
                points.push_back(matrix.apply(float2(x,y)));
                points.push_back(matrix.apply(float2(x + w,y)));
                points.push_back(matrix.apply(float2(x + w,y + h)));
                points.push_back(matrix.apply(float2(x,y + h)));
            }
            else
            {
                const   uint    perQuad =   std::max<uint>(2,(opt._curveSegments + 3) / 4);
                const   float   halfPi  =   float(PI * 0.5);
                /// 上边 -> 右上角 -> 右边 -> 右下角 -> 下边 -> 左下角 -> 左边 -> 左上角
                points.push_back(matrix.apply(float2(x + rx,y)));
                points.push_back(matrix.apply(float2(x + w - rx,y)));
                appendArc(points,matrix,x + w - rx,y + ry,rx,ry,-halfPi,0.0f,perQuad);
                points.push_back(matrix.apply(float2(x + w,y + h - ry)));
                appendArc(points,matrix,x + w - rx,y + h - ry,rx,ry,0.0f,halfPi,perQuad);
                points.push_back(matrix.apply(float2(x + rx,y + h)));
                appendArc(points,matrix,x + rx,y + h - ry,rx,ry,halfPi,float(PI),perQuad);
                points.push_back(matrix.apply(float2(x,y + ry)));
                appendArc(points,matrix,x + rx,y + ry,rx,ry,float(PI),float(PI * 1.5),perQuad);
            }
            pushContour(out,points,true);
        }

        /// <summary>
        /// 圆/椭圆
        /// </summary>
        static  void    buildEllipse(const FESVGMatrix& matrix,float cx,float cy,
                                     float rx,float ry,const FESVGParseOptions& opt,
                                     std::vector<FESVGSubPath>& out)
        {
            const   uint    segments    =   std::max<uint>(16,opt._curveSegments * 2);
            float2s     points;
            points.reserve(segments);
            for (uint i = 0; i < segments; ++i)
            {
                const   float   a   =   float(i) * float(TWO_PI) / float(segments);
                points.push_back(matrix.apply(float2(cx + rx * std::cos(a),
                                                     cy + ry * std::sin(a))));
            }
            pushContour(out,points,true);
        }

        /// <summary>
        /// 采样一段椭圆弧并追加到点集(不含起点)
        /// </summary>
        static  void    appendArc(float2s& points,const FESVGMatrix& matrix,
                                  float cx,float cy,float rx,float ry,
                                  float a0,float a1,uint segments)
        {
            for (uint i = 1; i <= segments; ++i)
            {
                const   float   t   =   float(i) / float(segments);
                const   float   a   =   a0 + (a1 - a0) * t;
                points.push_back(matrix.apply(float2(cx + rx * std::cos(a),
                                                     cy + ry * std::sin(a))));
            }
        }

        /// <summary>
        /// 追加一条闭合/开放折线
        /// </summary>
        static  void    pushContour(std::vector<FESVGSubPath>& out,float2s points,bool closed)
        {
            /// 去掉相邻重合点与首尾重合点
            float2s     clean;
            clean.reserve(points.size());
            for (const auto& p : points)
            {
                if (!clean.empty() && glm::distance2(clean.back(),p) < 1e-10f)
                    continue;
                clean.push_back(p);
            }
            if (clean.size() >= 2 && glm::distance2(clean.front(),clean.back()) < 1e-10f)
                clean.pop_back();

            if (clean.size() < 2)
                return;

            FESVGSubPath    sub;
            sub.points  =   std::move(clean);
            sub.closed  =   closed;
            out.push_back(std::move(sub));
        }

        /// <summary>
        /// 解析 points="x,y x,y ..." 属性
        /// </summary>
        static  void    parsePoints(const char* text,const FESVGMatrix& matrix,float2s& out)
        {
            const   char*   p   =   text;
            while (*p != '\0')
            {
                float   x   =   0.0f;
                float   y   =   0.0f;
                char*   end =   nullptr;
                x   =   std::strtof(p,&end);
                if (end == p)
                {
                    ++p;
                    continue;
                }
                p   =   end;
                while (*p != '\0' && !std::isdigit((unsigned char)*p)
                    && *p != '-' && *p != '+' && *p != '.')
                    ++p;
                y   =   std::strtof(p,&end);
                if (end == p)
                    break;
                p   =   end;
                out.push_back(matrix.apply(float2(x,y)));
            }
        }

        /// <summary>
        /// 解析 transform 属性
        /// </summary>
        static  FESVGMatrix    parseTransform(const char* text)
        {
            FESVGMatrix m;
            const   char*   p   =   text;
            while (*p != '\0')
            {
                while (*p != '\0' && !std::isalpha((unsigned char)*p))
                    ++p;
                if (*p == '\0')
                    break;

                char    name[16]    =   {0};
                size_t  ni          =   0;
                while (*p != '\0' && std::isalpha((unsigned char)*p) && ni < 15)
                    name[ni++]  =   *p++;
                name[ni]    =   '\0';

                while (*p != '\0' && (std::isspace((unsigned char)*p) || *p == ','))
                    ++p;
                if (*p != '(')
                    continue;
                ++p;

                float   values[6]   =   {0};
                uint    count       =   0;
                while (*p != '\0' && *p != ')' && count < 6)
                {
                    char*   end =   nullptr;
                    const   float   v   =   std::strtof(p,&end);
                    if (end == p)
                    {
                        ++p;
                        continue;
                    }
                    values[count++] =   v;
                    p   =   end;
                }
                if (*p == ')')
                    ++p;

                FESVGMatrix t;
                if (equals(name,"matrix") && count >= 6)
                {
                    t   =   FESVGMatrix{values[0],values[1],values[2],
                                        values[3],values[4],values[5]};
                }
                else if (equals(name,"translate"))
                {
                    t   =   FESVGMatrix::translate(values[0],count >= 2 ? values[1] : 0.0f);
                }
                else if (equals(name,"scale"))
                {
                    t   =   FESVGMatrix::scale(values[0],count >= 2 ? values[1] : values[0]);
                }
                else if (equals(name,"rotate"))
                {
                    t   =   (count >= 3) ? FESVGMatrix::rotate(values[0],values[1],values[2])
                                         : FESVGMatrix::rotate(values[0]);
                }
                else if (equals(name,"skewX"))
                {
                    t   =   FESVGMatrix::skewX(values[0]);
                }
                else if (equals(name,"skewY"))
                {
                    t   =   FESVGMatrix::skewY(values[0]);
                }
                else
                {
                    continue;
                }
                m   =   m * t;
            }
            return  m;
        }

        /// <summary>
        /// 解析元素样式(继承父样式,style 属性优先于表现属性)
        /// </summary>
        static  FESVGStyle  parseStyle(rapidxml::xml_node<>* node,const FESVGStyle& parent)
        {
            FESVGStyle  style   =   parent;

            /// style="key:value;..." 解析为键值对,优先级高于同名属性
            std::vector<std::pair<String,String>>   css;
            if (const char* s = attr(node,"style"))
            {
                const   char*   p   =   s;
                while (*p != '\0')
                {
                    const   char*   colon   =   std::strchr(p,':');
                    if (colon == nullptr)
                        break;
                    const   char*   semi    =   std::strchr(colon,';');
                    const   String  key     =   trim(String(p,colon - p));
                    const   String  value   =   semi != nullptr
                                            ?   trim(String(colon + 1,semi - colon - 1))
                                            :   trim(String(colon + 1));
                    css.emplace_back(key,value);
                    if (semi == nullptr)
                        break;
                    p   =   semi + 1;
                }
            }

            auto    lookup  =   [&](const char* key) -> const char*
            {
                for (const auto& kv : css)
                {
                    if (equals(kv.first.c_str(),key))
                        return  kv.second.c_str();
                }
                return  attr(node,key);
            };

            /// 可见性
            if (const char* display = lookup("display"))
            {
                if (equals(display,"none"))
                    style.visible   =   false;
            }
            if (const char* visibility = lookup("visibility"))
            {
                if (equals(visibility,"hidden") || equals(visibility,"collapse"))
                    style.visible   =   false;
            }

            /// 填充
            if (const char* fill = lookup("fill"))
            {
                float4  color;
                if (equals(fill,"none"))
                {
                    style.fill  =   false;
                }
                else if (parseColor(fill,color))
                {
                    style.fill      =   true;
                    style.fillColor =   color;
                }
            }
            /// 描边
            if (const char* stroke = lookup("stroke"))
            {
                float4  color;
                if (equals(stroke,"none"))
                {
                    style.stroke    =   false;
                }
                else if (parseColor(stroke,color))
                {
                    style.stroke        =   true;
                    style.strokeColor   =   color;
                }
            }
            if (const char* width = lookup("stroke-width"))
                style.strokeWidth   =   parseLeadingFloat(width);

            /// 不透明度(累计到 style.opacity,渲染时统一乘入 alpha)
            if (const char* opacity = lookup("opacity"))
                style.opacity   *=  clamp01(parseLeadingFloat(opacity));
            if (const char* opacity = lookup("fill-opacity"))
                style.fillColor.a   *=  clamp01(parseLeadingFloat(opacity));
            if (const char* opacity = lookup("stroke-opacity"))
                style.strokeColor.a *=  clamp01(parseLeadingFloat(opacity));

            return  style;
        }

        /// <summary>
        /// 颜色解析:支持 #rgb/#rgba/#rrggbb/#rrggbbaa、rgb()/rgba() 与常用颜色名
        /// </summary>
        static  bool    parseColor(const char* text,float4& out)
        {
            String  value   =   trim(text);
            if (value.empty())
                return  false;

            std::transform(value.begin(),value.end(),value.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });

            if (value[0] == '#')
            {
                const   String  hex =   value.substr(1);
                auto    hexVal  =   [](char c) -> int
                {
                    if (c >= '0' && c <= '9')
                        return  c - '0';
                    if (c >= 'a' && c <= 'f')
                        return  c - 'a' + 10;
                    return  -1;
                };
                if (hex.size() == 3 || hex.size() == 4)
                {
                    int r =   hexVal(hex[0]);
                    int g =   hexVal(hex[1]);
                    int b =   hexVal(hex[2]);
                    int a =   (hex.size() == 4) ? hexVal(hex[3]) : 15;
                    if (r < 0 || g < 0 || b < 0 || a < 0)
                        return  false;
                    out =   float4(r / 15.0f,g / 15.0f,b / 15.0f,a / 15.0f);
                    return  true;
                }
                if (hex.size() == 6 || hex.size() == 8)
                {
                    int     v[4]    =   {255,255,255,255};
                    const   size_t  n   =   hex.size() / 2;
                    for (size_t i = 0; i < n; ++i)
                    {
                        int hi  =   hexVal(hex[i * 2 + 0]);
                        int lo  =   hexVal(hex[i * 2 + 1]);
                        if (hi < 0 || lo < 0)
                            return  false;
                        v[i]    =   hi * 16 + lo;
                    }
                    out =   float4(v[0] / 255.0f,v[1] / 255.0f,
                                   v[2] / 255.0f,v[3] / 255.0f);
                    return  true;
                }
                return  false;
            }

            if (value.compare(0,4,"rgb(") == 0 || value.compare(0,5,"rgba(") == 0)
            {
                float   values[4]   =   {0.0f,0.0f,0.0f,1.0f};
                uint    count       =   0;
                const   char*   p   =   std::strchr(value.c_str(),'(') + 1;
                while (*p != '\0' && *p != ')' && count < 4)
                {
                    char*   end =   nullptr;
                    const   float   v   =   std::strtof(p,&end);
                    if (end == p)
                    {
                        ++p;
                        continue;
                    }
                    bool    percent =   false;
                    const   char*   q   =   end;
                    while (*q == ' ')
                        ++q;
                    if (*q == '%')
                    {
                        percent =   true;
                        end     =   (char*)(q + 1);
                    }
                    values[count++] =   percent ? v / 100.0f : v / 255.0f;
                    p   =   end;
                }
                if (count < 3)
                    return  false;
                if (count >= 4 && values[3] > 1.0f)
                    values[3]   =   1.0f;
                out =   float4(values[0],values[1],values[2],count >= 4 ? values[3] : 1.0f);
                return  true;
            }

            static  const   struct
            {
                const char* name;
                float       r,g,b;
            }   named[] =
            {
                {"black",0.0f,0.0f,0.0f},       {"white",1.0f,1.0f,1.0f},
                {"red",1.0f,0.0f,0.0f},         {"green",0.0f,0.502f,0.0f},
                {"lime",0.0f,1.0f,0.0f},        {"blue",0.0f,0.0f,1.0f},
                {"yellow",1.0f,1.0f,0.0f},      {"cyan",0.0f,1.0f,1.0f},
                {"aqua",0.0f,1.0f,1.0f},        {"magenta",1.0f,0.0f,1.0f},
                {"fuchsia",1.0f,0.0f,1.0f},     {"gray",0.502f,0.502f,0.502f},
                {"grey",0.502f,0.502f,0.502f},  {"silver",0.753f,0.753f,0.753f},
                {"orange",1.0f,0.647f,0.0f},    {"purple",0.502f,0.0f,0.502f},
                {"brown",0.647f,0.165f,0.165f}, {"pink",1.0f,0.753f,0.796f},
                {"navy",0.0f,0.0f,0.502f},      {"teal",0.0f,0.502f,0.502f},
                {"olive",0.502f,0.502f,0.0f},   {"transparent",0.0f,0.0f,0.0f}
            };
            for (const auto& c : named)
            {
                if (value == c.name)
                {
                    out =   float4(c.r,c.g,c.b,1.0f);
                    if (value == "transparent")
                        out.a   =   0.0f;
                    return  true;
                }
            }
            return  false;
        }

        /// <summary>
        /// 忽略大小写的字符串比较
        /// </summary>
        static  bool    equals(const char* a,const char* b)
        {
            if (a == nullptr || b == nullptr)
                return  a == b;
            while (*a != '\0' && *b != '\0')
            {
                if (std::tolower((unsigned char)*a) != std::tolower((unsigned char)*b))
                    return  false;
                ++a;
                ++b;
            }
            return  *a == *b;
        }
        /// <summary>
        /// 字符串去空白
        /// </summary>
        static  String  trim(const String& text)
        {
            size_t  begin   =   0;
            size_t  end     =   text.size();
            while (begin < end && std::isspace((unsigned char)text[begin]))
                ++begin;
            while (end > begin && std::isspace((unsigned char)text[end - 1]))
                --end;
            return  text.substr(begin,end - begin);
        }
        /// <summary>
        /// 解析前导浮点数(忽略 px/% 等单位)
        /// </summary>
        static  float   parseLeadingFloat(const char* text)
        {
            if (text == nullptr)
                return  0.0f;
            return  std::strtof(text,nullptr);
        }
        /// <summary>
        /// 解析浮点数组
        /// </summary>
        static  uint    parseFloatList(const char* text,float* out,uint maxCount)
        {
            uint            count   =   0;
            const   char*   p       =   text;
            while (*p != '\0' && count < maxCount)
            {
                char*   end =   nullptr;
                const   float   v   =   std::strtof(p,&end);
                if (end == p)
                {
                    ++p;
                    continue;
                }
                out[count++]    =   v;
                p               =   end;
            }
            return  count;
        }
        /// <summary>
        /// 限定到 [0,1]
        /// </summary>
        static  float   clamp01(float v)
        {
            return  std::max(0.0f,std::min(1.0f,v));
        }
        /// <summary>
        /// 读取属性
        /// </summary>
        static  const   char*   attr(rapidxml::xml_node<>* node,const char* name)
        {
            rapidxml::xml_attribute<>*  a   =   node->first_attribute(name);
            return  a != nullptr ? a->value() : nullptr;
        }
        /// <summary>
        /// 读取浮点属性
        /// </summary>
        static  float   attrFloat(rapidxml::xml_node<>* node,const char* name,float def)
        {
            const   char*   value   =   attr(node,name);
            return  value != nullptr ? parseLeadingFloat(value) : def;
        }
    };
}

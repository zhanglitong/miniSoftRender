#pragma     once

#include    "FESVGTypes.hpp"
#include    <algorithm>
#include    <cctype>
#include    <cmath>
#include    <cstdlib>
#include    <cstring>

namespace   FE
{
    /// <summary>
    /// SVG path 的 d 属性解析器,移植自 three.js SVGLoader 的路径命令处理,
    /// 支持 M/L/H/V/C/S/Q/T/A/Z(大小写即绝对/相对)全部命令。
    /// 曲线与圆弧在解析阶段细分后转为折线点。
    /// </summary>
    class   FESVGPathParser
    {
    public:
        /// <summary>
        /// 解析选项
        /// </summary>
        struct  Options
        {
            /// 曲线/圆弧细分数(越大越平滑)
            uint    _curveSegments  =   16;
        };
        /// <summary>
        /// 路径 token
        /// </summary>
        struct  Token
        {
            bool    isCommand   =   false;
            char    command     =   0;
            float   value       =   0.0f;
        };

        /// <summary>
        /// 解析 path 的 d 字符串
        /// </summary>
        /// <param name="d">path data</param>
        /// <param name="matrix">当前累计变换矩阵</param>
        /// <param name="opt">解析选项</param>
        /// <param name="out">输出子路径(追加)</param>
        static  void    parse(const char* d,const FESVGMatrix& matrix,
                              const Options& opt,std::vector<FESVGSubPath>& out)
        {
            if (d == nullptr)
                return;

            std::vector<Token>  tokens;
            tokenize(d,tokens);
            if (tokens.empty())
                return;

            float2  cur     =   float2(0.0f);
            float2  start   =   float2(0.0f);
            float2  ctrlC   =   float2(0.0f);
            float2  ctrlQ   =   float2(0.0f);
            char    prevCmd =   0;
            char    cmd     =   0;
            size_t  subIdx  =   (size_t)-1;

            /// 开始新的子路径,并把当前点作为起点
            auto    beginSub    =   [&](const float2& p)
            {
                out.emplace_back();
                subIdx          =   out.size() - 1;
                out[subIdx].closed  =   false;
                addPoint(out[subIdx].points,matrix.apply(p));
            };
            /// 当前子路径不存在时补一个(容错:路径直接以画线命令开始)
            auto    ensureSub   =   [&]()
            {
                if (subIdx == (size_t)-1 || subIdx >= out.size())
                    beginSub(cur);
            };
            /// 追加一个点(去重)
            auto    lineTo      =   [&](const float2& p)
            {
                ensureSub();
                addPoint(out[subIdx].points,matrix.apply(p));
            };
            /// 三次贝塞尔细分
            auto    cubicTo     =   [&](const float2& p0,const float2& p1,
                                        const float2& p2,const float2& p3)
            {
                ensureSub();
                const   uint    segs    =   std::max<uint>(2,opt._curveSegments);
                for (uint s = 1; s <= segs; ++s)
                {
                    const   float   t   =   float(s) / float(segs);
                    const   float   it  =   1.0f - t;
                    const   float2  p   =   p0 * (it * it * it)
                                        +   p1 * (3.0f * it * it * t)
                                        +   p2 * (3.0f * it * t * t)
                                        +   p3 * (t * t * t);
                    addPoint(out[subIdx].points,matrix.apply(p));
                }
            };
            /// 二次贝塞尔细分
            auto    quadTo      =   [&](const float2& p0,const float2& p1,const float2& p2)
            {
                ensureSub();
                const   uint    segs    =   std::max<uint>(2,opt._curveSegments);
                for (uint s = 1; s <= segs; ++s)
                {
                    const   float   t   =   float(s) / float(segs);
                    const   float   it  =   1.0f - t;
                    const   float2  p   =   p0 * (it * it)
                                        +   p1 * (2.0f * it * t)
                                        +   p2 * (t * t);
                    addPoint(out[subIdx].points,matrix.apply(p));
                }
            };
            /// 椭圆弧细分(SVG 规范 endpoint -> center 参数化)
            auto    arcTo       =   [&](const float2& from,float rx,float ry,
                                        float xAxisRotation,bool largeArc,bool sweep,
                                        const float2& to)
            {
                rx  =   std::fabs(rx);
                ry  =   std::fabs(ry);
                if (rx < 1e-6f || ry < 1e-6f)
                {
                    lineTo(to);
                    return;
                }

                const   float   phi     =   float(xAxisRotation * PI / 180.0);
                const   float   cosPhi  =   std::cos(phi);
                const   float   sinPhi  =   std::sin(phi);

                const   float   dx2     =   (from.x - to.x) * 0.5f;
                const   float   dy2     =   (from.y - to.y) * 0.5f;
                const   float   x1p     =   cosPhi * dx2 + sinPhi * dy2;
                const   float   y1p     =   -sinPhi * dx2 + cosPhi * dy2;

                float   rx2 =   rx * rx;
                float   ry2 =   ry * ry;
                const   float   x1p2    =   x1p * x1p;
                const   float   y1p2    =   y1p * y1p;

                /// 半径过小时按规范等比放大
                const   float   lambda  =   x1p2 / rx2 + y1p2 / ry2;
                if (lambda > 1.0f)
                {
                    const   float   k   =   std::sqrt(lambda);
                    rx  *=  k;
                    ry  *=  k;
                    rx2 =   rx * rx;
                    ry2 =   ry * ry;
                }

                float   denom   =   rx2 * y1p2 + ry2 * x1p2;
                float   numer   =   rx2 * ry2 - denom;
                if (numer < 0.0f)
                    numer   =   0.0f;
                float   coef    =   (largeArc != sweep) ? 1.0f : -1.0f;
                if (denom < 1e-12f)
                {
                    lineTo(to);
                    return;
                }
                coef    *=  std::sqrt(numer / denom);

                const   float   cxp =   coef * rx * y1p / ry;
                const   float   cyp =   -coef * ry * x1p / rx;

                const   float   cx  =   cosPhi * cxp - sinPhi * cyp + (from.x + to.x) * 0.5f;
                const   float   cy  =   sinPhi * cxp + cosPhi * cyp + (from.y + to.y) * 0.5f;

                /// 起止角与角跨度
                const   float   ux  =   (x1p - cxp) / rx;
                const   float   uy  =   (y1p - cyp) / ry;
                const   float   vx  =   (-x1p - cxp) / rx;
                const   float   vy  =   (-y1p - cyp) / ry;
                const   float   theta1  =   std::atan2(uy,ux);
                float   delta       =   std::atan2(ux * vy - uy * vx,ux * vx + uy * vy);
                if (!sweep && delta > 0.0f)
                    delta   -=  2.0f * float(PI);
                else if (sweep && delta < 0.0f)
                    delta   +=  2.0f * float(PI);

                const   uint    perQuad =   std::max<uint>(2,(opt._curveSegments + 3) / 4);
                const   uint    segs    =   std::max<uint>(2,
                        (uint)std::ceil(std::fabs(delta) / float(PI * 0.5) * float(perQuad)));
                for (uint s = 1; s <= segs; ++s)
                {
                    const   float   theta   =   theta1 + delta * (float(s) / float(segs));
                    const   float   ct      =   std::cos(theta);
                    const   float   st      =   std::sin(theta);
                    const   float2  p       =   float2(cosPhi * rx * ct - sinPhi * ry * st + cx,
                                                       sinPhi * rx * ct + cosPhi * ry * st + cy);
                    addPoint(out[subIdx].points,matrix.apply(p));
                }
            };

            size_t  i   =   0;
            while (i < tokens.size())
            {
                if (tokens[i].isCommand)
                {
                    cmd     =   tokens[i].command;
                    ++i;
                }
                else if (cmd == 0)
                {
                    ++i;
                    continue;
                }

                const   bool    relative    =   (cmd >= 'a' && cmd <= 'z');
                const   char    upper       =   (char)std::toupper((unsigned char)cmd);

                if (upper == 'Z')
                {
                    if (subIdx != (size_t)-1 && subIdx < out.size())
                    {
                        auto&   sub =   out[subIdx];
                        /// 去掉与起点重合的尾点
                        if (sub.points.size() >= 2
                         && glm::distance2(sub.points.front(),sub.points.back()) < 1e-12f)
                        {
                            sub.points.pop_back();
                        }
                        if (sub.points.size() >= 2)
                            sub.closed  =   true;
                    }
                    cur     =   start;
                    cmd     =   0;
                    continue;
                }

                float   v[7]    =   {0};
                const   uint    need    =   paramCount(upper);
                if (!takeNumbers(tokens,i,v,need))
                {
                    cmd =   0;
                    ++i;
                    continue;
                }

                switch (upper)
                {
                case 'M':
                    {
                        const   float2  p   =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        beginSub(p);
                        cur     =   p;
                        start   =   p;
                        /// 后续坐标对按 lineto 处理
                        cmd     =   relative ? 'l' : 'L';
                    }
                    break;
                case 'L':
                    {
                        const   float2  p   =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        lineTo(p);
                        cur     =   p;
                    }
                    break;
                case 'H':
                    {
                        const   float   x   =   relative ? cur.x + v[0] : v[0];
                        const   float2  p   =   float2(x,cur.y);
                        lineTo(p);
                        cur     =   p;
                    }
                    break;
                case 'V':
                    {
                        const   float   y   =   relative ? cur.y + v[0] : v[0];
                        const   float2  p   =   float2(cur.x,y);
                        lineTo(p);
                        cur     =   p;
                    }
                    break;
                case 'C':
                    {
                        const   float2  p1  =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        const   float2  p2  =   relative ? cur + float2(v[2],v[3])
                                                         : float2(v[2],v[3]);
                        const   float2  p3  =   relative ? cur + float2(v[4],v[5])
                                                         : float2(v[4],v[5]);
                        cubicTo(cur,p1,p2,p3);
                        ctrlC   =   p2;
                        cur     =   p3;
                    }
                    break;
                case 'S':
                    {
                        /// 反射上一段三次贝塞尔控制点
                        const   bool    prevCubic   =   (prevCmd == 'C' || prevCmd == 'c'
                                                     || prevCmd == 'S' || prevCmd == 's');
                        const   float2  p1  =   prevCubic ? cur * 2.0f - ctrlC : cur;
                        const   float2  p2  =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        const   float2  p3  =   relative ? cur + float2(v[2],v[3])
                                                         : float2(v[2],v[3]);
                        cubicTo(cur,p1,p2,p3);
                        ctrlC   =   p2;
                        cur     =   p3;
                    }
                    break;
                case 'Q':
                    {
                        const   float2  p1  =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        const   float2  p2  =   relative ? cur + float2(v[2],v[3])
                                                         : float2(v[2],v[3]);
                        quadTo(cur,p1,p2);
                        ctrlQ   =   p1;
                        cur     =   p2;
                    }
                    break;
                case 'T':
                    {
                        /// 反射上一段二次贝塞尔控制点
                        const   bool    prevQuad    =   (prevCmd == 'Q' || prevCmd == 'q'
                                                     || prevCmd == 'T' || prevCmd == 't');
                        const   float2  p1  =   prevQuad ? cur * 2.0f - ctrlQ : cur;
                        const   float2  p2  =   relative ? cur + float2(v[0],v[1])
                                                         : float2(v[0],v[1]);
                        quadTo(cur,p1,p2);
                        ctrlQ   =   p1;
                        cur     =   p2;
                    }
                    break;
                case 'A':
                    {
                        const   float2  p   =   relative ? cur + float2(v[5],v[6])
                                                         : float2(v[5],v[6]);
                        arcTo(cur,v[0],v[1],v[2],v[3] != 0.0f,v[4] != 0.0f,p);
                        cur     =   p;
                    }
                    break;
                default:
                    cmd =   0;
                    break;
                }
                prevCmd =   cmd;
            }
        }

    private:
        /// <summary>
        /// 命令所需参数个数
        /// </summary>
        static  uint    paramCount(char cmd)
        {
            switch (cmd)
            {
            case 'M':   return  2;
            case 'L':   return  2;
            case 'H':   return  1;
            case 'V':   return  1;
            case 'C':   return  6;
            case 'S':   return  4;
            case 'Q':   return  4;
            case 'T':   return  2;
            case 'A':   return  7;
            default:    return  0;
            }
        }
        /// <summary>
        /// 从 token 流读取 need 个数字(遇到命令 token 视为失败)
        /// </summary>
        static  bool    takeNumbers(const std::vector<Token>& tokens,size_t& i,
                                    float* out,uint need)
        {
            size_t  k   =   i;
            for (uint n = 0; n < need; ++n)
            {
                if (k >= tokens.size() || tokens[k].isCommand)
                    return  false;
                out[n]  =   tokens[k].value;
                ++k;
            }
            i   =   k;
            return  true;
        }
        /// <summary>
        /// 追加点,过滤重合点
        /// </summary>
        static  void    addPoint(float2s& points,const float2& p)
        {
            if (!points.empty() && glm::distance2(points.back(),p) < 1e-12f)
                return;
            points.push_back(p);
        }
        /// <summary>
        /// 字符串 -> token 流
        /// </summary>
        static  void    tokenize(const char* text,std::vector<Token>& tokens)
        {
            const   char*   p   =   text;
            while (*p != '\0')
            {
                /// 跳过空白与逗号
                while (*p != '\0' && (std::isspace((unsigned char)*p) || *p == ','))
                    ++p;
                if (*p == '\0')
                    break;

                const   char    ch  =   *p;
                if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
                {
                    Token   t;
                    t.isCommand =   true;
                    t.command   =   ch;
                    tokens.push_back(t);
                    ++p;
                    continue;
                }

                /// 数字:符号/整数/小数/指数
                char*   end =   nullptr;
                const   float   value   =   std::strtof(p,&end);
                if (end == p)
                {
                    /// 无法识别的字符直接跳过,避免死循环
                    ++p;
                    continue;
                }
                Token   t;
                t.isCommand =   false;
                t.value     =   value;
                tokens.push_back(t);
                p   =   end;
            }
        }
    };
}

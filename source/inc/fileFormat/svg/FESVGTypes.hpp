#pragma     once

#include    "../../FEMath.hpp"
#include    "../../FEDefine.h"
#include    <cmath>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// SVG 2D 仿射矩阵,对应 SVG matrix(a,b,c,d,e,f) 的 3x3 形式:
    /// | a c e |
    /// | b d f |
    /// | 0 0 1 |
    /// 点按照列向量右乘:M * p
    /// </summary>
    struct  FESVGMatrix
    {
        float   a   =   1.0f;
        float   b   =   0.0f;
        float   c   =   0.0f;
        float   d   =   1.0f;
        float   e   =   0.0f;
        float   f   =   0.0f;

        /// <summary>
        /// 将矩阵作用到二维点
        /// </summary>
        /// <param name="p">输入点</param>
        /// <returns>变换后的点</returns>
        inline  float2  apply(const float2& p) const
        {
            return  float2(a * p.x + c * p.y + e,
                           b * p.x + d * p.y + f);
        }

        /// <summary>
        /// 矩阵乘法:先应用 other,再应用 this,即 (this * other) * p == this * (other * p)
        /// </summary>
        /// <param name="other">右乘矩阵</param>
        /// <returns>组合矩阵</returns>
        inline  FESVGMatrix operator*(const FESVGMatrix& other) const
        {
            FESVGMatrix r;
            r.a =   a * other.a + c * other.b;
            r.b =   b * other.a + d * other.b;
            r.c =   a * other.c + c * other.d;
            r.d =   b * other.c + d * other.d;
            r.e =   a * other.e + c * other.f + e;
            r.f =   b * other.e + d * other.f + f;
            return  r;
        }

        /// <summary>
        /// 单位矩阵
        /// </summary>
        static  inline  FESVGMatrix    identity()
        {
            return  FESVGMatrix{};
        }
        /// <summary>
        /// 平移矩阵
        /// </summary>
        static  inline  FESVGMatrix    translate(float tx,float ty)
        {
            FESVGMatrix r;
            r.e =   tx;
            r.f =   ty;
            return  r;
        }
        /// <summary>
        /// 缩放矩阵
        /// </summary>
        static  inline  FESVGMatrix    scale(float sx,float sy)
        {
            FESVGMatrix r;
            r.a =   sx;
            r.d =   sy;
            return  r;
        }
        /// <summary>
        /// 旋转矩阵(角度制),可指定旋转中心
        /// </summary>
        static  inline  FESVGMatrix    rotate(float degree,float cx = 0.0f,float cy = 0.0f)
        {
            const   float   rad =   float(degree * PI / 180.0);
            const   float   cs  =   std::cos(rad);
            const   float   sn  =   std::sin(rad);
            FESVGMatrix r;
            r.a =   cs;
            r.b =   sn;
            r.c =   -sn;
            r.d =   cs;
            r.e =   cx - cs * cx + sn * cy;
            r.f =   cy - sn * cx - cs * cy;
            return  r;
        }
        /// <summary>
        /// 沿 X 轴斜切
        /// </summary>
        static  inline  FESVGMatrix    skewX(float degree)
        {
            FESVGMatrix r;
            r.c =   std::tan(float(degree * PI / 180.0));
            return  r;
        }
        /// <summary>
        /// 沿 Y 轴斜切
        /// </summary>
        static  inline  FESVGMatrix    skewY(float degree)
        {
            FESVGMatrix r;
            r.b =   std::tan(float(degree * PI / 180.0));
            return  r;
        }
    };

    /// <summary>
    /// SVG 填充/描边样式(已继承父节点并解析完成)
    /// </summary>
    struct  FESVGStyle
    {
        /// 是否填充
        bool        fill            =   true;
        /// 是否描边
        bool        stroke          =   false;
        /// 填充颜色(线性 RGBA)
        float4      fillColor       =   float4(0.0f,0.0f,0.0f,1.0f);
        /// 描边颜色(线性 RGBA)
        float4      strokeColor     =   float4(0.0f,0.0f,0.0f,1.0f);
        /// 描边宽度(用户单位)
        float       strokeWidth     =   1.0f;
        /// 整体不透明度(已乘入颜色 alpha 之外,用于组继承)
        float       opacity         =   1.0f;
        /// 是否可见(display:none / visibility:hidden)
        bool        visible         =   true;
    };

    /// <summary>
    /// 一条子路径(已按曲线细分并应用变换,首尾不重复)
    /// </summary>
    struct  FESVGSubPath
    {
        /// 折线点(已经过 CTM 变换)
        float2s     points;
        /// 是否闭合(对应 Z 或 polygon/circle/rect 等)
        bool        closed  =   false;
    };

    /// <summary>
    /// 一个 SVG 图形(对应 three.js SVGLoader 的一个 path)
    /// 可能包含多条子路径,用于表达洞与嵌套
    /// </summary>
    struct  FESVGPath
    {
        /// 元素 id
        String      id;
        /// 子路径集合
        std::vector<FESVGSubPath>   subPaths;
        /// 填充/描边样式
        FESVGStyle  style;
    };

    /// <summary>
    /// SVG 文档解析结果
    /// </summary>
    struct  FESVGDocument
    {
        /// svg 根节点宽度(用户单位,0 表示未指定)
        float       width       =   0.0f;
        /// svg 根节点高度(用户单位,0 表示未指定)
        float       height      =   0.0f;
        /// 是否包含 viewBox
        bool        hasViewBox  =   false;
        /// viewBox: x,y,width,height
        float4      viewBox     =   float4(0.0f,0.0f,0.0f,0.0f);
        /// 按文档顺序排列的图形
        std::vector<FESVGPath>  paths;

        /// <summary>
        /// 清空数据
        /// </summary>
        inline  void    clear()
        {
            width       =   0.0f;
            height      =   0.0f;
            hasViewBox  =   false;
            viewBox     =   float4(0.0f,0.0f,0.0f,0.0f);
            paths.clear();
        }

        /// <summary>
        /// 统计所有子路径点的包围盒
        /// </summary>
        /// <param name="minP">输出最小值</param>
        /// <param name="maxP">输出最大值</param>
        /// <returns>是否存在有效点</returns>
        inline  bool    bounds(float2& minP,float2& maxP) const
        {
            bool    valid   =   false;
            for (const auto& path : paths)
            {
                for (const auto& sub : path.subPaths)
                {
                    for (const auto& p : sub.points)
                    {
                        if (!valid)
                        {
                            minP    =   p;
                            maxP    =   p;
                            valid   =   true;
                        }
                        else
                        {
                            minP    =   glm::min(minP,p);
                            maxP    =   glm::max(maxP,p);
                        }
                    }
                }
            }
            return  valid;
        }
    };

    /// <summary>
    /// 多边形有向面积(正为逆时针,坐标系为数学 y 向上)
    /// </summary>
    /// <param name="points">闭合多边形点集(首尾不重复)</param>
    /// <returns>面积的两倍</returns>
    inline  float   fesvgSignedArea2(const float2s& points)
    {
        const   size_t  n   =   points.size();
        if (n < 3)
            return  0.0f;

        float   sum =   0.0f;
        for (size_t i = 0,j = n - 1; i < n; j = i++)
        {
            sum +=  points[j].x * points[i].y - points[i].x * points[j].y;
        }
        return  sum;
    }

    /// <summary>
    /// 判断点是否在多边形内部(射线法,支持非凸)
    /// </summary>
    /// <param name="p">待判断点</param>
    /// <param name="points">闭合多边形点集(首尾不重复)</param>
    /// <returns>是否在内部</returns>
    inline  bool    fesvgPointInPolygon(const float2& p,const float2s& points)
    {
        const   size_t  n   =   points.size();
        if (n < 3)
            return  false;

        bool    inside  =   false;
        for (size_t i = 0,j = n - 1; i < n; j = i++)
        {
            const   float2& a   =   points[i];
            const   float2& b   =   points[j];
            if (((a.y > p.y) != (b.y > p.y)) &&
                (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x))
            {
                inside  =   !inside;
            }
        }
        return  inside;
    }
}

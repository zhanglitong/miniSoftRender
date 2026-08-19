#pragma     once

#include    "../../FEObject.h"
#include    "../../FEColor.hpp"

namespace   FE
{
    struct  FEParamGrid
    {
        /// <summary>
        /// 网格的总边长
        /// </summary>
        uint32_t    _size;
        /// <summary>
        /// 网格的等分数(横纵各划分为多少格)
        /// </summary>
        uint32_t    _divs;
        /// <summary>
        /// 中心线的颜色
        /// </summary>
        Rgba8        _color1;
        Rgba8        _color2;
        Rgba8        _color3;
        /// <summary>
        /// 普通网格线的颜色
        /// </summary>
        Rgba8       _color4;
        int         _dir    =   0;
    };

    class   FEGridImpl
    {
    public:
        float3s     vertices;
        Rgba8s      colors;

    public:
        template<bool bColor>
        static  void    build(const FEParamGrid& param,FEGridImpl& result)
        {
            if (param._dir == 0)
            {
                result.vertices    =   extractXYLines(param);
                if constexpr (bColor)
                    result.colors  =   extractXYColor(param);
            }
            else if (param._dir == 1)
            {
                result.vertices    =   extractXZLines(param);
                if constexpr (bColor)
                    result.colors  =   extractXZColor(param);
            }
        }   

        /// <summary>
        /// 获取线段数据，两个点组成一个线段
        /// </summary>
        /// <returns></returns>
        static  float3s     extractXYLines(const FEParamGrid& param)
        {
            const auto  step    =   (float)(param._size) / (float)(param._divs);
            const auto  halfSize=   (float)(param._size) * 0.5f;
            float       k       =   -halfSize;
            uint32_t    nCnt    =   (param._divs + 1) * 4;
            float3s     verts(nCnt);
       
            for (uint32_t i = 0; i <= param._divs; i++)
            {
                verts[i*4 + 0]      =   {-halfSize, k,   0};
                verts[i*4 + 1]      =   {+halfSize, k,   0};
                                                     
                verts[i*4 + 2]      =   {k, -halfSize,   0};
                verts[i*4 + 3]      =   {k, +halfSize,   0};

                k += step;
            }
            return  verts;
        }

        static  float3s     extractXZLines(const FEParamGrid& param)
        {
            const auto  step    =   (float)(param._size) / (float)(param._divs);
            const auto  halfSize=   (float)(param._size) * 0.5f;
            float       k       =   -halfSize;
            uint32_t    nCnt    =   (param._divs + 1) * 4;
            float3s     verts(nCnt);

            for (uint32_t i = 0; i <= param._divs; i++)
            {
                verts[i*4 + 0]      =   {-halfSize, 0,  k};
                verts[i*4 + 1]      =   {+halfSize, 0,  k};

                verts[i*4 + 2]      =   {k,  0, -halfSize};
                verts[i*4 + 3]      =   {k,  0, +halfSize};

                k += step;
            }
            return  verts;
        }

        static  Rgba8s      extractXYColor(const FEParamGrid& param)
        {
            const auto  center  =   param._divs / 2;
            uint32_t    nCnt    =   (param._divs + 1) * 4;
            Rgba8s      colors(nCnt);
            for (uint32_t i = 0; i <= param._divs; i++)
            {
                if (i == center)
                {
                    colors[i*4 + 0]     =   param._color1;
                    colors[i*4 + 1]     =   param._color1;

                    colors[i*4 + 2]     =   param._color2;
                    colors[i*4 + 3]     =   param._color2;
                }
                else
                {
                    colors[i*4 + 0]     =   param._color4;
                    colors[i*4 + 1]     =   param._color4;

                    colors[i*4 + 2]     =   param._color4;
                    colors[i*4 + 3]     =   param._color4;
                }
            }
            return  colors;
        }

        static  Rgba8s      extractXZColor(const FEParamGrid& param)
        {
            const auto  center  =   param._divs / 2;
            uint32_t    nCnt    =   (param._divs + 1) * 4;
            Rgba8s      colors(nCnt);
            for (uint32_t i = 0; i <= param._divs; i++)
            {
                if (i == center)
                {
                    colors[i*4 + 0]     =   param._color1;
                    colors[i*4 + 1]     =   param._color1;

                    colors[i*4 + 2]     =   param._color3;
                    colors[i*4 + 3]     =   param._color3;
                }
                else
                {
                    colors[i*4 + 0]     =   param._color4;
                    colors[i*4 + 1]     =   param._color4;

                    colors[i*4 + 2]     =   param._color4;
                    colors[i*4 + 3]     =   param._color4;
                }
            }
            return  colors;
        }
    };
}

#pragma     once

#include    <algorithm>
#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{

    struct  FEA2b10g10r10Snorm
    {
        union
        {
            struct
            {
                int32_t _r : 10;  // Red   分量，有符号，范围 -512 ~ 511
                int32_t _g : 10;  // Green 分量，有符号，范围 -512 ~ 511
                int32_t _b : 10;  // Blue  分量，有符号，范围 -512 ~ 511
                int32_t _a : 2;   // Alpha 分量，有符号，范围 -2 ~ 1
            };
            int32_t _value;
        };
        
        FEA2b10g10r10Snorm(int32_t value = 0)
        {
            _value  = value;
        }
        /// <summary>
        /// 由归一化浮点构造: r/g/b 域 [-1,1] 线性映射到 [-511,511], 与 red()*oneOver511F 解码互逆
        /// </summary>
        /// <param name="x">-1~+1</param>
        /// <param name="y">-1~+1</param>
        /// <param name="z">-1~+1</param>
        /// <param name="w">-2 ~ 1</param>
        explicit FEA2b10g10r10Snorm(float x,float y,float z,float w)
        {
            _r   =   int(std::lround((std::min)((std::max)(x,-1.0f),1.0f) * 511.0f));
            _g   =   int(std::lround((std::min)((std::max)(y,-1.0f),1.0f) * 511.0f));
            _b   =   int(std::lround((std::min)((std::max)(z,-1.0f),1.0f) * 511.0f));
            _a   =   int(std::lround((std::min)((std::max)(w,-2.0f),1.0f)));
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="r">-512~511</param>
        /// <param name="g">-512~511</param>
        /// <param name="b">-512~511</param>
        /// <param name="a">0,1,2,3</param>
        explicit FEA2b10g10r10Snorm(int r,int g,int b,int a)
        {
            _r   =   r;
            _g   =   g;
            _b   =   b;
            _a   =   a;
        }
        int     red()   const   {   return  _r; }
        int     green() const   {   return  _g; }
        int     blue()  const   {   return  _b; }
        int     alpha() const   {   return  _a; }

        operator    int32_t() const
        {
            return  _value;
        }
    };

}


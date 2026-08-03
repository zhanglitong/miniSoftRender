#pragma     once

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
                int32_t _r : 10;  // Red   ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -512 ~ 511
                int32_t _g : 10;  // Green ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -512 ~ 511
                int32_t _b : 10;  // Blue  ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -512 ~ 511
                int32_t _a : 2;   // Alpha ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -2 ~ 1
            };
            int32_t _value;
        };
        
        FEA2b10g10r10Snorm(int32_t value = 0)
        {
            _value  = value;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="x">-1~+1</param>
        /// <param name="y">-1~+1</param>
        /// <param name="z">-1~+1</param>
        /// <param name="w">-2 ~ 1</param>
        explicit FEA2b10g10r10Snorm(float x,float y,float z,float w)
        {
            _r   =   int((x + 1) * 1023.0f - 512.0f);
            _g   =   int((y + 1) * 1023.0f - 512.0f);
            _b   =   int((z + 1) * 1023.0f - 512.0f);
            _a   =   int(w);
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


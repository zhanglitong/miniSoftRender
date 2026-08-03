#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{

    struct  FEA2b10g10r10Unorm
    {
        union
        {
            struct
            {
                uint32_t _r : 10;  // Red   ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ 0~1023
                uint32_t _g : 10;  // Green ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ 0~1023
                uint32_t _b : 10;  // Blue  ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ 0~1023
                uint32_t _a : 2;   // Alpha ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ 0~3
            };
            uint32_t _value;
        };
        
        FEA2b10g10r10Unorm(uint32_t value = 0)
        {
            _value  = value;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="x">0~+1</param>
        /// <param name="y">0~+1</param>
        /// <param name="z">0~+1</param>
        /// <param name="w">0~3</param>
        explicit FEA2b10g10r10Unorm(float x,float y,float z,float w)
        {
            _r   =   uint32_t(x * 1023.0f);
            _g   =   uint32_t(y * 1023.0f);
            _b   =   uint32_t(z * 1023.0f);
            _a   =   uint32_t(w);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="r">0~1023</param>
        /// <param name="g">0~1023</param>
        /// <param name="b">0~1023</param>
        /// <param name="a">0,1,2,3</param>
        explicit FEA2b10g10r10Unorm(uint32_t r,uint32_t g,uint32_t b,uint32_t a)
        {
            _r   =   r;
            _g   =   g;
            _b   =   b;
            _a   =   a;
        }
        uint32_t    red()   const   {   return  _r; }
        uint32_t    green() const   {   return  _g; }
        uint32_t    blue()  const   {   return  _b; }
        uint32_t    alpha() const   {   return  _a; }
        operator    uint32_t() const
        {
            return  _value;
        }
    };

}


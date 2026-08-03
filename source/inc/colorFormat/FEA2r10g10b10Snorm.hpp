#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct  FEA2r10g10b10Snorm
    {
        union
        {
            struct
            {
                int32_t _b : 10;  // Blue  ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -511 ~ 511
                int32_t _g : 10;  // Green ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -511 ~ 511
                int32_t _r : 10;  // Red   ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -511 ~ 511
                int32_t _a : 2;   // Alpha ·ÖÁ¿£¬ÓÐ·ûºÅ£¬·¶Î§ -2 ~ 1
            };
            int32_t _value;
        };

        FEA2r10g10b10Snorm(int32_t value = 0)
        {
            _value  =   value;
        }
        explicit FEA2r10g10b10Snorm(float x,float y,float z,float w = 0)
        {
            _r   =   int(x * 511.0f);
            _g   =   int(y * 511.0f);
            _b   =   int(z * 511.0f);
            _a   =   int(w);
        }
        operator    int32_t() const
        {
            return  _value;
        }
        operator    int32_t&()
        {
            return  _value;
        }
    };

}


#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct  FEA2r10g10b10Unorm
    {
        union
        {
            struct
            {
                uint32_t _b : 10;  // Blue  分量，有符号，范围 0 ~ 1023
                uint32_t _g : 10;  // Green 分量，有符号，范围 0 ~ 1023
                uint32_t _r : 10;  // Red   分量，有符号，范围 0 ~ 1023
                uint32_t _a : 2;   // Alpha 分量，有符号，范围 0 ~ 3
            };
            uint32_t _value;
        };

        FEA2r10g10b10Unorm(int32_t value = 0)
        {
            _value  =   value;
        }
        explicit FEA2r10g10b10Unorm(float x,float y,float z,float w = 0)
        {
            _r   =   uint32_t(x * 1023.0f);
            _g   =   uint32_t(y * 1023.0f);
            _b   =   uint32_t(z * 1023.0f);
            _a   =   uint32_t(w);
        }
        operator    uint32_t() const
        {
            return  _value;
        }
        operator    uint32_t&()
        {
            return  _value;
        }
    };

}


#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct  FER9g9b9a5Unorm
    {
        union
        {
            struct
            {
                uint32_t    _a : 5;
                uint32_t    _b : 9;
                uint32_t    _g : 9;
                uint32_t    _r : 9;
            } ;
            uint32_t        _value;
        };
        
       
        FER9g9b9a5Unorm(uint32_t value = 0)
        {
            _value = value;

        }
        explicit FER9g9b9a5Unorm(uint32_t x,uint32_t y,uint32_t z,uint32_t w)
        {
            _r   =   x;
            _g   =   y;
            _b   =   z;
            _a   =   w;
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


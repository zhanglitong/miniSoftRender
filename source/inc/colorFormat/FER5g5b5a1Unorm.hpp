#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct  FER5g5b5a1Unorm
    {
        union
        {
            struct
            {
                uint16_t    _a : 1;  // bit  0
                uint16_t    _b : 5;  // bit  1 -  5
                uint16_t    _g : 5;  // bit  6 - 10
                uint16_t    _r : 5;  // bit 11 - 15
            };
            uint16_t        _value;
        };
        
        FER5g5b5a1Unorm(uint16_t value = 0)
        {
            _value  =   value;

        }
        explicit FER5g5b5a1Unorm(uint8_t r,uint8_t g,uint8_t b,uint8_t a)
        {
            _r  =   r;
            _g  =   g;
            _b  =   b;
            _a  =   a;
        }
        auto&       setRed(uint8_t r)
        {
            _r  =   r;
            return  *this;
        }
        auto&       setGreen(uint8_t g)
        {
            _g  =   g;
            return  *this;
        }
        auto&       setBlue(uint8_t b)
        {
            _b  =   b;
            return  *this;
        }
        auto&       setAlpha(uint8_t a)
        {
            _a  =   a;
            return  *this;
        }
        uint8_t     red() const
        {
            return  (uint8_t)_r;
        }
        uint8_t     green() const
        {
            return  (uint8_t)_g;
        }
        uint8_t     blue() const
        {
            return  (uint8_t)_b;
        }
        uint8_t     alpha() const
        {
            return  (uint8_t)_a;
        }
        operator    uint16_t() const
        {
            return  _value;
        }
        operator    uint16_t&()
        {
            return  _value;
        }

    };

}


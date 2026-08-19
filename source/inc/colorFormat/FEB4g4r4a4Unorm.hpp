#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct   FEB4g4r4a4Unorm
    {
    public:
        FEB4g4r4a4Unorm(uint8_t r,uint8_t g,uint8_t b,uint8_t a)
        {
            _r  =   r;
            _g  =   g;
            _b  =   b;
            _a  =   a;
        }
        operator    uint16_t() const
        {
            return  _value;
        }
        operator    uint16_t&()
        {
            return  _value;
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
    protected:
        union
        {
            uint16_t    _value;
            struct  
            {
                uint16_t _a : 4;
                uint16_t _r : 4;
                uint16_t _g : 4;
                uint16_t _b : 4;
            };
        };
    };

}


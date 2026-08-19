#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct   FER4g4Unorm
    {
    public:
        FER4g4Unorm(uint8_t r,uint8_t g)
        {
            _r  =   r;
            _g  =   g;
        }
        operator    uint8_t() const
        {
            return  _value;
        }
        operator    uint8_t&()
        {
            return  _value;
        }
        auto&       setR(uint8_t r)
        {
            _r  =   r;
            return  *this;
        }
        auto&       setG(uint8_t g)
        {
            _g  =   g;
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
    protected:
        union
        {
            uint8_t    _value;
            struct  
            {
                uint8_t _r : 4;
                uint8_t _g : 4;
            };
        };
    };

}


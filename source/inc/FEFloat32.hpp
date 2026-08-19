#pragma     once

#include    <cstddef>
#include    <cstdint>
#include    <cmath>

namespace   FE
{
    struct  float32
    {
        float32(float val = 0)
            :_data(val)
        {}
        template<class T>
        float32(T val)
        {
            _data   =   float(val);
        }
        
        operator    float() const
        {
            return  _data;
        }
    public:
        float   _data;
    };
}


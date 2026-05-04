#pragma     once

#include    "FEDefine.h"
#include    "FEMath.hpp"
namespace   FE
{
    template<class T>
    class    TColor
    {
    public:
    public:
        tvec4<T>    _color;
    };

    using   Rgba8   =   TColor<uint8>;
    using   Rgba16  =   TColor<float16>;
    using   Rgba32  =   TColor<float32>;

}


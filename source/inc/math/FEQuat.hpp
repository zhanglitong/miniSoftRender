#pragma     once
#include    "FEVec3.hpp"

namespace   FE
{
    template<typename T>
    using   tquat   =   glm::tquat<T,   glm::defaultp>;

    template<typename T>
    inline tquat<T> slerp(const tquat<T>& a,const tquat<T>& b,T t) 
    {
        return glm::slerp(a, b, t);
    }
}

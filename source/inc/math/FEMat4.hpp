#pragma     once

#include    "FEVec3.hpp"
#include    "FEVec4.hpp"
#include    "FEQuat.hpp"
#include    "glm/gtx/euler_angles.hpp"
#include    "glm/gtc/quaternion.hpp"
#include    "glm/gtc/matrix_transform.hpp"
namespace   FE
{
    template<typename T>
    using   tmat4       =   glm::mat<4,4, T, glm::defaultp>; 

    template<typename T>
    static inline   tmat4<T>   quatToMat4(const tquat<T>& q) 
    {
        return  glm::mat4_cast(q);
    } 

    template<typename T>
    static inline   tquat<T>   mat4ToQuat(const tmat4<T>& m) 
    {
        return  glm::quat_cast(m);
    } 
}


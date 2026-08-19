

#define     GLM_FORCE_RADIANS
#define     GLM_FORCE_DEPTH_ZERO_TO_ONE
#define     GLM_ENABLE_EXPERIMENTAL

#include    <glm/glm.hpp>
#include    <glm/gtc/quaternion.hpp>
#include    <glm/gtc/matrix_transform.hpp>
#include    <glm/gtx/compatibility.hpp>
#include    <glm/gtx/norm.hpp>
#include    "FEDefine.h"


namespace   FE
{
    template<typename T>
    using tvec2 = glm::vec<2, T, glm::defaultp>;

    using   glm::ceil;
    using   glm::floor;

    using   glm::length;
    using   glm::length2;

    using   glm::distance;
    using   glm::distance2;
    using   glm::min;
    using   glm::max;

    using   glm::clamp;
    using   glm::dot;
    using   glm::cross;
    using   glm::normalize;

    using   glm::perspective;
    using   glm::translate;
    using   glm::rotate;
    using   glm::scale;

    using   glm::ortho;
    using   glm::inverse;
    using   glm::transpose;

    using   glm::lerp;
    using   glm::slerp;
    using   glm::determinant;

}

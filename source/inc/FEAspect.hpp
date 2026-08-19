#pragma     once
#include    <string>
#include    "FEMath.hpp"

namespace   FE
{
    enum    FEAspect
    {
        ASPECT_COLOR_BIT                =   0x00000001,
        ASPECT_DEPTH_BIT                =   0x00000002,
        ASPECT_STENCIL_BIT              =   0x00000004,
        ASPECT_METADATA_BIT             =   0x00000008,
        ASPECT_PLANE_0_BIT              =   0x00000010,
        ASPECT_PLANE_1_BIT              =   0x00000020,
        ASPECT_PLANE_2_BIT              =   0x00000040,
        ASPECT_NONE                     =   0,
        ASPECT_MEMORY_PLANE_0_BIT_EXT   =   0x00000080,
        ASPECT_MEMORY_PLANE_1_BIT_EXT   =   0x00000100,
        ASPECT_MEMORY_PLANE_2_BIT_EXT   =   0x00000200,
        ASPECT_MEMORY_PLANE_3_BIT_EXT   =   0x00000400,
        ASPECT_PLANE_0_BIT_KHR          =   ASPECT_PLANE_0_BIT,
        ASPECT_PLANE_1_BIT_KHR          =   ASPECT_PLANE_1_BIT,
        ASPECT_PLANE_2_BIT_KHR          =   ASPECT_PLANE_2_BIT,
    };

}

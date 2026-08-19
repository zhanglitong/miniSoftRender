#pragma     once
#include    <string>
#include    "FEMath.hpp"

namespace   FE
{
    enum    FEImageLayout:uint32_t
    {
        IL_UNDEFINED                                    =   0,
        IL_GENERAL                                      =   1,
        IL_COLOR_ATTACHMENT_OPTIMAL                     =   2,
        IL_DEPTH_STENCIL_ATTACHMENT_OPTIMAL             =   3,
        IL_DEPTH_STENCIL_READ_ONLY_OPTIMAL              =   4,
        IL_SHADER_READ_ONLY_OPTIMAL                     =   5,
        IL_TRANSFER_SRC_OPTIMAL                         =   6,
        IL_TRANSFER_DST_OPTIMAL                         =   7,
        IL_PREINITIALIZED                               =   8,
        IL_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL   =   1000117000,
        IL_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL   =   1000117001,
        IL_DEPTH_ATTACHMENT_OPTIMAL                     =   1000241000,
        IL_DEPTH_READ_ONLY_OPTIMAL                      =   1000241001,
        IL_STENCIL_ATTACHMENT_OPTIMAL                   =   1000241002,
        IL_STENCIL_READ_ONLY_OPTIMAL                    =   1000241003,
        IL_READ_ONLY_OPTIMAL                            =   1000314000,
        IL_ATTACHMENT_OPTIMAL                           =   1000314001,
        IL_PRESENT_SRC                                  =   1000001002,
        IL_VIDEO_DECODE_DST                             =   1000024000,
        IL_VIDEO_DECODE_SRC                             =   1000024001,
        IL_VIDEO_DECODE_DPB                             =   1000024002,
        IL_SHARED_PRESENT                               =   1000111000,
        IL_FRAGMENT_DENSITY_MAP_OPTIMAL                 =   1000218000,
        IL_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL     =   1000164003,
    };
}

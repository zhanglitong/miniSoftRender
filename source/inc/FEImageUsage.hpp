#pragma     once
#include    "FEFlags.hpp"

namespace   FE
{
    enum    FEImageUsageBit 
    {
        USAGE_TRANSFER_SRC                  =   0x00000001,
        USAGE_TRANSFER_DST                  =   0x00000002,
        USAGE_SAMPLED                       =   0x00000004,
        USAGE_STORAGE                       =   0x00000008,
        USAGE_COLOR_ATTACHMENT              =   0x00000010,
        USAGE_DEPTH_STENCIL_ATTACHMENT      =   0x00000020,
        USAGE_TRANSIENT_ATTACHMENT          =   0x00000040,
        USAGE_INPUT_ATTACHMENT              =   0x00000080,
        USAGE_VIDEO_DECODE_DST              =   0x00000400,
        USAGE_VIDEO_DECODE_SRC              =   0x00000800,
        USAGE_VIDEO_DECODE_DPB              =   0x00001000,
        USAGE_FRAGMENT_DENSITY_MAP          =   0x00000200,
        USAGE_HOST_TRANSFER                 =   0x00400000,
        USAGE_ATTACHMENT_FEEDBACK_LOOP      =   0x00080000,
        USAGE_SAMPLE_WEIGHT_QCOM            =   0x00100000,
        USAGE_SAMPLE_BLOCK_MATCH_QCOM       =   0x00200000,
    };

    using   ImageUsageBits  =   FEFlags<FEImageUsageBit, uint32_t>;

}

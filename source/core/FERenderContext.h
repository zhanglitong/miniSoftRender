#pragma     once

#include    "FERaster.h"
#include    "FEPipeline.h"
#include    "FERenderPass.h"

namespace   FE
{
    class   FERenderContext
    {
    public:
        FERaster*       _raster;
        FERenderPass*   _renderPass;
        FEPipeline*     _pipeline;
    };
}
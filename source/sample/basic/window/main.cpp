
#include    <iostream>
#include    <stdio.h>
#include    "../../../core/FERenderSystem.h"

using   namespace   FE;
int     main(int argc,char** argv)
{
    RenderSysPtr    sys     =   RenderSysPtr(new FERenderSystem());
    RasterPtr       raster  =   RasterPtr(new FERaster());
    FBOPtr          fbo     =   FBOPtr(new FEFrameBuffer());
    PipelinePtr     pipeline=   PipelinePtr(new FEPipeline());
    ProgramPtr      prg     =   FEProgram::create("","");
    RenderPassPtr   rp      =   RenderPassPtr(new FERenderPass());

    {
        FEPipelineInfo  cInfo;

        cInfo._program              =   prg.get();
        cInfo._binds.resize(1);
        cInfo._binds[0].binding     =   0;
        cInfo._binds[0].inputRate   =   VERTEX_INPUT_VERTEX;
        cInfo._binds[0].stride      =   0;
        cInfo._binds[0].attrs       =   {{0,0,FMT_RGBAF32,0}};
        if (pipeline->create(cInfo) != CallResult::CR_SUCCESS)
            return  0;
    }

    auto    cmd =   sys->createCmdBuffer();
    cmd->reset();

    cmd->begin({});
    cmd->end();

    return  0;
}


#include    <iostream>
#include    <stdio.h>
#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEDateTime.hpp"
#include    "../../../inc/FESetting.hpp"
#include    "../../../inc/mesh/FEMesh.hpp"
#include    "../../../inc/geometry/FEGeometry.hpp"
#include    "../../../inc/geometry/FEGeometryParam.hpp"

#include    "../../../core/FEImageHelper.hpp"

#include    "../../../inc/FEStreamReader.hpp"
#include    "../../../inc/FEStreamWriter.hpp"
#include    "../../../inc/FEFileFormatHelper.hpp"
using   namespace   FE;

constexpr   const uint32_t    FBO_WIDTH   =   512;
constexpr   const uint32_t    FBO_HEIGHT  =   512;

struct  UBO
{
    mat4    m;
    mat4    v;
    mat4    p;
    mat4    mvp;
};

int     main(int argc,char** argv)
{
    FESetting       cfg;
    cfg.get("test")
        .set<int>(100)
        .get("111")
        .value<int>(123);
    FEDateTime      dt;
    FEContext       ctx;
    char            sx[64]  =   {};
    dt.toDateString(sx);
    BufferPtr       vbo     =   new FEBuffer(ctx);
    vbo->cInfo()._buffer    =   {float4(1,1,1,0),float4(1,2,3),float4(5,6,7)};

    UBO             uboData;
    uboData.m   =   mat4();
    uboData.v   =   lookAt<float>(float3(0,0,0),float3(0,0,0),float3(0,0,0));
    uboData.p   =   perspective<float>(45.0f,1.0f,0.1f,100.0f);
    uboData.mvp =   uboData.p * uboData.v * uboData.m;
    BufferPtr       ubo     =   new FEBuffer(ctx);
    /// ubo->cInfo()._buffer    =   uboData;

    RenderSysPtr    sys     =   RenderSysPtr(new FERenderSystem(ctx));
    RasterPtr       raster  =   RasterPtr(new FERaster(ctx));
    FBOPtr          fbo     =   FBOPtr(new FEFrameBuffer(ctx));
    PipelinePtr     pipeline=   PipelinePtr(new FEPipeline(ctx));
    ProgramPtr      prg     =   FEProgram::create(ctx,"","");
    RenderPassPtr   rp      =   RenderPassPtr(new FERenderPass(ctx));

    {
        PLCreateInfo  cInfo;

        cInfo._program              =   prg;
        cInfo._binds.resize(1);
        cInfo._binds[0].binding     =   0;
        cInfo._binds[0].inputRate   =   V_INPUT_VERTEX;
        cInfo._binds[0].stride      =   0;
        cInfo._binds[0].attrs       =   {{0,0,FMT_R32G32B32A32_FLOAT,0}};
        if (pipeline->create(cInfo) != FEResult::ER_SUCCESS)
            return  0;
    }

    auto    cmd =   sys->createCmdBuffer();
    cmd->reset();
    FBCreateInfo    fboInf;
    ImagePtr        pColor  =   new FEImage(ctx,FBO_WIDTH,FBO_HEIGHT,FMT_R8G8B8A8_UNORM);        
    ImagePtr        pDepth  =   new FEImage(ctx,FBO_WIDTH,FBO_HEIGHT,FMT_D32_UNORM);
    fboInf.renderPass       =   rp;
    fboInf.colors.push_back(pColor);
    fboInf.depth            =   pDepth;
    fboInf.width            =   FBO_WIDTH;
    fboInf.height           =   FBO_HEIGHT;

    pColor->fill(Rgba8(255,0,0,255));

    auto    result  =   fbo->create(fboInf);
    assert(result);
    cmd->bindFrameBuffer(fbo);
    cmd->bindPipeline(pipeline);
    cmd->pushBuffer(pipeline,0,0,vbo);
    cmd->pushBuffer(pipeline,0,1,ubo);
    cmd->pushPrimitive(PT_POINTS);
    cmd->draw(0,3,0,1);
    cmd->end();
    cmd->submit();

    FE::FEImageHelper::saveBmp(*pColor,"d:/xx.bmp");


    return  0;
}

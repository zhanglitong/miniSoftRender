#pragma     once

#include    "../inc/FEDefine.h"
#include    "../inc/FEObject.h"
#include    "FEStruct.h"
#include    "FEShaderRT.h"

namespace   FE
{
    struct  FEVSBuildIn
    {
        uint32_t    gl_VertexId;
        uint32_t    gl_InstanceId;
        float4      gl_Position;
        RectU16     gl_ViewPort;

        inline  void    setVertexId(uint32_t id)
        {
            gl_VertexId =   id;
        }
        inline  void    setInstanceId(uint32_t id)
        {
            gl_InstanceId =   id;
        }
        inline  float3  project(const float4& dc) const
        {
            float   invW    =   1.0f/dc.w;
            float3  screen(dc.x * invW,dc.y * invW,dc.z * invW);

            // map to range 0 - 1
            screen.x    =   screen.x * 0.5f + 0.5f;
            screen.y    =   screen.y * 0.5f + 0.5f;
            screen.z    =   screen.z * 0.5f + 0.5f;
            // map to viewport
            screen.x    =   screen.x * gl_ViewPort.width();
            screen.y    =   gl_ViewPort.bottom() - (screen.y * gl_ViewPort.height());
            return  screen;
        }
    };

    struct  FEPSBuildIn
    {};
    struct  FERenderState 
    {
        PrimitiveTopology   primitive           =   PT_POINTS;
        FEVSBuildIn         vsBuildIn;
        float4              position[3];
        ShaderModule        vs;
        ShaderModule        ps;
        FEFrameBuffer*      fbo                 =   nullptr;
        FEPipeline*         pl                  =   nullptr;
    };
}

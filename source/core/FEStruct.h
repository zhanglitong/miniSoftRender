#pragma     once
#include    <string>
#include    "../inc/FEObject.h"
#include    "../inc/FEMath.hpp"
#include    "../inc/FEFormat.hpp"
#include    "../inc/FEResult.hpp"


namespace   FE
{
    class   FERenderPass;
    class   FEFrameBuffer;
    class   FEPipeline;
    class   FEDescriptor;
    class   FEShader;
    class   FEProgram;

    using   String  =   std::string;

    enum    ShaderStage : uint32_t
    {
        SHADER_VERTEX       =   (1<<1),
        SHADER_FRAGMENT     =   (1<<2),
        SHADER_TES          =   (1<<3),
        SHADER_TCS          =   (1<<4),
        SHADER_GEM          =   (1<<5),
        SHADER_COMPUTE      =   (1<<6),
        SHADER_RAYGEN       =   (1<<7),
        SHADER_ANYHIT       =   (1<<8),
        SHADER_CLOSESTHIT   =   (1<<9),
        SHADER_MISS         =   (1<<10),
        SHADER_INTERSECT    =   (1<<11),
        SHADER_CALLABLE     =   (1<<12),
        SHADER_MESH         =   (1<<13),
        SHADER_TASK         =   (1<<14),
    };

    enum    StructureType:uint32_t
    {
    };

    enum  PrimitiveTopology :uint32_t
    {
        PT_POINTS,
        PT_LINES,
        PT_LINE_STRIP,
        PT_LINE_LOOP,
        PT_TRIANGLES,
        PT_TRIANGLE_STRIP,
        PT_TRIANGLE_FAN,
    };

    union   FEClearColorValue
    {
        float       float32[4];
        int32_t     int32[4];
        uint32_t    uint32[4];
    } ;

    struct  FEClearDepthStencilValue 
    {
        float       depth;
        uint32_t    stencil;
    };


    union   FEClearValue 
    {
        FEClearColorValue           color;
        FEClearDepthStencilValue    depthStencil;
    };


    struct  FERPBeginInfo 
    {
        FERenderPass*       renderPass      =   nullptr; 
        FEFrameBuffer*      framebuffer     =   nullptr;
        RectU16             renderArea;
        const FEClearValue* pClearValues    =   nullptr;
        uint32_t            clearValueCount =   0;
    };
    
}
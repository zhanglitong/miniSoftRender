#pragma     once
#include    "wgpu.h"
#include    "FEFormat.hpp"
#include    "FEAspect.hpp"
#include    "FEImageLayout.hpp"
#include    "graphic/FEShaderType.hpp"
#include    "graphic/FEGraphicEnums.h"
#include    "mesh/FEPrimitive.hpp"


namespace   FE
{

    inline  static  auto  system2Native(EPrimitive pri)
    {
        switch(pri)
        {

        case PRI_POINTS          :  return  WGPUPrimitiveTopology_PointList;
        case PRI_LINES           :  return  WGPUPrimitiveTopology_LineList;
        case PRI_LINE_STRIP      :  return  WGPUPrimitiveTopology_LineStrip;
        case PRI_TRIANGLES       :  return  WGPUPrimitiveTopology_TriangleList;
        case PRI_TRIANGLE_STRIP  :  return  WGPUPrimitiveTopology_TriangleStrip;
        case PRI_TRIANGLE_FAN    :  return  WGPUPrimitiveTopology_Undefined;
        }
        return  WGPUPrimitiveTopology_PointList;
    }

    inline  static  auto    system2Native(FEFormat fmt)
    {
        switch(fmt)
        {
        case FMT_R8_UNORM           :   return  WGPUTextureFormat_R8Unorm;
        case FMT_R8G8_UNORM         :   return  WGPUTextureFormat_RG8Unorm;
        case FMT_R8G8B8_UNORM       :   return  WGPUTextureFormat_RGBA8Unorm;
        case FMT_R8G8B8A8_UNORM     :   return  WGPUTextureFormat_RGBA8Unorm;


        case FMT_R8_UINT            :   return  WGPUTextureFormat_R8Uint;
        case FMT_R8G8_UINT          :   return  WGPUTextureFormat_RG8Uint;
        case FMT_R8G8B8_UINT        :   return  WGPUTextureFormat_RGBA8Uint;
        case FMT_R8G8B8A8_UINT      :   return  WGPUTextureFormat_RGBA8Uint;

        case FMT_R8_SINT            :   return  WGPUTextureFormat_R8Sint;
        case FMT_R8G8_SINT          :   return  WGPUTextureFormat_RG8Sint;
        case FMT_R8G8B8_SINT        :   return  WGPUTextureFormat_RGBA8Sint;
        case FMT_R8G8B8A8_SINT      :   return  WGPUTextureFormat_RGBA8Sint;


        case FMT_R8_SSCALED         :   return  WGPUTextureFormat_R8Unorm;
        case FMT_R8G8_SSCALED       :   return  WGPUTextureFormat_RG8Unorm;
        case FMT_R8G8B8_SSCALED     :   return  WGPUTextureFormat_RGBA8Unorm;
        case FMT_R8G8B8A8_SSCALED   :   return  WGPUTextureFormat_RGBA8Unorm;

        case FMT_R8_USCALED         :   return  WGPUTextureFormat_R8Unorm;
        case FMT_R8G8_USCALED       :   return  WGPUTextureFormat_RG8Unorm;
        case FMT_R8G8B8_USCALED     :   return  WGPUTextureFormat_RGBA8Unorm;
        case FMT_R8G8B8A8_USCALED   :   return  WGPUTextureFormat_RGBA8Unorm;

        case FMT_B8G8R8A8_UNORM     :   return  WGPUTextureFormat_BGRA8Unorm;

        
        case FMT_R32_FLOAT          :   return  WGPUTextureFormat_R32Float;
        case FMT_R32G32_FLOAT       :   return  WGPUTextureFormat_RG32Float;
        case FMT_R32G32B32_FLOAT    :   return  WGPUTextureFormat_RGBA32Float;
        case FMT_R32G32B32A32_FLOAT :   return  WGPUTextureFormat_RGBA32Float;

        case FMT_R16_FLOAT          :   return  WGPUTextureFormat_R16Float;
        case FMT_R16G16_FLOAT       :   return  WGPUTextureFormat_RG16Float;
        case FMT_R16G16B16_FLOAT    :   return  WGPUTextureFormat_RGBA16Float;
        case FMT_R16G16B16A16_FLOAT :   return  WGPUTextureFormat_RGBA16Float;

        case FMT_R4G4B4A4_UNORM     :   return  WGPUTextureFormat_Undefined;
        case FMT_B4G4R4A4_UNORM     :   return  WGPUTextureFormat_Undefined;
        case FMT_R5G6B5_UNORM       :   return  WGPUTextureFormat_Undefined;
        case FMT_B5G6R5_UNORM       :   return  WGPUTextureFormat_Undefined;
        case FMT_R5G5B5A1_UNORM     :   return  WGPUTextureFormat_Undefined;
        case FMT_B5G5R5A1_UNORM     :   return  WGPUTextureFormat_Undefined;
        case FMT_A1R5G5B5_UNORM     :   return  WGPUTextureFormat_Undefined;


        case FMT_A2B10G10R10_UNORM  :   return  WGPUTextureFormat_RGB10A2Unorm; 
        case FMT_A2B10G10R10_SNORM  :   return  WGPUTextureFormat_Undefined; 
        case FMT_A2B10G10R10_USCALED:   return  WGPUTextureFormat_RGB10A2Unorm; 
        case FMT_A2B10G10R10_SSCALED:   return  WGPUTextureFormat_Undefined; 
        case FMT_A2B10G10R10_UINT   :   return  WGPUTextureFormat_RGB10A2Uint; 
        case FMT_A2B10G10R10_SINT   :   return  WGPUTextureFormat_Undefined; 

        case FMT_R32_UINT           :   return  WGPUTextureFormat_R32Uint;
        case FMT_R32_SINT           :   return  WGPUTextureFormat_R32Sint;
        case FMT_R32G32_UINT        :   return  WGPUTextureFormat_RG32Uint;
        case FMT_R32G32_SINT        :   return  WGPUTextureFormat_RG32Sint;
       
        case FMT_R32G32B32_UINT     :   return  WGPUTextureFormat_RGBA32Uint;
        case FMT_R32G32B32_SINT     :   return  WGPUTextureFormat_RGBA32Sint;
        
        case FMT_R32G32B32A32_UINT  :   return  WGPUTextureFormat_RGBA32Uint;
        case FMT_R32G32B32A32_SINT  :   return  WGPUTextureFormat_RGBA32Sint;

        case FMT_D32_UNORM          :   return  WGPUTextureFormat_Depth32Float;
        case FMT_D32_S8_UNORM       :   return  WGPUTextureFormat_Depth32FloatStencil8;
        case FMT_D16_UNORM          :   return  WGPUTextureFormat_Depth16Unorm;

        default:
            assert(0!=0);
            return  WGPUTextureFormat_RGBA8Unorm;
        }
    }

    
    inline  static  auto    system2Native(FEAspect val)
    {
        switch(val)
        {
        case ASPECT_COLOR_BIT               : return  WGPUTextureAspect_All       ;
        case ASPECT_DEPTH_BIT               : return  WGPUTextureAspect_DepthOnly ;
        case ASPECT_STENCIL_BIT             : return  WGPUTextureAspect_StencilOnly;
        default                             : return  WGPUTextureAspect_All       ;
        }
    }

    inline  static  auto  system2Native(FEShaderType val)
    {
        switch(val)
        {
        case ST_VERTEX_BIT                  :   return  WGPUShaderStage_Vertex  ;
        case ST_FRAGMENT_BIT                :   return  WGPUShaderStage_Fragment;
        case ST_COMPUTE_BIT                 :   return  WGPUShaderStage_Compute ;
        default                             :   return  WGPUShaderStage_None   ;
        }
    }


    inline  static  auto  system2Native(GBType fmt)
    {
        switch(fmt)
        {
        case GPUBufferType_Vertex:      return  WGPUBufferUsage_Vertex;
        case GPUBufferType_Index:       return  WGPUBufferUsage_Index;
        case GPUBufferType_Uniform:     return  WGPUBufferUsage_Uniform;
        default:                        return  WGPUBufferUsage_Uniform;
        }
    }


    inline  static  auto  system2Native(PLType fmt)
    {
        switch(fmt)
        {
        case PL_GRAPIC:         return  0;
        case PL_COMPUTE:        return  0;
        case PL_RAY_TRACING:    return  0;
        default:                return  0;
        }
    }
    inline  static  auto  system2Native(FEIndexType fmt)
    {
        switch(fmt)
        {
        case INDEX_UINT16:      return  WGPUIndexFormat_Uint16;
        case INDEX_UINT32:      return  WGPUIndexFormat_Uint32;
        default:                return  WGPUIndexFormat_Uint16;
        }
    }

    inline  static  auto    system2Native(MemoryUsages hFlags)
    {
        (void)hFlags;
        return  WGPUBufferUsage_None;
    }

    inline  static  auto    system2Native(BufferUsages hFlags)
    {
        WGPUBufferUsage flags   =   WGPUBufferUsage_None;
        if(hFlags.hasFlag(BufferUsage::TRANSFER_SRC_BIT))                          flags |=  WGPUBufferUsage_CopySrc;
        if(hFlags.hasFlag(BufferUsage::TRANSFER_DST_BIT))                          flags |=  WGPUBufferUsage_CopyDst;
        if(hFlags.hasFlag(BufferUsage::UNIFORM_BUFFER_BIT))                        flags |=  WGPUBufferUsage_Uniform;
        if(hFlags.hasFlag(BufferUsage::STORAGE_BUFFER_BIT))                        flags |=  WGPUBufferUsage_Storage;
        if(hFlags.hasFlag(BufferUsage::INDEX_BUFFER_BIT))                          flags |=  WGPUBufferUsage_Index;
        if(hFlags.hasFlag(BufferUsage::VERTEX_BUFFER_BIT))                         flags |=  WGPUBufferUsage_Vertex;
        if(hFlags.hasFlag(BufferUsage::INDIRECT_BUFFER_BIT))                       flags |=  WGPUBufferUsage_Indirect;
        if(hFlags.hasFlag(BufferUsage::SHADER_DEVICE_ADDRESS_BIT))                 flags |=  WGPUBufferUsage_QueryResolve;
        if(hFlags.hasFlag(BufferUsage::ACCELERATION_STRUCTURE_STORAGE_BIT))        flags |=  WGPUBufferUsage_Storage;
        if(hFlags.hasFlag(BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY)) flags |=  WGPUBufferUsage_CopySrc;

        return  flags;
    }
    static  auto    system2Native(DynamicState state)
    {
        (void)state;
        return  0;
    }

    static  auto    system2Native(FEImageLayout layout)
    {
        (void)layout;
        return  0;
    }

}

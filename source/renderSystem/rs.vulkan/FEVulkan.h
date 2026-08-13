#pragma     once
#include    <vulkan/vulkan.h>
#include    "FEFormat.hpp"
#include    "FEAspect.hpp"
#include    "FEImageLayout.hpp"
#include    "FEImageUsage.hpp"
#include    "FEImageType.hpp"
#include    "graphic/FEShaderType.hpp"
#include    "graphic/FEGraphicEnums.h"
#include    "mesh/FEPrimitive.hpp"


#define    VK_CHECK_RESULT(f)       \
{                                   \
    VkResult res = (f);             \
    if (res != VK_SUCCESS)          \
    {                               \
        assert(res != VK_SUCCESS);  \
    }                               \
}

namespace   FE
{

    inline  static  auto  system2Native(EPrimitive pri)
    {
        switch(pri)
        {

        case PRI_POINTS          :  return  VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PRI_LINES           :  return  VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PRI_LINE_STRIP      :  return  VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PRI_TRIANGLES       :  return  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PRI_TRIANGLE_STRIP  :  return  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PRI_TRIANGLE_FAN    :  return  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        }
        return  VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }

    inline  static  auto    system2Native(FEFormat fmt)
    {
        switch(fmt)
        {
        case FMT_R8_UNORM           :   return  VK_FORMAT_R8_UNORM;
        case FMT_R8G8_UNORM         :   return  VK_FORMAT_R8G8_UNORM;
        case FMT_R8G8B8_UNORM       :   return  VK_FORMAT_R8G8B8_UNORM;
        case FMT_R8G8B8A8_UNORM     :   return  VK_FORMAT_R8G8B8A8_UNORM;


        case FMT_R8_UINT            :   return  VK_FORMAT_R8_UINT;
        case FMT_R8G8_UINT          :   return  VK_FORMAT_R8G8_UINT;
        case FMT_R8G8B8_UINT        :   return  VK_FORMAT_R8G8B8_UINT;
        case FMT_R8G8B8A8_UINT      :   return  VK_FORMAT_R8G8B8A8_UINT;

        case FMT_R8_SINT            :   return  VK_FORMAT_R8_SINT;
        case FMT_R8G8_SINT          :   return  VK_FORMAT_R8G8_SINT;
        case FMT_R8G8B8_SINT        :   return  VK_FORMAT_R8G8B8_SINT;
        case FMT_R8G8B8A8_SINT      :   return  VK_FORMAT_R8G8B8A8_SINT;


        case FMT_R8_SSCALED         :   return  VK_FORMAT_R8_SSCALED;
        case FMT_R8G8_SSCALED       :   return  VK_FORMAT_R8G8_SSCALED;
        case FMT_R8G8B8_SSCALED     :   return  VK_FORMAT_R8G8B8_SSCALED;
        case FMT_R8G8B8A8_SSCALED   :   return  VK_FORMAT_R8G8B8A8_SSCALED;

        case FMT_R8_USCALED         :   return  VK_FORMAT_R8_USCALED;
        case FMT_R8G8_USCALED       :   return  VK_FORMAT_R8G8_USCALED;
        case FMT_R8G8B8_USCALED     :   return  VK_FORMAT_R8G8B8_USCALED;
        case FMT_R8G8B8A8_USCALED   :   return  VK_FORMAT_R8G8B8A8_USCALED;

        case FMT_B8G8R8A8_UNORM     :   return  VK_FORMAT_B8G8R8A8_UNORM;

        
        case FMT_R32_FLOAT          :   return  VK_FORMAT_R32_SFLOAT;
        case FMT_R32G32_FLOAT       :   return  VK_FORMAT_R32G32_SFLOAT;
        case FMT_R32G32B32_FLOAT    :   return  VK_FORMAT_R32G32B32_SFLOAT;
        case FMT_R32G32B32A32_FLOAT :   return  VK_FORMAT_R32G32B32A32_SFLOAT;

        case FMT_R16_FLOAT          :   return  VK_FORMAT_R16_SFLOAT;
        case FMT_R16G16_FLOAT       :   return  VK_FORMAT_R16G16_SFLOAT;
        case FMT_R16G16B16_FLOAT    :   return  VK_FORMAT_R16G16B16_SFLOAT;
        case FMT_R16G16B16A16_FLOAT :   return  VK_FORMAT_R16G16B16A16_SFLOAT;

        case FMT_R4G4B4A4_UNORM     :   return  VK_FORMAT_R4G4B4A4_UNORM_PACK16 ;
        case FMT_B4G4R4A4_UNORM     :   return  VK_FORMAT_B4G4R4A4_UNORM_PACK16 ;
        case FMT_R5G6B5_UNORM       :   return  VK_FORMAT_R5G6B5_UNORM_PACK16   ;
        case FMT_B5G6R5_UNORM       :   return  VK_FORMAT_B5G6R5_UNORM_PACK16   ;
        case FMT_R5G5B5A1_UNORM     :   return  VK_FORMAT_R5G5B5A1_UNORM_PACK16 ;
        case FMT_B5G5R5A1_UNORM     :   return  VK_FORMAT_B5G5R5A1_UNORM_PACK16 ;
        case FMT_A1R5G5B5_UNORM     :   return  VK_FORMAT_A1R5G5B5_UNORM_PACK16 ;


        case FMT_A2B10G10R10_UNORM  :   return  VK_FORMAT_A2B10G10R10_UNORM_PACK32  ; 
        case FMT_A2B10G10R10_SNORM  :   return  VK_FORMAT_A2B10G10R10_SNORM_PACK32  ; 
        case FMT_A2B10G10R10_USCALED:   return  VK_FORMAT_A2B10G10R10_USCALED_PACK32; 
        case FMT_A2B10G10R10_SSCALED:   return  VK_FORMAT_A2B10G10R10_SSCALED_PACK32; 
        case FMT_A2B10G10R10_UINT   :   return  VK_FORMAT_A2B10G10R10_UINT_PACK32   ; 
        case FMT_A2B10G10R10_SINT   :   return  VK_FORMAT_A2B10G10R10_SINT_PACK32   ; 

        case FMT_R32_UINT           :   return  VK_FORMAT_R32_UINT;
        case FMT_R32_SINT           :   return  VK_FORMAT_R32_SINT;
        case FMT_R32G32_UINT        :   return  VK_FORMAT_R32G32_UINT;
        case FMT_R32G32_SINT        :   return  VK_FORMAT_R32G32_SINT;
       
        case FMT_R32G32B32_UINT     :   return  VK_FORMAT_R32G32B32_UINT;
        case FMT_R32G32B32_SINT     :   return  VK_FORMAT_R32G32B32_SINT;
        
        case FMT_R32G32B32A32_UINT  :   return  VK_FORMAT_R32G32B32A32_UINT;
        case FMT_R32G32B32A32_SINT  :   return  VK_FORMAT_R32G32B32A32_SINT;

        case FMT_D32_UNORM          :   return  VK_FORMAT_D32_SFLOAT;
        case FMT_D32_S8_UNORM       :   return  VK_FORMAT_D32_SFLOAT_S8_UINT;

        default:
            assert(0!=0);
            return  VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    
    inline  static  auto    system2Native(FEAspect val)
    {
        switch(val)
        {
        case ASPECT_COLOR_BIT               : return  VK_IMAGE_ASPECT_COLOR_BIT             ;
        case ASPECT_DEPTH_BIT               : return  VK_IMAGE_ASPECT_DEPTH_BIT             ;
        case ASPECT_STENCIL_BIT             : return  VK_IMAGE_ASPECT_STENCIL_BIT           ;
        case ASPECT_METADATA_BIT            : return  VK_IMAGE_ASPECT_METADATA_BIT          ;
        case ASPECT_PLANE_0_BIT             : return  VK_IMAGE_ASPECT_PLANE_0_BIT           ;
        case ASPECT_PLANE_1_BIT             : return  VK_IMAGE_ASPECT_PLANE_1_BIT           ;
        case ASPECT_PLANE_2_BIT             : return  VK_IMAGE_ASPECT_PLANE_2_BIT           ;
        case ASPECT_NONE                    : return  VK_IMAGE_ASPECT_NONE                  ;
        case ASPECT_MEMORY_PLANE_0_BIT_EXT  : return  VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
        case ASPECT_MEMORY_PLANE_1_BIT_EXT  : return  VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT;
        case ASPECT_MEMORY_PLANE_2_BIT_EXT  : return  VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT;
        case ASPECT_MEMORY_PLANE_3_BIT_EXT  : return  VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT;
        default                             : return  VK_IMAGE_ASPECT_COLOR_BIT             ;
        }
    }

    inline  static  auto  system2Native(FEShaderType val)
    {
        switch(val)
        {
        case ST_VERTEX_BIT                  :   return  VK_SHADER_STAGE_VERTEX_BIT                  ;
        case ST_TESSELLATION_CONTROL_BIT    :   return  VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT    ;
        case ST_TESSELLATION_EVALUATION_BIT :   return  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT ;
        case ST_GEOMETRY_BIT                :   return  VK_SHADER_STAGE_GEOMETRY_BIT                ;
        case ST_FRAGMENT_BIT                :   return  VK_SHADER_STAGE_FRAGMENT_BIT                ;
        case ST_COMPUTE_BIT                 :   return  VK_SHADER_STAGE_COMPUTE_BIT                 ;
        case ST_RAYGEN_BIT                  :   return  VK_SHADER_STAGE_RAYGEN_BIT_KHR              ;
        case ST_ANY_HIT_BIT                 :   return  VK_SHADER_STAGE_ANY_HIT_BIT_KHR             ;
        case ST_CLOSEST_HIT_BIT             :   return  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR         ;
        case ST_MISS_BIT                    :   return  VK_SHADER_STAGE_MISS_BIT_KHR                ;
        case ST_INTERSECTION_BIT            :   return  VK_SHADER_STAGE_INTERSECTION_BIT_KHR        ;
        case ST_CALLABLE_BIT                :   return  VK_SHADER_STAGE_CALLABLE_BIT_KHR            ;
        case ST_TASK_BIT                    :   return  VK_SHADER_STAGE_TASK_BIT_EXT                ;
        case ST_MESH_BIT                    :   return  VK_SHADER_STAGE_MESH_BIT_EXT                ;
        default                             :   return  VK_SHADER_STAGE_VERTEX_BIT                  ;
        }
    }


    inline  static  auto  system2Native(GBType fmt)
    {
        switch(fmt)
        {
        case GPUBufferType_Vertex:      return  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case GPUBufferType_Index:       return  VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case GPUBufferType_Uniform:     return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        default:                        return  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
    }


    inline  static  auto  system2Native(PLType fmt)
    {
        switch(fmt)
        {
        case PL_GRAPIC:         return  VK_PIPELINE_BIND_POINT_GRAPHICS;
        case PL_COMPUTE:        return  VK_PIPELINE_BIND_POINT_COMPUTE;
        case PL_RAY_TRACING:    return  VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        default:                return  VK_PIPELINE_BIND_POINT_GRAPHICS;
        }
    }
    inline  static  auto  system2Native(FEIndexType fmt)
    {
        switch(fmt)
        {
        case INDEX_UINT16:      return  VK_INDEX_TYPE_UINT16;
        case INDEX_UINT32:      return  VK_INDEX_TYPE_UINT32;
        case INDEX_UINT8:       return  VK_INDEX_TYPE_UINT8_EXT;
        default:                return  VK_INDEX_TYPE_UINT16;
        }
    }

    inline  static  auto    system2Native(MemoryUsages hFlags)
    {
        VkFlags flags   =   0;
        if (hFlags.hasFlag(MemoryUsage::DEVICE_LOCAL_BIT))      flags    |=  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (hFlags.hasFlag(MemoryUsage::HOST_VISIBLE_BIT))      flags    |=  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (hFlags.hasFlag(MemoryUsage::HOST_COHERENT_BIT))     flags    |=  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (hFlags.hasFlag(MemoryUsage::HOST_CACHED_BIT))       flags    |=  VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        if (hFlags.hasFlag(MemoryUsage::LAZILY_ALLOCATED_BIT))  flags    |=  VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

        return  flags;
    }

    inline  static  auto    system2Native(BufferUsages hFlags)
    {
        VkFlags flags   =   0;
        if(hFlags.hasFlag(BufferUsage::TRANSFER_SRC_BIT                             )) flags    |=  VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if(hFlags.hasFlag(BufferUsage::TRANSFER_DST_BIT                             )) flags    |=  VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if(hFlags.hasFlag(BufferUsage::UNIFORM_TEXEL_BUFFER_BIT                     )) flags    |=  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::STORAGE_TEXEL_BUFFER_BIT                     )) flags    |=  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::UNIFORM_BUFFER_BIT                           )) flags    |=  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::STORAGE_BUFFER_BIT                           )) flags    |=  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::INDEX_BUFFER_BIT                             )) flags    |=  VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::VERTEX_BUFFER_BIT                            )) flags    |=  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::INDIRECT_BUFFER_BIT                          )) flags    |=  VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        if(hFlags.hasFlag(BufferUsage::SHADER_DEVICE_ADDRESS_BIT                    )) flags    |=  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if(hFlags.hasFlag(BufferUsage::VIDEO_DECODE_SRC_BIT                         )) flags    |=  VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
        if(hFlags.hasFlag(BufferUsage::VIDEO_DECODE_DST_BIT                         )) flags    |=  VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR;
        if(hFlags.hasFlag(BufferUsage::TRANSFORM_FEEDBACK_BUFFER_BIT                )) flags    |=  VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT        )) flags    |=  VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::CONDITIONAL_RENDERING_BIT                    )) flags    |=  VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY )) flags    |=  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        if(hFlags.hasFlag(BufferUsage::ACCELERATION_STRUCTURE_STORAGE_BIT           )) flags    |=  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        if(hFlags.hasFlag(BufferUsage::SHADER_BINDING_TABLE_BIT                     )) flags    |=  VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
        /// if(hFlags.hasFlag(BufferUsage::VIDEO_ENCODE_DST_BIT                     )) flags    |=  VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
        /// if(hFlags.hasFlag(BufferUsage::VIDEO_ENCODE_SRC_BIT                     )) flags    |=  VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
        if(hFlags.hasFlag(BufferUsage::SAMPLER_DESCRIPTOR_BUFFER_BIT                )) flags    |=  VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::RESOURCE_DESCRIPTOR_BUFFER_BIT               )) flags    |=  VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT       )) flags    |=  VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::MICROMAP_BUILD_INPUT_READ_ONLY_BIT           )) flags    |=  VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT;
        if(hFlags.hasFlag(BufferUsage::MICROMAP_STORAGE_BIT                         )) flags    |=  VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT;

        return  flags;
    }
    static  auto    system2Native(DynamicState state)
    {
        switch(state)
        {
        case VIEWPORT                               :   return  VK_DYNAMIC_STATE_VIEWPORT                               ;
        case SCISSOR                                :   return  VK_DYNAMIC_STATE_SCISSOR                                ;
        case LINE_WIDTH                             :   return  VK_DYNAMIC_STATE_LINE_WIDTH                             ;
        case DEPTH_BIAS                             :   return  VK_DYNAMIC_STATE_DEPTH_BIAS                             ;
        case BLEND_CONSTANTS                        :   return  VK_DYNAMIC_STATE_BLEND_CONSTANTS                        ;
        case DEPTH_BOUNDS                           :   return  VK_DYNAMIC_STATE_DEPTH_BOUNDS                           ;
        case STENCIL_COMPARE_MASK                   :   return  VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK                   ;
        case STENCIL_WRITE_MASK                     :   return  VK_DYNAMIC_STATE_STENCIL_WRITE_MASK                     ;
        case STENCIL_REFERENCE                      :   return  VK_DYNAMIC_STATE_STENCIL_REFERENCE                      ;
        case CULL_MODE                              :   return  VK_DYNAMIC_STATE_CULL_MODE                              ;
        case FRONT_FACE                             :   return  VK_DYNAMIC_STATE_FRONT_FACE                             ;
        case PRIMITIVE_TOPOLOGY                     :   return  VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY                     ;
        case VIEWPORT_WITH_COUNT                    :   return  VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT                    ;
        case SCISSOR_WITH_COUNT                     :   return  VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT                     ;
        case VERTEX_INPUT_BINDING_STRIDE            :   return  VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE            ;
        case DEPTH_TEST_ENABLE                      :   return  VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE                      ;
        case DEPTH_WRITE_ENABLE                     :   return  VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE                     ;
        case DEPTH_COMPARE_OP                       :   return  VK_DYNAMIC_STATE_DEPTH_COMPARE_OP                       ;
        case DEPTH_BOUNDS_TEST_ENABLE               :   return  VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE               ;
        case STENCIL_TEST_ENABLE                    :   return  VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE                    ;
        case STENCIL_OP                             :   return  VK_DYNAMIC_STATE_STENCIL_OP                             ;
        case RASTERIZER_DISCARD_ENABLE              :   return  VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE              ;
        case DEPTH_BIAS_ENABLE                      :   return  VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE                      ;
        case PRIMITIVE_RESTART_ENABLE               :   return  VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE               ;
        case VIEWPORT_W_SCALING                     :   return  VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV                  ;
        case DISCARD_RECTANGLE                      :   return  VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT                  ;
        case DISCARD_RECTANGLE_ENABLE               :   return  VK_DYNAMIC_STATE_DISCARD_RECTANGLE_ENABLE_EXT           ;
        case DISCARD_RECTANGLE_MODE                 :   return  VK_DYNAMIC_STATE_DISCARD_RECTANGLE_MODE_EXT             ;
        case SAMPLE_LOCATIONS                       :   return  VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT                   ;
        case RAY_TRACING_PIPELINE_STACK_SIZE        :   return  VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR    ;
        case VIEWPORT_SHADING_RATE_PALETTE          :   return  VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV       ;
        case VIEWPORT_COARSE_SAMPLE_ORDER           :   return  VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV        ;
        case EXCLUSIVE_SCISSOR_ENABLE               :   return  VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_ENABLE_NV            ;
        case EXCLUSIVE_SCISSOR                      :   return  VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV                   ;
        case FRAGMENT_SHADING_RATE                  :   return  VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR              ;
        case LINE_STIPPLE                           :   return  VK_DYNAMIC_STATE_LINE_STIPPLE_EXT                       ;
        case VERTEX_INPUT                           :   return  VK_DYNAMIC_STATE_VERTEX_INPUT_EXT                       ;
        case PATCH_CONTROL_POINTS                   :   return  VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT               ;
        case LOGIC_OP                               :   return  VK_DYNAMIC_STATE_LOGIC_OP_EXT                           ;
        case COLOR_WRITE_ENABLE                     :   return  VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT                 ;
        case TESSELLATION_DOMAIN_ORIGIN             :   return  VK_DYNAMIC_STATE_TESSELLATION_DOMAIN_ORIGIN_EXT         ;
        case DEPTH_CLAMP_ENABLE                     :   return  VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT                 ;
        case POLYGON_MODE                           :   return  VK_DYNAMIC_STATE_POLYGON_MODE_EXT                       ;
        case RASTERIZATION_SAMPLES                  :   return  VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT              ;
        case SAMPLE_MASK                            :   return  VK_DYNAMIC_STATE_SAMPLE_MASK_EXT                        ;
        case ALPHA_TO_COVERAGE_ENABLE               :   return  VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT           ;
        case ALPHA_TO_ONE_ENABLE                    :   return  VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT                ;
        case LOGIC_OP_ENABLE                        :   return  VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT                    ;
        case COLOR_BLEND_ENABLE                     :   return  VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT                 ;
        case COLOR_BLEND_EQUATION                   :   return  VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT               ;
        case COLOR_WRITE_MASK                       :   return  VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT                   ;
        case RASTERIZATION_STREAM                   :   return  VK_DYNAMIC_STATE_RASTERIZATION_STREAM_EXT               ;
        case CONSERVATIVE_RASTERIZATION_MODE        :   return  VK_DYNAMIC_STATE_CONSERVATIVE_RASTERIZATION_MODE_EXT    ;
        case EXTRA_PRIMITIVE_OVERESTIMATION_SIZE    :   return  VK_DYNAMIC_STATE_EXTRA_PRIMITIVE_OVERESTIMATION_SIZE_EXT;
        case DEPTH_CLIP_ENABLE                      :   return  VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT                  ;
        case SAMPLE_LOCATIONS_ENABLE                :   return  VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT            ;
        case COLOR_BLEND_ADVANCED                   :   return  VK_DYNAMIC_STATE_COLOR_BLEND_ADVANCED_EXT               ;
        case PROVOKING_VERTEX_MODE                  :   return  VK_DYNAMIC_STATE_PROVOKING_VERTEX_MODE_EXT              ;
        case LINE_RASTERIZATION_MODE                :   return  VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT            ;
        case LINE_STIPPLE_ENABLE                    :   return  VK_DYNAMIC_STATE_LINE_STIPPLE_ENABLE_EXT                ;
        case DEPTH_CLIP_NEGATIVE_ONE_TO_ONE         :   return  VK_DYNAMIC_STATE_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE_EXT     ;
        case VIEWPORT_W_SCALING_ENABLE              :   return  VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_ENABLE_NV           ;
        case VIEWPORT_SWIZZLE                       :   return  VK_DYNAMIC_STATE_VIEWPORT_SWIZZLE_NV                    ;
        case COVERAGE_TO_COLOR_ENABLE               :   return  VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_ENABLE_NV            ;
        case COVERAGE_TO_COLOR_LOCATION             :   return  VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_LOCATION_NV          ;
        case COVERAGE_MODULATION_MODE               :   return  VK_DYNAMIC_STATE_COVERAGE_MODULATION_MODE_NV            ;
        case COVERAGE_MODULATION_TABLE_ENABLE       :   return  VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_ENABLE_NV    ;
        case COVERAGE_MODULATION_TABLE              :   return  VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_NV           ;
        case SHADING_RATE_IMAGE_ENABLE              :   return  VK_DYNAMIC_STATE_SHADING_RATE_IMAGE_ENABLE_NV           ;
        case REPRESENTATIVE_FRAGMENT_TEST_ENABLE    :   return  VK_DYNAMIC_STATE_REPRESENTATIVE_FRAGMENT_TEST_ENABLE_NV ;
        case COVERAGE_REDUCTION_MODE                :   return  VK_DYNAMIC_STATE_COVERAGE_REDUCTION_MODE_NV             ;
        case ATTACHMENT_FEEDBACK_LOOP_ENABLE        :   return  VK_DYNAMIC_STATE_ATTACHMENT_FEEDBACK_LOOP_ENABLE_EXT    ;
        }
        return  VK_DYNAMIC_STATE_VIEWPORT;
    }

    static  auto    system2Native(FEImageLayout layout)
    {
        switch(layout)
        {
        case IL_UNDEFINED:                                    return  VK_IMAGE_LAYOUT_UNDEFINED;
        case IL_GENERAL:                                      return  VK_IMAGE_LAYOUT_GENERAL;
        case IL_PREINITIALIZED:                               return  VK_IMAGE_LAYOUT_PREINITIALIZED;
        case IL_TRANSFER_SRC_OPTIMAL:                         return  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case IL_TRANSFER_DST_OPTIMAL:                         return  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case IL_COLOR_ATTACHMENT_OPTIMAL:                     return  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case IL_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:             return  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case IL_DEPTH_STENCIL_READ_ONLY_OPTIMAL:              return  VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case IL_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:   return  VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        case IL_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:   return  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        case IL_DEPTH_ATTACHMENT_OPTIMAL:                     return  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case IL_DEPTH_READ_ONLY_OPTIMAL:                      return  VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        case IL_STENCIL_ATTACHMENT_OPTIMAL:                   return  VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        case IL_STENCIL_READ_ONLY_OPTIMAL:                    return  VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
        case IL_SHADER_READ_ONLY_OPTIMAL:                     return  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case IL_READ_ONLY_OPTIMAL:                            return  VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        case IL_ATTACHMENT_OPTIMAL:                           return  VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        case IL_PRESENT_SRC:                                  return  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case IL_VIDEO_DECODE_DST:                             return  VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR;
        case IL_VIDEO_DECODE_SRC:                             return  VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR;
        case IL_VIDEO_DECODE_DPB:                             return  VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
        case IL_SHARED_PRESENT:                               return  VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
        case IL_FRAGMENT_DENSITY_MAP_OPTIMAL:                 return  VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        case IL_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL:     return  VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        default:
            assert(0!=0);
            return  VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    static  auto    system2Native(ImageUsageBits ubits)
    {
        VkFlags flags   =   0;
        if(ubits.hasFlag(USAGE_TRANSFER_SRC))                   flags    |=  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if(ubits.hasFlag(USAGE_TRANSFER_DST))                   flags    |=  VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if(ubits.hasFlag(USAGE_SAMPLED))                        flags    |=  VK_IMAGE_USAGE_SAMPLED_BIT;
        if(ubits.hasFlag(USAGE_STORAGE))                        flags    |=  VK_IMAGE_USAGE_STORAGE_BIT;
        if(ubits.hasFlag(USAGE_COLOR_ATTACHMENT))               flags    |=  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if(ubits.hasFlag(USAGE_DEPTH_STENCIL_ATTACHMENT))       flags    |=  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if(ubits.hasFlag(USAGE_TRANSIENT_ATTACHMENT))           flags    |=  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        if(ubits.hasFlag(USAGE_INPUT_ATTACHMENT))               flags    |=  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if(ubits.hasFlag(USAGE_VIDEO_DECODE_DST))               flags    |=  VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
        if(ubits.hasFlag(USAGE_VIDEO_DECODE_SRC))               flags    |=  VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
        if(ubits.hasFlag(USAGE_VIDEO_DECODE_DPB))               flags    |=  VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
        if(ubits.hasFlag(USAGE_FRAGMENT_DENSITY_MAP))           flags    |=  VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
        if(ubits.hasFlag(USAGE_HOST_TRANSFER))                  flags    |=  VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
        if(ubits.hasFlag(USAGE_ATTACHMENT_FEEDBACK_LOOP))       flags    |=  VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT;
        if(ubits.hasFlag(USAGE_SAMPLE_WEIGHT_QCOM))             flags    |=  VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM;
        if(ubits.hasFlag(USAGE_SAMPLE_BLOCK_MATCH_QCOM))        flags    |=  VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM;
        return flags;
    }
    static  auto    system2Native(FEImageType type)
    {
        switch(type)
        {
        case IT_1D:     return  VK_IMAGE_TYPE_1D;
        case IT_2D:     return  VK_IMAGE_TYPE_2D;
        case IT_3D:     return  VK_IMAGE_TYPE_3D;
        default:
            assert(0!=0);
            return  VK_IMAGE_TYPE_2D;
        }
    }
    
    inline VkImageMemoryBarrier createImageMemoryBarrier()
    {
        VkImageMemoryBarrier imBarrier {};
        imBarrier.sType                 =   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imBarrier.srcQueueFamilyIndex   =   VK_QUEUE_FAMILY_IGNORED;
        imBarrier.dstQueueFamilyIndex   =   VK_QUEUE_FAMILY_IGNORED;
        return imBarrier;
    }

    inline  void    insertImageMemoryBarrier(   VkCommandBuffer cmdbuffer,
                                                VkImage image,
                                                VkAccessFlags srcAccessMask,
                                                VkAccessFlags dstAccessMask,
                                                VkImageLayout oldImageLayout,
                                                VkImageLayout newImageLayout,
                                                VkPipelineStageFlags srcStageMask,
                                                VkPipelineStageFlags dstStageMask,
                                                VkImageSubresourceRange subresourceRange)
    {
        VkImageMemoryBarrier imBarrier  =   createImageMemoryBarrier();
        imBarrier.srcAccessMask         =   srcAccessMask;
        imBarrier.dstAccessMask         =   dstAccessMask;
        imBarrier.oldLayout             =   oldImageLayout;
        imBarrier.newLayout             =   newImageLayout;
        imBarrier.image                 =   image;
        imBarrier.subresourceRange      =   subresourceRange;

        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imBarrier);
    }
}



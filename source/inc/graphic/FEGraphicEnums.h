#pragma     once
#include    "../FEFlags.hpp"
#include    "../FEStringHelper.hpp"
#include    "../mesh/FEPrimitive.hpp"

namespace   FE
{
    enum  GBType
    {
        GPUBufferType_Vertex,
        GPUBufferType_Index,
        GPUBufferType_Uniform,
    };

    enum    PLType
    {
        PL_GRAPIC,
        PL_COMPUTE,
        PL_RAY_TRACING,
    };

    enum    FEIndexType
    {
        INDEX_UINT16    =   0,
        INDEX_UINT32    =   1,
        INDEX_UINT8     =   2,
    };

    enum    MemoryUsage :uint32_t
    {
        DEVICE_DEFAULT_BIT      =   0x00000000,
        /// <summary>
        /// GPU端内存
        /// </summary>
        DEVICE_LOCAL_BIT        =   0x00000001,
        /// <summary>
        /// CPU端内存
        /// </summary>
        HOST_VISIBLE_BIT        =   0x00000002,
        /// <summary>
        /// CPU/GPU都可见
        /// </summary>
        HOST_COHERENT_BIT       =   0x00000004,
        /// <summary>
        /// HOST_CACHED
        /// </summary>
        HOST_CACHED_BIT         =   0x00000008,
        /// <summary>
        /// GPU 懒加载
        /// </summary>
        LAZILY_ALLOCATED_BIT    =   0x00000010,
    } ;
    using   MemoryUsages        =  FEFlags<MemoryUsage,uint>;


    enum    BufferUsage :uint32_t
    {
        TRANSFER_SRC_BIT                                =   0x00000001,
        TRANSFER_DST_BIT                                =   0x00000002,
        UNIFORM_TEXEL_BUFFER_BIT                        =   0x00000004,
        STORAGE_TEXEL_BUFFER_BIT                        =   0x00000008,
        UNIFORM_BUFFER_BIT                              =   0x00000010,
        STORAGE_BUFFER_BIT                              =   0x00000020,
        INDEX_BUFFER_BIT                                =   0x00000040,
        VERTEX_BUFFER_BIT                               =   0x00000080,
        INDIRECT_BUFFER_BIT                             =   0x00000100,
        SHADER_DEVICE_ADDRESS_BIT                       =   0x00020000,
        VIDEO_DECODE_SRC_BIT                            =   0x00002000,
        VIDEO_DECODE_DST_BIT                            =   0x00004000,
        TRANSFORM_FEEDBACK_BUFFER_BIT                   =   0x00000800,
        TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT           =   0x00001000,
        CONDITIONAL_RENDERING_BIT                       =   0x00000200,
        ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY    =   0x00080000,
        ACCELERATION_STRUCTURE_STORAGE_BIT              =   0x00100000,
        SHADER_BINDING_TABLE_BIT                        =   0x00000400,
        VIDEO_ENCODE_DST_BIT                            =   0x00008000,
        VIDEO_ENCODE_SRC_BIT                            =   0x00010000,
        SAMPLER_DESCRIPTOR_BUFFER_BIT                   =   0x00200000,
        RESOURCE_DESCRIPTOR_BUFFER_BIT                  =   0x00400000,
        PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT          =   0x04000000,
        MICROMAP_BUILD_INPUT_READ_ONLY_BIT              =   0x00800000,
        MICROMAP_STORAGE_BIT                            =   0x01000000,
    };

    using   BufferUsages        =  FEFlags<BufferUsage,uint>;


    enum    FEDescType :uint16_t
    {
        DT_SAMPLER                          =   0,
        DT_COMBINED_IMAGE_SAMPLER           =   1,
        DT_SAMPLED_IMAGE                    =   2,
        DT_STORAGE_IMAGE                    =   3,
        DT_UNIFORM_TEXEL_BUFFER             =   4,
        DT_STORAGE_TEXEL_BUFFER             =   5,
        DT_UNIFORM_BUFFER                   =   6,
        DT_STORAGE_BUFFER                   =   7,
        DT_UNIFORM_BUFFER_DYNAMIC           =   8,
        DT_STORAGE_BUFFER_DYNAMIC           =   9,
        DT_INPUT_ATTACHMENT                =   10,
    };
    enum    FEShaderType :uint32
    {
        ST_VERTEX_BIT                   =   (1<<0),
        ST_TESSELLATION_CONTROL_BIT     =   ((ST_VERTEX_BIT                  )<<1),
        ST_TESSELLATION_EVALUATION_BIT  =   ((ST_TESSELLATION_CONTROL_BIT    )<<1),
        ST_GEOMETRY_BIT                 =   ((ST_TESSELLATION_EVALUATION_BIT )<<1),
        ST_FRAGMENT_BIT                 =   ((ST_GEOMETRY_BIT                )<<1),
        ST_COMPUTE_BIT                  =   ((ST_FRAGMENT_BIT                )<<1),
        ST_RAYGEN_BIT                   =   ((ST_COMPUTE_BIT                 )<<1),
        ST_ANY_HIT_BIT                  =   ((ST_RAYGEN_BIT                  )<<1),
        ST_CLOSEST_HIT_BIT              =   ((ST_ANY_HIT_BIT                 )<<1),
        ST_MISS_BIT                     =   ((ST_CLOSEST_HIT_BIT             )<<1),
        ST_INTERSECTION_BIT             =   ((ST_MISS_BIT                    )<<1),
        ST_CALLABLE_BIT                 =   ((ST_INTERSECTION_BIT            )<<1),
        ST_TASK_BIT                     =   ((ST_CALLABLE_BIT                )<<1),
        ST_MESH_BIT                     =   ((ST_TASK_BIT                    )<<1),
    };

    using   ShaderTypes                 =   FEFlags<FEShaderType,uint>;

    /// <summary>
    /// 输入槽信息
    /// </summary>
    enum    FEInputSlot :uint32
    {
        IS_VERTEX_POS               =   1,
        IS_VERTEX_NOR               =   (IS_VERTEX_POS          ) << 1,
        IS_VERTEX_COLOR0            =   (IS_VERTEX_NOR          ) << 1,
        IS_VERTEX_COLOR1            =   (IS_VERTEX_COLOR0       ) << 1,
        IS_VERTEX_LOD               =   (IS_VERTEX_COLOR1       ) << 1,
        IS_VERTEX_TEXCOORD0         =   (IS_VERTEX_LOD          ) << 1,
        IS_VERTEX_TEXCOORD1         =   (IS_VERTEX_TEXCOORD0    ) << 1,
        IS_VERTEX_TEXCOORD2         =   (IS_VERTEX_TEXCOORD1    ) << 1,
        IS_VERTEX_TEXCOORD3         =   (IS_VERTEX_TEXCOORD2    ) << 1,
        IS_VERTEX_TANGENT           =   (IS_VERTEX_TEXCOORD3    ) << 1,
        IS_VERTEX_BONE_INDEX        =   (IS_VERTEX_TANGENT      ) << 1,
        IS_VERTEX_BONE_WEIGHT       =   (IS_VERTEX_BONE_INDEX   ) << 1,
        IS_VERTEX_TARGET0           =   (IS_VERTEX_BONE_WEIGHT  ) << 1,
        IS_VERTEX_TARGET1           =   (IS_VERTEX_TARGET0      ) << 1,
        IS_VERTEX_TARGET2           =   (IS_VERTEX_TARGET1      ) << 1,
        IS_VERTEX_TARGET3           =   (IS_VERTEX_TARGET2      ) << 1,
        IS_INSTANCE_TRANSLATE       =   (IS_VERTEX_TARGET3      ) << 1,
        IS_INSTANCE_ITRANSLATE      =   (IS_INSTANCE_TRANSLATE  ) << 1,
        IS_INSTANCE_SCALE           =   (IS_INSTANCE_ITRANSLATE ) << 1,
        IS_INSTANCE_ROTATE          =   (IS_INSTANCE_SCALE      ) << 1,
        IS_INSTANCE_BOUNDSPHERE     =   (IS_INSTANCE_ROTATE     ) << 1,
        IS_INSTANCE_INSTANCE        =   (IS_INSTANCE_BOUNDSPHERE) << 1,
        IS_INSTANCE_LOD_INDEX       =   (IS_INSTANCE_INSTANCE   ) << 1,
        IS_INSTANCE_FLAG            =   (IS_INSTANCE_LOD_INDEX  ) << 1,
        IS_INSTANCE_MAT_C0          =   (IS_INSTANCE_FLAG       ) << 1,
        IS_INSTANCE_MAT_C1          =   (IS_INSTANCE_MAT_C0     ) << 1,
        IS_INSTANCE_MAT_C2          =   (IS_INSTANCE_MAT_C1     ) << 1,
        IS_INSTANCE_MAT_C3          =   (IS_INSTANCE_MAT_C2     ) << 1,
        IS_INSTANCE_COLOR           =   (IS_INSTANCE_MAT_C3     ) << 1,
        
    };

    using   InputSlotBits   =    FEFlags<FEInputSlot,uint32>;

    enum    DynamicState:uint16
    {
        VIEWPORT                               ,    
        SCISSOR                                ,    
        LINE_WIDTH                             ,    
        DEPTH_BIAS                             ,    
        BLEND_CONSTANTS                        ,    
        DEPTH_BOUNDS                           ,    
        STENCIL_COMPARE_MASK                   ,    
        STENCIL_WRITE_MASK                     ,    
        STENCIL_REFERENCE                      ,    
        CULL_MODE                              ,    
        FRONT_FACE                             ,    
        PRIMITIVE_TOPOLOGY                     ,    
        VIEWPORT_WITH_COUNT                    ,    
        SCISSOR_WITH_COUNT                     ,    
        VERTEX_INPUT_BINDING_STRIDE            ,    
        DEPTH_TEST_ENABLE                      ,    
        DEPTH_WRITE_ENABLE                     ,    
        DEPTH_COMPARE_OP                       ,    
        DEPTH_BOUNDS_TEST_ENABLE               ,    
        STENCIL_TEST_ENABLE                    ,    
        STENCIL_OP                             ,    
        RASTERIZER_DISCARD_ENABLE              ,    
        DEPTH_BIAS_ENABLE                      ,    
        PRIMITIVE_RESTART_ENABLE               ,    
        VIEWPORT_W_SCALING                     ,
        DISCARD_RECTANGLE                      ,
        DISCARD_RECTANGLE_ENABLE               ,
        DISCARD_RECTANGLE_MODE                 ,
        SAMPLE_LOCATIONS                       ,
        RAY_TRACING_PIPELINE_STACK_SIZE        ,
        VIEWPORT_SHADING_RATE_PALETTE          ,
        VIEWPORT_COARSE_SAMPLE_ORDER           ,
        EXCLUSIVE_SCISSOR_ENABLE               ,
        EXCLUSIVE_SCISSOR                      ,
        FRAGMENT_SHADING_RATE                  ,
        LINE_STIPPLE                           ,
        VERTEX_INPUT                           ,
        PATCH_CONTROL_POINTS                   ,
        LOGIC_OP                               ,
        COLOR_WRITE_ENABLE                     ,
        TESSELLATION_DOMAIN_ORIGIN             ,
        DEPTH_CLAMP_ENABLE                     ,
        POLYGON_MODE                           ,
        RASTERIZATION_SAMPLES                  ,
        SAMPLE_MASK                            ,
        ALPHA_TO_COVERAGE_ENABLE               ,
        ALPHA_TO_ONE_ENABLE                    ,
        LOGIC_OP_ENABLE                        ,
        COLOR_BLEND_ENABLE                     ,
        COLOR_BLEND_EQUATION                   ,
        COLOR_WRITE_MASK                       ,
        RASTERIZATION_STREAM                   ,
        CONSERVATIVE_RASTERIZATION_MODE        ,
        EXTRA_PRIMITIVE_OVERESTIMATION_SIZE    ,
        DEPTH_CLIP_ENABLE                      ,
        SAMPLE_LOCATIONS_ENABLE                ,
        COLOR_BLEND_ADVANCED                   ,
        PROVOKING_VERTEX_MODE                  ,
        LINE_RASTERIZATION_MODE                ,
        LINE_STIPPLE_ENABLE                    ,
        DEPTH_CLIP_NEGATIVE_ONE_TO_ONE         ,
        VIEWPORT_W_SCALING_ENABLE              ,
        VIEWPORT_SWIZZLE                       ,
        COVERAGE_TO_COLOR_ENABLE               ,
        COVERAGE_TO_COLOR_LOCATION             ,
        COVERAGE_MODULATION_MODE               ,
        COVERAGE_MODULATION_TABLE_ENABLE       ,
        COVERAGE_MODULATION_TABLE              ,
        SHADING_RATE_IMAGE_ENABLE              ,
        REPRESENTATIVE_FRAGMENT_TEST_ENABLE    ,
        COVERAGE_REDUCTION_MODE                ,
        ATTACHMENT_FEEDBACK_LOOP_ENABLE        ,
    };

    using   DynamicStates   =   std::vector<DynamicState>;

    inline  auto    nameOfState(DynamicState state)
    {
        switch(state)
        {
        case VIEWPORT                               :   return  "VIEWPORT";
        case SCISSOR                                :   return  "SCISSOR";
        case LINE_WIDTH                             :   return  "LINE_WIDTH";
        case DEPTH_BIAS                             :   return  "DEPTH_BIAS";
        case BLEND_CONSTANTS                        :   return  "BLEND_CONSTANTS";
        case DEPTH_BOUNDS                           :   return  "DEPTH_BOUNDS";
        case STENCIL_COMPARE_MASK                   :   return  "STENCIL_COMPARE_MASK";
        case STENCIL_WRITE_MASK                     :   return  "STENCIL_WRITE_MASK";
        case STENCIL_REFERENCE                      :   return  "STENCIL_REFERENCE";
        case CULL_MODE                              :   return  "CULL_MODE";
        case FRONT_FACE                             :   return  "FRONT_FACE";
        case PRIMITIVE_TOPOLOGY                     :   return  "PRIMITIVE_TOPOLOGY";
        case VIEWPORT_WITH_COUNT                    :   return  "VIEWPORT_WITH_COUNT";
        case SCISSOR_WITH_COUNT                     :   return  "SCISSOR_WITH_COUNT";
        case VERTEX_INPUT_BINDING_STRIDE            :   return  "VERTEX_INPUT_BINDING_STRIDE";
        case DEPTH_TEST_ENABLE                      :   return  "DEPTH_TEST_ENABLE";
        case DEPTH_WRITE_ENABLE                     :   return  "DEPTH_WRITE_ENABLE";
        case DEPTH_COMPARE_OP                       :   return  "DEPTH_COMPARE_OP";
        case DEPTH_BOUNDS_TEST_ENABLE               :   return  "DEPTH_BOUNDS_TEST_ENABLE";
        case STENCIL_TEST_ENABLE                    :   return  "STENCIL_TEST_ENABLE";
        case STENCIL_OP                             :   return  "STENCIL_OP";
        case RASTERIZER_DISCARD_ENABLE              :   return  "RASTERIZER_DISCARD_ENABLE";
        case DEPTH_BIAS_ENABLE                      :   return  "DEPTH_BIAS_ENABLE";
        case PRIMITIVE_RESTART_ENABLE               :   return  "PRIMITIVE_RESTART_ENABLE";
        case VIEWPORT_W_SCALING                     :   return  "VIEWPORT_W_SCALING";
        case DISCARD_RECTANGLE                      :   return  "DISCARD_RECTANGLE";
        case DISCARD_RECTANGLE_ENABLE               :   return  "DISCARD_RECTANGLE_ENABLE";
        case DISCARD_RECTANGLE_MODE                 :   return  "DISCARD_RECTANGLE_MODE";
        case SAMPLE_LOCATIONS                       :   return  "SAMPLE_LOCATIONS";
        case RAY_TRACING_PIPELINE_STACK_SIZE        :   return  "RAY_TRACING_PIPELINE_STACK_SIZE";
        case VIEWPORT_SHADING_RATE_PALETTE          :   return  "VIEWPORT_SHADING_RATE_PALETTE";
        case VIEWPORT_COARSE_SAMPLE_ORDER           :   return  "VIEWPORT_COARSE_SAMPLE_ORDER";
        case EXCLUSIVE_SCISSOR_ENABLE               :   return  "EXCLUSIVE_SCISSOR_ENABLE";
        case EXCLUSIVE_SCISSOR                      :   return  "EXCLUSIVE_SCISSOR";
        case FRAGMENT_SHADING_RATE                  :   return  "FRAGMENT_SHADING_RATE";
        case LINE_STIPPLE                           :   return  "LINE_STIPPLE";
        case VERTEX_INPUT                           :   return  "VERTEX_INPUT";
        case PATCH_CONTROL_POINTS                   :   return  "PATCH_CONTROL_POINTS";
        case LOGIC_OP                               :   return  "LOGIC_OP";
        case COLOR_WRITE_ENABLE                     :   return  "COLOR_WRITE_ENABLE";
        case TESSELLATION_DOMAIN_ORIGIN             :   return  "TESSELLATION_DOMAIN_ORIGIN";
        case DEPTH_CLAMP_ENABLE                     :   return  "DEPTH_CLAMP_ENABLE";
        case POLYGON_MODE                           :   return  "POLYGON_MODE";
        case RASTERIZATION_SAMPLES                  :   return  "RASTERIZATION_SAMPLES";
        case SAMPLE_MASK                            :   return  "SAMPLE_MASK";
        case ALPHA_TO_COVERAGE_ENABLE               :   return  "ALPHA_TO_COVERAGE_ENABLE";
        case ALPHA_TO_ONE_ENABLE                    :   return  "ALPHA_TO_ONE_ENABLE";
        case LOGIC_OP_ENABLE                        :   return  "LOGIC_OP_ENABLE";
        case COLOR_BLEND_ENABLE                     :   return  "COLOR_BLEND_ENABLE";
        case COLOR_BLEND_EQUATION                   :   return  "COLOR_BLEND_EQUATION";
        case COLOR_WRITE_MASK                       :   return  "COLOR_WRITE_MASK";
        case RASTERIZATION_STREAM                   :   return  "RASTERIZATION_STREAM";
        case CONSERVATIVE_RASTERIZATION_MODE        :   return  "CONSERVATIVE_RASTERIZATION_MODE";
        case EXTRA_PRIMITIVE_OVERESTIMATION_SIZE    :   return  "EXTRA_PRIMITIVE_OVERESTIMATION_SIZE";
        case DEPTH_CLIP_ENABLE                      :   return  "DEPTH_CLIP_ENABLE";
        case SAMPLE_LOCATIONS_ENABLE                :   return  "SAMPLE_LOCATIONS_ENABLE";
        case COLOR_BLEND_ADVANCED                   :   return  "COLOR_BLEND_ADVANCED";
        case PROVOKING_VERTEX_MODE                  :   return  "PROVOKING_VERTEX_MODE";
        case LINE_RASTERIZATION_MODE                :   return  "LINE_RASTERIZATION_MODE";
        case LINE_STIPPLE_ENABLE                    :   return  "LINE_STIPPLE_ENABLE";
        case DEPTH_CLIP_NEGATIVE_ONE_TO_ONE         :   return  "DEPTH_CLIP_NEGATIVE_ONE_TO_ONE";
        case VIEWPORT_W_SCALING_ENABLE              :   return  "VIEWPORT_W_SCALING_ENABLE";
        case VIEWPORT_SWIZZLE                       :   return  "VIEWPORT_SWIZZLE";
        case COVERAGE_TO_COLOR_ENABLE               :   return  "COVERAGE_TO_COLOR_ENABLE";
        case COVERAGE_TO_COLOR_LOCATION             :   return  "COVERAGE_TO_COLOR_LOCATION";
        case COVERAGE_MODULATION_MODE               :   return  "COVERAGE_MODULATION_MODE";
        case COVERAGE_MODULATION_TABLE_ENABLE       :   return  "COVERAGE_MODULATION_TABLE_ENABLE";
        case COVERAGE_MODULATION_TABLE              :   return  "COVERAGE_MODULATION_TABLE";
        case SHADING_RATE_IMAGE_ENABLE              :   return  "SHADING_RATE_IMAGE_ENABLE";
        case REPRESENTATIVE_FRAGMENT_TEST_ENABLE    :   return  "REPRESENTATIVE_FRAGMENT_TEST_ENABLE";
        case COVERAGE_REDUCTION_MODE                :   return  "COVERAGE_REDUCTION_MODE";
        case ATTACHMENT_FEEDBACK_LOOP_ENABLE        :   return  "ATTACHMENT_FEEDBACK_LOOP_ENABLE";
        }
        return  "";
    }

    inline  auto    stateFromName(const char* name)
    {
        if(     _stricmp(name,nameOfState(VIEWPORT                               )) == 0)    return  VIEWPORT                               ;
        else if(_stricmp(name,nameOfState(SCISSOR                                )) == 0)    return  SCISSOR                                ;
        else if(_stricmp(name,nameOfState(LINE_WIDTH                             )) == 0)    return  LINE_WIDTH                             ;
        else if(_stricmp(name,nameOfState(DEPTH_BIAS                             )) == 0)    return  DEPTH_BIAS                             ;
        else if(_stricmp(name,nameOfState(BLEND_CONSTANTS                        )) == 0)    return  BLEND_CONSTANTS                        ;
        else if(_stricmp(name,nameOfState(DEPTH_BOUNDS                           )) == 0)    return  DEPTH_BOUNDS                           ;
        else if(_stricmp(name,nameOfState(STENCIL_COMPARE_MASK                   )) == 0)    return  STENCIL_COMPARE_MASK                   ;
        else if(_stricmp(name,nameOfState(STENCIL_WRITE_MASK                     )) == 0)    return  STENCIL_WRITE_MASK                     ;
        else if(_stricmp(name,nameOfState(STENCIL_REFERENCE                      )) == 0)    return  STENCIL_REFERENCE                      ;
        else if(_stricmp(name,nameOfState(CULL_MODE                              )) == 0)    return  CULL_MODE                              ;
        else if(_stricmp(name,nameOfState(FRONT_FACE                             )) == 0)    return  FRONT_FACE                             ;
        else if(_stricmp(name,nameOfState(PRIMITIVE_TOPOLOGY                     )) == 0)    return  PRIMITIVE_TOPOLOGY                     ;
        else if(_stricmp(name,nameOfState(VIEWPORT_WITH_COUNT                    )) == 0)    return  VIEWPORT_WITH_COUNT                    ;
        else if(_stricmp(name,nameOfState(SCISSOR_WITH_COUNT                     )) == 0)    return  SCISSOR_WITH_COUNT                     ;
        else if(_stricmp(name,nameOfState(VERTEX_INPUT_BINDING_STRIDE            )) == 0)    return  VERTEX_INPUT_BINDING_STRIDE            ;
        else if(_stricmp(name,nameOfState(DEPTH_TEST_ENABLE                      )) == 0)    return  DEPTH_TEST_ENABLE                      ;
        else if(_stricmp(name,nameOfState(DEPTH_WRITE_ENABLE                     )) == 0)    return  DEPTH_WRITE_ENABLE                     ;
        else if(_stricmp(name,nameOfState(DEPTH_COMPARE_OP                       )) == 0)    return  DEPTH_COMPARE_OP                       ;
        else if(_stricmp(name,nameOfState(DEPTH_BOUNDS_TEST_ENABLE               )) == 0)    return  DEPTH_BOUNDS_TEST_ENABLE               ;
        else if(_stricmp(name,nameOfState(STENCIL_TEST_ENABLE                    )) == 0)    return  STENCIL_TEST_ENABLE                    ;
        else if(_stricmp(name,nameOfState(STENCIL_OP                             )) == 0)    return  STENCIL_OP                             ;
        else if(_stricmp(name,nameOfState(RASTERIZER_DISCARD_ENABLE              )) == 0)    return  RASTERIZER_DISCARD_ENABLE              ;
        else if(_stricmp(name,nameOfState(DEPTH_BIAS_ENABLE                      )) == 0)    return  DEPTH_BIAS_ENABLE                      ;
        else if(_stricmp(name,nameOfState(PRIMITIVE_RESTART_ENABLE               )) == 0)    return  PRIMITIVE_RESTART_ENABLE               ;
        else if(_stricmp(name,nameOfState(VIEWPORT_W_SCALING                     )) == 0)    return  VIEWPORT_W_SCALING                     ;
        else if(_stricmp(name,nameOfState(DISCARD_RECTANGLE                      )) == 0)    return  DISCARD_RECTANGLE                      ;
        else if(_stricmp(name,nameOfState(DISCARD_RECTANGLE_ENABLE               )) == 0)    return  DISCARD_RECTANGLE_ENABLE               ;
        else if(_stricmp(name,nameOfState(DISCARD_RECTANGLE_MODE                 )) == 0)    return  DISCARD_RECTANGLE_MODE                 ;
        else if(_stricmp(name,nameOfState(SAMPLE_LOCATIONS                       )) == 0)    return  SAMPLE_LOCATIONS                       ;
        else if(_stricmp(name,nameOfState(RAY_TRACING_PIPELINE_STACK_SIZE        )) == 0)    return  RAY_TRACING_PIPELINE_STACK_SIZE        ;
        else if(_stricmp(name,nameOfState(VIEWPORT_SHADING_RATE_PALETTE          )) == 0)    return  VIEWPORT_SHADING_RATE_PALETTE          ;
        else if(_stricmp(name,nameOfState(VIEWPORT_COARSE_SAMPLE_ORDER           )) == 0)    return  VIEWPORT_COARSE_SAMPLE_ORDER           ;
        else if(_stricmp(name,nameOfState(EXCLUSIVE_SCISSOR_ENABLE               )) == 0)    return  EXCLUSIVE_SCISSOR_ENABLE               ;
        else if(_stricmp(name,nameOfState(EXCLUSIVE_SCISSOR                      )) == 0)    return  EXCLUSIVE_SCISSOR                      ;
        else if(_stricmp(name,nameOfState(FRAGMENT_SHADING_RATE                  )) == 0)    return  FRAGMENT_SHADING_RATE                  ;
        else if(_stricmp(name,nameOfState(LINE_STIPPLE                           )) == 0)    return  LINE_STIPPLE                           ;
        else if(_stricmp(name,nameOfState(VERTEX_INPUT                           )) == 0)    return  VERTEX_INPUT                           ;
        else if(_stricmp(name,nameOfState(PATCH_CONTROL_POINTS                   )) == 0)    return  PATCH_CONTROL_POINTS                   ;
        else if(_stricmp(name,nameOfState(LOGIC_OP                               )) == 0)    return  LOGIC_OP                               ;
        else if(_stricmp(name,nameOfState(COLOR_WRITE_ENABLE                     )) == 0)    return  COLOR_WRITE_ENABLE                     ;
        else if(_stricmp(name,nameOfState(TESSELLATION_DOMAIN_ORIGIN             )) == 0)    return  TESSELLATION_DOMAIN_ORIGIN             ;
        else if(_stricmp(name,nameOfState(DEPTH_CLAMP_ENABLE                     )) == 0)    return  DEPTH_CLAMP_ENABLE                     ;
        else if(_stricmp(name,nameOfState(POLYGON_MODE                           )) == 0)    return  POLYGON_MODE                           ;
        else if(_stricmp(name,nameOfState(RASTERIZATION_SAMPLES                  )) == 0)    return  RASTERIZATION_SAMPLES                  ;
        else if(_stricmp(name,nameOfState(SAMPLE_MASK                            )) == 0)    return  SAMPLE_MASK                            ;
        else if(_stricmp(name,nameOfState(ALPHA_TO_COVERAGE_ENABLE               )) == 0)    return  ALPHA_TO_COVERAGE_ENABLE               ;
        else if(_stricmp(name,nameOfState(ALPHA_TO_ONE_ENABLE                    )) == 0)    return  ALPHA_TO_ONE_ENABLE                    ;
        else if(_stricmp(name,nameOfState(LOGIC_OP_ENABLE                        )) == 0)    return  LOGIC_OP_ENABLE                        ;
        else if(_stricmp(name,nameOfState(COLOR_BLEND_ENABLE                     )) == 0)    return  COLOR_BLEND_ENABLE                     ;
        else if(_stricmp(name,nameOfState(COLOR_BLEND_EQUATION                   )) == 0)    return  COLOR_BLEND_EQUATION                   ;
        else if(_stricmp(name,nameOfState(COLOR_WRITE_MASK                       )) == 0)    return  COLOR_WRITE_MASK                       ;
        else if(_stricmp(name,nameOfState(RASTERIZATION_STREAM                   )) == 0)    return  RASTERIZATION_STREAM                   ;
        else if(_stricmp(name,nameOfState(CONSERVATIVE_RASTERIZATION_MODE        )) == 0)    return  CONSERVATIVE_RASTERIZATION_MODE        ;
        else if(_stricmp(name,nameOfState(EXTRA_PRIMITIVE_OVERESTIMATION_SIZE    )) == 0)    return  EXTRA_PRIMITIVE_OVERESTIMATION_SIZE    ;
        else if(_stricmp(name,nameOfState(DEPTH_CLIP_ENABLE                      )) == 0)    return  DEPTH_CLIP_ENABLE                      ;
        else if(_stricmp(name,nameOfState(SAMPLE_LOCATIONS_ENABLE                )) == 0)    return  SAMPLE_LOCATIONS_ENABLE                ;
        else if(_stricmp(name,nameOfState(COLOR_BLEND_ADVANCED                   )) == 0)    return  COLOR_BLEND_ADVANCED                   ;
        else if(_stricmp(name,nameOfState(PROVOKING_VERTEX_MODE                  )) == 0)    return  PROVOKING_VERTEX_MODE                  ;
        else if(_stricmp(name,nameOfState(LINE_RASTERIZATION_MODE                )) == 0)    return  LINE_RASTERIZATION_MODE                ;
        else if(_stricmp(name,nameOfState(LINE_STIPPLE_ENABLE                    )) == 0)    return  LINE_STIPPLE_ENABLE                    ;
        else if(_stricmp(name,nameOfState(DEPTH_CLIP_NEGATIVE_ONE_TO_ONE         )) == 0)    return  DEPTH_CLIP_NEGATIVE_ONE_TO_ONE         ;
        else if(_stricmp(name,nameOfState(VIEWPORT_W_SCALING_ENABLE              )) == 0)    return  VIEWPORT_W_SCALING_ENABLE              ;
        else if(_stricmp(name,nameOfState(VIEWPORT_SWIZZLE                       )) == 0)    return  VIEWPORT_SWIZZLE                       ;
        else if(_stricmp(name,nameOfState(COVERAGE_TO_COLOR_ENABLE               )) == 0)    return  COVERAGE_TO_COLOR_ENABLE               ;
        else if(_stricmp(name,nameOfState(COVERAGE_TO_COLOR_LOCATION             )) == 0)    return  COVERAGE_TO_COLOR_LOCATION             ;
        else if(_stricmp(name,nameOfState(COVERAGE_MODULATION_MODE               )) == 0)    return  COVERAGE_MODULATION_MODE               ;
        else if(_stricmp(name,nameOfState(COVERAGE_MODULATION_TABLE_ENABLE       )) == 0)    return  COVERAGE_MODULATION_TABLE_ENABLE       ;
        else if(_stricmp(name,nameOfState(COVERAGE_MODULATION_TABLE              )) == 0)    return  COVERAGE_MODULATION_TABLE              ;
        else if(_stricmp(name,nameOfState(SHADING_RATE_IMAGE_ENABLE              )) == 0)    return  SHADING_RATE_IMAGE_ENABLE              ;
        else if(_stricmp(name,nameOfState(REPRESENTATIVE_FRAGMENT_TEST_ENABLE    )) == 0)    return  REPRESENTATIVE_FRAGMENT_TEST_ENABLE    ;
        else if(_stricmp(name,nameOfState(COVERAGE_REDUCTION_MODE                )) == 0)    return  COVERAGE_REDUCTION_MODE                ;
        else if(_stricmp(name,nameOfState(ATTACHMENT_FEEDBACK_LOOP_ENABLE        )) == 0)    return  ATTACHMENT_FEEDBACK_LOOP_ENABLE        ;
        else                                                                                 return  VIEWPORT;
    }

   
}

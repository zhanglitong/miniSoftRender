#pragma     once

#include    "graphic/FEGPipeline.h"
#include    "FEVulkan.h"
#include    "VKDSetPool.h"
#include    "VKDSetLayout.h"

namespace   FE
{

    /// <summary>
    /// 
    /// </summary>
    struct  RenderState
    {
        VkDynamicState  _name;
        bool            _enabled;
    };
    /// <summary>
    ///
    /// 
    /// </summary>
    struct  StateBits
    {
        RenderState _VIEWPORT                                   =   {   VK_DYNAMIC_STATE_VIEWPORT                                   ,false};
        RenderState _SCISSOR                                    =   {   VK_DYNAMIC_STATE_SCISSOR                                    ,false};
        RenderState _LINE_WIDTH                                 =   {   VK_DYNAMIC_STATE_LINE_WIDTH                                 ,false};
        RenderState _DEPTH_BIAS                                 =   {   VK_DYNAMIC_STATE_DEPTH_BIAS                                 ,false};
        RenderState _BLEND_CONSTANTS                            =   {   VK_DYNAMIC_STATE_BLEND_CONSTANTS                            ,false};
        RenderState _DEPTH_BOUNDS                               =   {   VK_DYNAMIC_STATE_DEPTH_BOUNDS                               ,false};
        RenderState _STENCIL_COMPARE_MASK                       =   {   VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK                       ,false};
        RenderState _STENCIL_WRITE_MASK                         =   {   VK_DYNAMIC_STATE_STENCIL_WRITE_MASK                         ,false};
        RenderState _STENCIL_REFERENCE                          =   {   VK_DYNAMIC_STATE_STENCIL_REFERENCE                          ,false};
        RenderState _CULL_MODE                                  =   {   VK_DYNAMIC_STATE_CULL_MODE                                  ,false};
        RenderState _FRONT_FACE                                 =   {   VK_DYNAMIC_STATE_FRONT_FACE                                 ,false};
        RenderState _PRIMITIVE_TOPOLOGY                         =   {   VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY                         ,false};
        RenderState _VIEWPORT_WITH_COUNT                        =   {   VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT                        ,false};
        RenderState _SCISSOR_WITH_COUNT                         =   {   VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT                         ,false};
        RenderState _VERTEX_INPUT_BINDING_STRIDE                =   {   VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE                ,false};
        RenderState _DEPTH_TEST_ENABLE                          =   {   VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE                          ,false};
        RenderState _DEPTH_WRITE_ENABLE                         =   {   VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE                         ,false};
        RenderState _DEPTH_COMPARE_OP                           =   {   VK_DYNAMIC_STATE_DEPTH_COMPARE_OP                           ,false};
        RenderState _DEPTH_BOUNDS_TEST_ENABLE                   =   {   VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE                   ,false};
        RenderState _STENCIL_TEST_ENABLE                        =   {   VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE                        ,false};
        RenderState _STENCIL_OP                                 =   {   VK_DYNAMIC_STATE_STENCIL_OP                                 ,false};
        RenderState _RASTERIZER_DISCARD_ENABLE                  =   {   VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE                  ,false};
        RenderState _DEPTH_BIAS_ENABLE                          =   {   VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE                          ,false};
        RenderState _PRIMITIVE_RESTART_ENABLE                   =   {   VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE                   ,false};
        RenderState _VIEWPORT_W_SCALING_NV                      =   {   VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV                      ,false};
        RenderState _DISCARD_RECTANGLE_EXT                      =   {   VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT                      ,false};
        RenderState _DISCARD_RECTANGLE_ENABLE_EXT               =   {   VK_DYNAMIC_STATE_DISCARD_RECTANGLE_ENABLE_EXT               ,false};
        RenderState _DISCARD_RECTANGLE_MODE_EXT                 =   {   VK_DYNAMIC_STATE_DISCARD_RECTANGLE_MODE_EXT                 ,false};
        RenderState _SAMPLE_LOCATIONS_EXT                       =   {   VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT                       ,false};
        RenderState _RAY_TRACING_PIPELINE_STACK_SIZE_KHR        =   {   VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR        ,false};
        RenderState _VIEWPORT_SHADING_RATE_PALETTE_NV           =   {   VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV           ,false};
        RenderState _VIEWPORT_COARSE_SAMPLE_ORDER_NV            =   {   VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV            ,false};
        RenderState _EXCLUSIVE_SCISSOR_ENABLE_NV                =   {   VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_ENABLE_NV                ,false};
        RenderState _EXCLUSIVE_SCISSOR_NV                       =   {   VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV                       ,false};
        RenderState _FRAGMENT_SHADING_RATE_KHR                  =   {   VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR                  ,false};
        RenderState _LINE_STIPPLE_EXT                           =   {   VK_DYNAMIC_STATE_LINE_STIPPLE_EXT                           ,false};
        RenderState _VERTEX_INPUT_EXT                           =   {   VK_DYNAMIC_STATE_VERTEX_INPUT_EXT                           ,false};
        RenderState _PATCH_CONTROL_POINTS_EXT                   =   {   VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT                   ,false};
        RenderState _LOGIC_OP_EXT                               =   {   VK_DYNAMIC_STATE_LOGIC_OP_EXT                               ,false};
        RenderState _COLOR_WRITE_ENABLE_EXT                     =   {   VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT                     ,false};
        RenderState _TESSELLATION_DOMAIN_ORIGIN_EXT             =   {   VK_DYNAMIC_STATE_TESSELLATION_DOMAIN_ORIGIN_EXT             ,false};
        RenderState _DEPTH_CLAMP_ENABLE_EXT                     =   {   VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT                     ,false};
        RenderState _POLYGON_MODE_EXT                           =   {   VK_DYNAMIC_STATE_POLYGON_MODE_EXT                           ,false};
        RenderState _RASTERIZATION_SAMPLES_EXT                  =   {   VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT                  ,false};
        RenderState _SAMPLE_MASK_EXT                            =   {   VK_DYNAMIC_STATE_SAMPLE_MASK_EXT                            ,false};
        RenderState _ALPHA_TO_COVERAGE_ENABLE_EXT               =   {   VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT               ,false};
        RenderState _ALPHA_TO_ONE_ENABLE_EXT                    =   {   VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT                    ,false};
        RenderState _LOGIC_OP_ENABLE_EXT                        =   {   VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT                        ,false};
        RenderState _COLOR_BLEND_ENABLE_EXT                     =   {   VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT                     ,false};
        RenderState _COLOR_BLEND_EQUATION_EXT                   =   {   VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT                   ,false};
        RenderState _COLOR_WRITE_MASK_EXT                       =   {   VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT                       ,false};
        RenderState _RASTERIZATION_STREAM_EXT                   =   {   VK_DYNAMIC_STATE_RASTERIZATION_STREAM_EXT                   ,false};
        RenderState _CONSERVATIVE_RASTERIZATION_MODE_EXT        =   {   VK_DYNAMIC_STATE_CONSERVATIVE_RASTERIZATION_MODE_EXT        ,false};
        RenderState _EXTRA_PRIMITIVE_OVERESTIMATION_SIZE_EXT    =   {   VK_DYNAMIC_STATE_EXTRA_PRIMITIVE_OVERESTIMATION_SIZE_EXT    ,false};
        RenderState _DEPTH_CLIP_ENABLE_EXT                      =   {   VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT                      ,false};
        RenderState _SAMPLE_LOCATIONS_ENABLE_EXT                =   {   VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT                ,false};
        RenderState _COLOR_BLEND_ADVANCED_EXT                   =   {   VK_DYNAMIC_STATE_COLOR_BLEND_ADVANCED_EXT                   ,false};
        RenderState _PROVOKING_VERTEX_MODE_EXT                  =   {   VK_DYNAMIC_STATE_PROVOKING_VERTEX_MODE_EXT                  ,false};
        RenderState _LINE_RASTERIZATION_MODE_EXT                =   {   VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT                ,false};
        RenderState _LINE_STIPPLE_ENABLE_EXT                    =   {   VK_DYNAMIC_STATE_LINE_STIPPLE_ENABLE_EXT                    ,false};
        RenderState _DEPTH_CLIP_NEGATIVE_ONE_TO_ONE_EXT         =   {   VK_DYNAMIC_STATE_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE_EXT         ,false};
        RenderState _VIEWPORT_W_SCALING_ENABLE_NV               =   {   VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_ENABLE_NV               ,false};
        RenderState _VIEWPORT_SWIZZLE_NV                        =   {   VK_DYNAMIC_STATE_VIEWPORT_SWIZZLE_NV                        ,false};
        RenderState _COVERAGE_TO_COLOR_ENABLE_NV                =   {   VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_ENABLE_NV                ,false};
        RenderState _COVERAGE_TO_COLOR_LOCATION_NV              =   {   VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_LOCATION_NV              ,false};
        RenderState _COVERAGE_MODULATION_MODE_NV                =   {   VK_DYNAMIC_STATE_COVERAGE_MODULATION_MODE_NV                ,false};
        RenderState _COVERAGE_MODULATION_TABLE_ENABLE_NV        =   {   VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_ENABLE_NV        ,false};
        RenderState _COVERAGE_MODULATION_TABLE_NV               =   {   VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_NV               ,false};
        RenderState _SHADING_RATE_IMAGE_ENABLE_NV               =   {   VK_DYNAMIC_STATE_SHADING_RATE_IMAGE_ENABLE_NV               ,false};
        RenderState _REPRESENTATIVE_FRAGMENT_TEST_ENABLE_NV     =   {   VK_DYNAMIC_STATE_REPRESENTATIVE_FRAGMENT_TEST_ENABLE_NV     ,false};
        RenderState _COVERAGE_REDUCTION_MODE_NV                 =   {   VK_DYNAMIC_STATE_COVERAGE_REDUCTION_MODE_NV                 ,false};
        RenderState _ATTACHMENT_FEEDBACK_LOOP_ENABLE_EXT        =   {   VK_DYNAMIC_STATE_ATTACHMENT_FEEDBACK_LOOP_ENABLE_EXT        ,false};
    };
    using   DSetPoolVKs =   std::vector<DSetPoolVK>;
    class   VKGPipeline :public TRSObject<VkPipeline,FEGPipeline>
    {
    protected:
        DSetPoolVKs _pools;
    public:
        VKGPipeline(FEContext& ctx)
            :TRSObject<VkPipeline,FEGPipeline>(ctx)
        {}
        VKGPipeline(const VKGPipeline& other)
            :TRSObject<VkPipeline,FEGPipeline>(other)
        {}

        virtual ~VKGPipeline();

        virtual bool    create(const CreateInfo& info)  override;

        virtual DSets   createDSets()  override;

        virtual Handle  nativeLayout() const override
        {
            return  Handle(_layout);
        }
    protected:
        VkPipelineLayout    _layout         =   nullptr;
        DSetLayouts         _dsLayouts      =   {};
    };
}

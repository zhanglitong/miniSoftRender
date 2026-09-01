#pragma     once

#include    "graphic/FEGPipeline.h"
#include    "wgpu.h"
#include    "WGDSetLayout.h"
#include    "WGDSetPool.h"

namespace   FE
{
    /// compute pipeline (WGPUComputePipeline) 的 WebGPU 实现
    /// 与 WGPipeline(图形管线)对应,但创建的是 WGPUComputePipelineDescriptor
    class   WGCPipeline :public TRSObject<WGPUComputePipeline, FEGPipeline>
    {
    public:
        WGCPipeline(FEContext& ctx)
            :TRSObject<WGPUComputePipeline, FEGPipeline>(ctx)
        {}
        WGCPipeline(const WGCPipeline& other)
            :TRSObject<WGPUComputePipeline, FEGPipeline>(other)
        {}

        virtual ~WGCPipeline();

        virtual bool        create(const CreateInfo& info) override;
        virtual DSets       createDSets() override;
        virtual Handle      nativeLayout() const override
        {
            return  Handle(_layout);
        }
        /// 标记为 compute 管线,供 FECmdBuffer/bindPipeline 路由到 wgpuCmdBeginComputePass 等
        virtual PLType      type() const override
        {
            return  PL_COMPUTE;
        }

    protected:
        WGPUPipelineLayout          _layout    =   nullptr;
        DSetLayouts                 _dsLayouts;
        std::vector<DSetPool>       _pools;
    };
}

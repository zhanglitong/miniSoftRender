#pragma     once

#include    "graphic/FEDSetLayout.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGDSetLayout :public TRSObject<WGPUBindGroupLayout, FEDSetLayout>
    {
    public:
        WGDSetLayout(FEContext& ctx)
            :TRSObject<WGPUBindGroupLayout, FEDSetLayout>(ctx)
        {}
        WGDSetLayout(const WGDSetLayout& other)
            :TRSObject<WGPUBindGroupLayout, FEDSetLayout>(other)
        {}

        virtual ~WGDSetLayout();

        virtual bool    create(const CreateInfo& cInf) override;
        virtual DSet    createDSet() override;
    };
}

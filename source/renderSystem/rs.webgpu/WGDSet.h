#pragma     once

#include    "graphic/FEDSet.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGDSet :public TRSObject<WGPUBindGroup, FEDSet>
    {
    public:
        WGDSet(FEContext& ctx)
            :TRSObject<WGPUBindGroup, FEDSet>(ctx)
        {}
        WGDSet(const WGDSet& other)
            :TRSObject<WGPUBindGroup, FEDSet>(other)
        {}

        virtual ~WGDSet();

        virtual bool    create(const CreateInfo& cInf) override;
        virtual bool    update() override;
    };
}
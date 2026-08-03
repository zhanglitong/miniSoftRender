#pragma     once

#include    "graphic/FEFrameBuffer.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGFrameBuffer :public TRSObject<void*, FEFrameBuffer>
    {
    public:
        WGFrameBuffer(FEContext& ctx)
            :TRSObject<void*, FEFrameBuffer>(ctx)
        {}
        WGFrameBuffer(const WGFrameBuffer& other)
            :TRSObject<void*, FEFrameBuffer>(other)
        {}

        virtual ~WGFrameBuffer();

        virtual bool    create(const CreateInfo& cInf) override;
    };
}